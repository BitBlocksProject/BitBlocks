// Copyright (c) 2026 The BitBlocks developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//
// Pre-flight measurement for the OpenSSL -> libsecp256k1 verifier migration.
//
// Consensus signature verification currently runs through OpenSSL (CECKey in
// ecwrapper.cpp). Moving it to libsecp256k1 is only safe if the two agree on
// every signature and public key that our chain actually contains. This walks
// the raw blk000??.dat files and, for every signature-looking and
// pubkey-looking push it finds, asks BOTH libraries whether they accept it.
//
// Both verifiers are always linked (ecwrapper.cpp is in libbitcoin_common and
// libsecp256k1 is in LDADD regardless of USE_SECP256K1), so this runs as a
// single binary in either configuration.
//
// It is skipped unless BBK_BLOCKS_DIR points at a blocks/ directory, so it
// does not affect `make check` for anyone without a synced datadir:
//
//   BBK_BLOCKS_DIR=~/.bitblocks/blocks src/test/test_bitblocks \
//       --run_test=verifier_scan_tests
//
// BLOCKING criteria for the migration (see doc/secp256k1-migration.md):
//   * any pubkey or signature where the two libraries disagree
//   * any hybrid (0x06/0x07) pubkey that is actually spent
//

#include "chainparams.h"
#include "clientversion.h"
#include "ecwrapper.h"
#include "main.h"
#include "primitives/block.h"
#include "script/interpreter.h"
#include "script/script.h"
#include "streams.h"
#include "uint256.h"
#include "utilstrencodings.h"

#include <secp256k1.h>

#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>

#include <stdio.h>
#include <map>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(verifier_scan_tests)

namespace
{
struct ScanStats {
    uint64_t nBlocks;
    uint64_t nTxs;
    uint64_t nSigs;
    uint64_t nPubKeys;

    uint64_t nHybridPubKeysSpent;    // 0x06/0x07 in a scriptSig
    uint64_t nHybridPubKeysInOutput; // 0x06/0x07 in a bare-pubkey output
    uint64_t nBadSigEncoding;        // fails IsValidSignatureEncoding()
    uint64_t nUncompressedP2PKBad;   // P2PK output whose key is off-curve

    uint64_t nPubKeyDisagree; // OpenSSL and secp256k1 disagree on a pubkey
    uint64_t nSigDisagree;    // ... or on a signature

    ScanStats() { memset(this, 0, sizeof(*this)); }
};

//! Does OpenSSL accept this as a point on secp256k1?
bool OpenSSLAcceptsPubKey(const std::vector<unsigned char>& vch)
{
    CECKey key;
    return key.SetPubKey(&vch[0], vch.size());
}

//! Does libsecp256k1 accept it?
bool Secp256k1AcceptsPubKey(const std::vector<unsigned char>& vch)
{
    return secp256k1_ec_pubkey_verify(&vch[0], vch.size()) == 1;
}

//! Does OpenSSL's ASN.1 parser accept this DER signature?
bool OpenSSLParsesSig(const std::vector<unsigned char>& vch)
{
    if (vch.empty())
        return false;
    ECDSA_SIG* sig = ECDSA_SIG_new();
    if (sig == NULL)
        return false;
    const unsigned char* p = &vch[0];
    bool ok = (d2i_ECDSA_SIG(&sig, &p, vch.size()) != NULL);
    ECDSA_SIG_free(sig);
    return ok;
}

//! Does libsecp256k1's hand-rolled parser accept it?
//
// secp256k1_ecdsa_verify() distinguishes a signature that failed to parse
// (-2) from one that parsed but did not verify (0), so we can compare parsers
// without needing the real sighash or the spent output's scriptPubKey.
bool Secp256k1ParsesSig(const std::vector<unsigned char>& vch, const std::vector<unsigned char>& vchPubKey)
{
    if (vch.empty())
        return false;
    // Anything shorter than a minimal DER header would be read out of bounds
    // by the 2014-era parser; the caller guards this, we mirror it here.
    if (vch.size() < 6)
        return false;
    unsigned char msg[32] = {};
    int ret = secp256k1_ecdsa_verify(msg, sizeof(msg), &vch[0], vch.size(), &vchPubKey[0], vchPubKey.size());
    return ret != -2;
}

bool LooksLikeSig(const std::vector<unsigned char>& vch)
{
    return vch.size() >= 8 && vch.size() <= 73 && vch[0] == 0x30;
}

bool LooksLikePubKey(const std::vector<unsigned char>& vch)
{
    if (vch.size() == 33)
        return vch[0] == 0x02 || vch[0] == 0x03;
    if (vch.size() == 65)
        return vch[0] == 0x04 || vch[0] == 0x06 || vch[0] == 0x07;
    return false;
}

//! Collect every data push in a script.
void CollectPushes(const CScript& script, std::vector<std::vector<unsigned char> >& vRet)
{
    CScript::const_iterator pc = script.begin();
    opcodetype opcode;
    std::vector<unsigned char> vch;
    while (pc < script.end()) {
        if (!script.GetOp(pc, opcode, vch))
            return; // unparsable tail; take what we have
        if (!vch.empty())
            vRet.push_back(vch);
    }
}

//! A reference pubkey to feed the signature-parse probe, so that a pubkey
//! failure cannot be mistaken for a signature-parse failure.
const std::vector<unsigned char>& ProbePubKey()
{
    static std::vector<unsigned char> vch = ParseHex(
        "0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798"
        "483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8");
    return vch;
}

void ScanScript(const CScript& script, bool fIsScriptSig, const uint256& txid, unsigned int nIndex, ScanStats& stats, std::vector<std::string>& vFindings)
{
    std::vector<std::vector<unsigned char> > vPushes;
    CollectPushes(script, vPushes);

    // A P2SH redeem script is itself the last push; look inside it too.
    if (fIsScriptSig && !vPushes.empty()) {
        const std::vector<unsigned char>& vchLast = vPushes.back();
        if (vchLast.size() > 1 && vchLast.size() <= MAX_SCRIPT_ELEMENT_SIZE) {
            CScript redeem(vchLast.begin(), vchLast.end());
            CollectPushes(redeem, vPushes);
        }
    }

    for (unsigned int i = 0; i < vPushes.size(); i++) {
        const std::vector<unsigned char>& vch = vPushes[i];

        if (LooksLikePubKey(vch)) {
            stats.nPubKeys++;
            bool fHybrid = (vch.size() == 65 && (vch[0] == 0x06 || vch[0] == 0x07));
            if (fHybrid) {
                if (fIsScriptSig)
                    stats.nHybridPubKeysSpent++;
                else
                    stats.nHybridPubKeysInOutput++;
                vFindings.push_back(strprintf("HYBRID pubkey %s in %s %s:%u",
                    HexStr(vch), fIsScriptSig ? "scriptSig" : "scriptPubKey", txid.ToString(), nIndex));
            }

            bool fOpenSSL = OpenSSLAcceptsPubKey(vch);
            bool fSecp = Secp256k1AcceptsPubKey(vch);
            if (fOpenSSL != fSecp) {
                stats.nPubKeyDisagree++;
                vFindings.push_back(strprintf("DISAGREE pubkey %s openssl=%d secp256k1=%d in %s:%u",
                    HexStr(vch), (int)fOpenSSL, (int)fSecp, txid.ToString(), nIndex));
            }

            // A bare-pubkey (P2PK) output with an uncompressed key gets stored
            // in the UTXO set compressed to its x coordinate, and is expanded
            // again by CScriptCompressor::Decompress() on every read. If the
            // point is not on the curve that expansion fails.
            if (!fIsScriptSig && vch.size() == 65 && !fSecp) {
                stats.nUncompressedP2PKBad++;
                vFindings.push_back(strprintf("OFF-CURVE uncompressed key in output %s:%u: %s",
                    txid.ToString(), nIndex, HexStr(vch)));
            }
        }

        if (fIsScriptSig && LooksLikeSig(vch)) {
            stats.nSigs++;

            // The signature carries a trailing hashtype byte inside the push.
            std::vector<unsigned char> vchDer(vch.begin(), vch.end() - 1);

            if (!IsValidSignatureEncoding(vch)) {
                stats.nBadSigEncoding++;
                vFindings.push_back(strprintf("BAD-DER sig %s in %s:%u",
                    HexStr(vch), txid.ToString(), nIndex));
            }

            bool fOpenSSL = OpenSSLParsesSig(vchDer);
            bool fSecp = Secp256k1ParsesSig(vchDer, ProbePubKey());
            if (fOpenSSL != fSecp) {
                stats.nSigDisagree++;
                vFindings.push_back(strprintf("DISAGREE sig %s openssl=%d secp256k1=%d in %s:%u",
                    HexStr(vchDer), (int)fOpenSSL, (int)fSecp, txid.ToString(), nIndex));
            }
        }
    }
}

//! Stream every block out of one blk000??.dat file.
//
// Same on-disk framing as CBlockUndo/CBlock writing in main.cpp: a 4-byte
// network magic, a 4-byte size, then the serialized block.
bool ScanBlockFile(const boost::filesystem::path& path, ScanStats& stats, std::vector<std::string>& vFindings)
{
    FILE* file = fopen(path.string().c_str(), "rb");
    if (file == NULL)
        return false;

    const MessageStartChars& start = Params().MessageStart();

    while (true) {
        unsigned char hdr[8];
        if (fread(hdr, 1, sizeof(hdr), file) != sizeof(hdr))
            break; // end of file (or trailing zero padding)

        if (memcmp(hdr, start, 4) != 0)
            break; // padding / not our network: stop on this file

        unsigned int nSize = hdr[4] | (hdr[5] << 8) | (hdr[6] << 16) | ((unsigned int)hdr[7] << 24);
        if (nSize < 80 || nSize > MAX_BLOCK_SIZE + 8)
            break;

        std::vector<unsigned char> vchBlock(nSize);
        if (fread(&vchBlock[0], 1, nSize, file) != nSize)
            break;

        CDataStream ss(vchBlock, SER_DISK, CLIENT_VERSION);
        CBlock block;
        try {
            ss >> block;
        } catch (const std::exception&) {
            break;
        }

        stats.nBlocks++;
        for (unsigned int i = 0; i < block.vtx.size(); i++) {
            const CTransaction& tx = block.vtx[i];
            stats.nTxs++;
            uint256 txid = tx.GetHash();

            if (!tx.IsCoinBase()) {
                for (unsigned int j = 0; j < tx.vin.size(); j++)
                    ScanScript(tx.vin[j].scriptSig, true, txid, j, stats, vFindings);
            }
            for (unsigned int j = 0; j < tx.vout.size(); j++)
                ScanScript(tx.vout[j].scriptPubKey, false, txid, j, stats, vFindings);
        }
    }

    fclose(file);
    return true;
}
} // anonymous namespace

BOOST_AUTO_TEST_CASE(scan_chain_for_verifier_divergence)
{
    const char* pszDir = getenv("BBK_BLOCKS_DIR");
    if (pszDir == NULL) {
        BOOST_TEST_MESSAGE("BBK_BLOCKS_DIR not set, skipping chain scan");
        return;
    }

    SelectParams(CBaseChainParams::MAIN);
    ECC_Start();

    boost::filesystem::path dir(pszDir);
    BOOST_REQUIRE_MESSAGE(boost::filesystem::is_directory(dir), "BBK_BLOCKS_DIR is not a directory: " << pszDir);

    std::vector<boost::filesystem::path> vFiles;
    for (boost::filesystem::directory_iterator it(dir); it != boost::filesystem::directory_iterator(); ++it) {
        std::string name = it->path().filename().string();
        if (name.compare(0, 3, "blk") == 0 && name.size() > 4 && name.compare(name.size() - 4, 4, ".dat") == 0)
            vFiles.push_back(it->path());
    }
    std::sort(vFiles.begin(), vFiles.end());
    BOOST_REQUIRE_MESSAGE(!vFiles.empty(), "no blk*.dat files found in " << pszDir);

    ScanStats stats;
    std::vector<std::string> vFindings;
    for (unsigned int i = 0; i < vFiles.size(); i++) {
        BOOST_TEST_MESSAGE("scanning " << vFiles[i].filename().string());
        ScanBlockFile(vFiles[i], stats, vFindings);
    }

    BOOST_TEST_MESSAGE("blocks="            << stats.nBlocks
                    << " txs="              << stats.nTxs
                    << " signatures="       << stats.nSigs
                    << " pubkeys="          << stats.nPubKeys);
    BOOST_TEST_MESSAGE("hybrid_spent="      << stats.nHybridPubKeysSpent
                    << " hybrid_in_output=" << stats.nHybridPubKeysInOutput
                    << " bad_der="          << stats.nBadSigEncoding
                    << " offcurve_p2pk="    << stats.nUncompressedP2PKBad);
    BOOST_TEST_MESSAGE("pubkey_disagreements=" << stats.nPubKeyDisagree
                    << " sig_disagreements="   << stats.nSigDisagree);

    for (unsigned int i = 0; i < vFindings.size() && i < 200; i++)
        BOOST_TEST_MESSAGE(vFindings[i]);

    // These are the migration blockers.
    BOOST_CHECK_EQUAL(stats.nPubKeyDisagree, 0);
    BOOST_CHECK_EQUAL(stats.nSigDisagree, 0);
    BOOST_CHECK_EQUAL(stats.nHybridPubKeysSpent, 0);
    BOOST_CHECK_EQUAL(stats.nUncompressedP2PKBad, 0);

    ECC_Stop();
}

BOOST_AUTO_TEST_SUITE_END()
