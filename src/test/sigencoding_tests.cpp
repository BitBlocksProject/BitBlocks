// Copyright (c) 2026 The BitBlocks developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//
// Signature encoding vectors, and regressions for malformed signatures
// reaching the verifier.
//
// test/data/sig_canonical.json and sig_noncanonical.json have been shipped in
// JSON_TEST_FILES since the fork was created, but no test ever referenced
// them. They are exactly the vectors that matter when the consensus verifier
// changes, so wire them up.
//

#include "data/sig_canonical.json.h"
#include "data/sig_noncanonical.json.h"

#include "key.h"
#include "pubkey.h"
#include "random.h"
#include "script/interpreter.h"
#include "script/script_error.h"
#include "uint256.h"
#include "utilstrencodings.h"

#include <string>
#include <vector>

#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_writer_template.h"

#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>

using namespace json_spirit;

extern Array read_json(const std::string& jsondata);

BOOST_AUTO_TEST_SUITE(sigencoding_tests)

BOOST_AUTO_TEST_CASE(canonical_vectors)
{
    Array tests = read_json(std::string(json_tests::sig_canonical,
                                        json_tests::sig_canonical + sizeof(json_tests::sig_canonical)));

    unsigned int nChecked = 0;
    BOOST_FOREACH (Value& tv, tests) {
        std::string str = tv.get_str();
        if (!IsHex(str))
            continue;
        std::vector<unsigned char> sig = ParseHex(str);
        BOOST_CHECK_MESSAGE(IsValidSignatureEncoding(sig), "not accepted as canonical DER: " << str);
        nChecked++;
    }
    BOOST_CHECK(nChecked > 0);
}

BOOST_AUTO_TEST_CASE(noncanonical_vectors)
{
    Array tests = read_json(std::string(json_tests::sig_noncanonical,
                                        json_tests::sig_noncanonical + sizeof(json_tests::sig_noncanonical)));

    unsigned int nChecked = 0;
    BOOST_FOREACH (Value& tv, tests) {
        std::string str = tv.get_str();
        // The file interleaves human-readable labels with the vectors.
        if (!IsHex(str))
            continue;
        std::vector<unsigned char> sig = ParseHex(str);
        // The file mixes two kinds of vector: signatures whose DER shape is
        // wrong, and signatures whose hashtype byte is. Only STRICTENC covers
        // both -- IsValidSignatureEncoding() deliberately ignores the
        // hashtype, which IsDefinedHashtypeSignature() checks instead.
        ScriptError err = SCRIPT_ERR_OK;
        BOOST_CHECK_MESSAGE(!CheckSignatureEncoding(sig, SCRIPT_VERIFY_DERSIG | SCRIPT_VERIFY_STRICTENC, &err),
                            "wrongly accepted: " << str);
        nChecked++;
    }
    BOOST_CHECK(nChecked > 0);
}

//
// The 2014-era libsecp256k1 DER parser indexes sig[0], sig[3] and sig[lenr+5]
// before it consults the buffer length, and secp256k1_ecdsa_verify() does not
// guard siglen either. Nothing in the consensus flags stops a short signature
// from reaching CPubKey::Verify(): CheckSignatureEncoding() only runs
// IsValidSignatureEncoding() when DERSIG, LOW_S or STRICTENC is set.
//
// So: truncated and empty signatures must be rejected, and must not read out
// of bounds doing it. Run this under valgrind or an ASan build to check the
// second half of that claim.
//
BOOST_AUTO_TEST_CASE(short_signatures_are_rejected)
{
    CKey key;
    key.MakeNewKey(true);
    CPubKey pubkey = key.GetPubKey();
    BOOST_REQUIRE(pubkey.IsFullyValid());

    uint256 hash = GetRandHash();

    // A real signature, then every truncation of it.
    std::vector<unsigned char> vchSig;
    BOOST_REQUIRE(key.Sign(hash, vchSig));
    BOOST_CHECK(pubkey.Verify(hash, vchSig));

    for (unsigned int nLen = 0; nLen < vchSig.size(); nLen++) {
        std::vector<unsigned char> vchTruncated(vchSig.begin(), vchSig.begin() + nLen);
        BOOST_CHECK_MESSAGE(!pubkey.Verify(hash, vchTruncated),
                            "truncated signature of length " << nLen << " was accepted");
    }

    // Garbage of every short length, including the empty vector.
    for (unsigned int nLen = 0; nLen <= 8; nLen++) {
        std::vector<unsigned char> vchGarbage(nLen, 0x30);
        BOOST_CHECK_MESSAGE(!pubkey.Verify(hash, vchGarbage),
                            "garbage signature of length " << nLen << " was accepted");
    }
}

//
// CScriptCompressor::Decompress() expands a bare-pubkey output out of the UTXO
// set on every read. Nothing validates that such an output holds a point on
// the curve when it is created, so Decompress() has to fail gracefully rather
// than abort.
//
BOOST_AUTO_TEST_CASE(offcurve_pubkey_does_not_abort)
{
    // x = 7 is not on secp256k1: 7^3 + 7 = 350 is not a quadratic residue
    // modulo p, so no y exists and the point cannot be decompressed.
    std::vector<unsigned char> vch = ParseHex("020000000000000000000000000000000000000000000000000000000000000007");
    BOOST_REQUIRE_EQUAL(vch.size(), 33U);

    CPubKey pubkey(vch.begin(), vch.end());
    BOOST_CHECK(!pubkey.IsFullyValid());
    BOOST_CHECK(!pubkey.Decompress()); // must return, not assert
}

//
// A sign/verify round trip, which is the one check that exercises whichever
// verifier this build actually uses.
//
BOOST_AUTO_TEST_CASE(sign_verify_roundtrip)
{
    for (int i = 0; i < 10; i++) {
        CKey key;
        key.MakeNewKey(i % 2 == 0);
        CPubKey pubkey = key.GetPubKey();
        BOOST_REQUIRE(pubkey.IsFullyValid());

        uint256 hash = GetRandHash();
        std::vector<unsigned char> vchSig;
        BOOST_REQUIRE(key.Sign(hash, vchSig));

        BOOST_CHECK(pubkey.Verify(hash, vchSig));

        // A flipped bit anywhere in the signature must not verify.
        std::vector<unsigned char> vchBad = vchSig;
        vchBad[vchBad.size() - 1] ^= 0x01;
        BOOST_CHECK(!pubkey.Verify(hash, vchBad));

        // Nor should the right signature under the wrong hash.
        BOOST_CHECK(!pubkey.Verify(GetRandHash(), vchSig));
    }
}

BOOST_AUTO_TEST_SUITE_END()
