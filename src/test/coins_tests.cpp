// Copyright (c) 2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "clientversion.h"
#include "coins.h"
#include "main.h"
#include "random.h"
#include "streams.h"
#include "uint256.h"
#include "undo.h"

#include <vector>
#include <map>

#include <boost/test/unit_test.hpp>

namespace
{
class CCoinsViewTest : public CCoinsView
{
    uint256 hashBestBlock_;
    std::map<uint256, CCoins> map_;

public:
    bool GetCoins(const uint256& txid, CCoins& coins) const
    {
        std::map<uint256, CCoins>::const_iterator it = map_.find(txid);
        if (it == map_.end()) {
            return false;
        }
        coins = it->second;
        if (coins.IsPruned() && insecure_rand() % 2 == 0) {
            // Randomly return false in case of an empty entry.
            return false;
        }
        return true;
    }

    bool HaveCoins(const uint256& txid) const
    {
        CCoins coins;
        return GetCoins(txid, coins);
    }

    uint256 GetBestBlock() const { return hashBestBlock_; }

    bool BatchWrite(CCoinsMap& mapCoins, const uint256& hashBlock)
    {
        for (CCoinsMap::iterator it = mapCoins.begin(); it != mapCoins.end(); ) {
            map_[it->first] = it->second.coins;
            if (it->second.coins.IsPruned() && insecure_rand() % 3 == 0) {
                // Randomly delete empty entries on write.
                map_.erase(it->first);
            }
            mapCoins.erase(it++);
        }
        mapCoins.clear();
        hashBestBlock_ = hashBlock;
        return true;
    }

    bool GetStats(CCoinsStats& stats) const { return false; }
};
}

BOOST_AUTO_TEST_SUITE(coins_tests)

static const unsigned int NUM_SIMULATION_ITERATIONS = 40000;

// This is a large randomized insert/remove simulation test on a variable-size
// stack of caches on top of CCoinsViewTest.
//
// It will randomly create/update/delete CCoins entries to a tip of caches, with
// txids picked from a limited list of random 256-bit hashes. Occasionally, a
// new tip is added to the stack of caches, or the tip is flushed and removed.
//
// During the process, booleans are kept to make sure that the randomized
// operation hits all branches.
BOOST_AUTO_TEST_CASE(coins_cache_simulation_test)
{
    // Various coverage trackers.
    bool removed_all_caches = false;
    bool reached_4_caches = false;
    bool added_an_entry = false;
    bool removed_an_entry = false;
    bool updated_an_entry = false;
    bool found_an_entry = false;
    bool missed_an_entry = false;

    // A simple map to track what we expect the cache stack to represent.
    std::map<uint256, CCoins> result;

    // The cache stack.
    CCoinsViewTest base; // A CCoinsViewTest at the bottom.
    std::vector<CCoinsViewCache*> stack; // A stack of CCoinsViewCaches on top.
    stack.push_back(new CCoinsViewCache(&base)); // Start with one cache.

    // Use a limited set of random transaction ids, so we do test overwriting entries.
    std::vector<uint256> txids;
    txids.resize(NUM_SIMULATION_ITERATIONS / 8);
    for (unsigned int i = 0; i < txids.size(); i++) {
        txids[i] = GetRandHash();
    }

    for (unsigned int i = 0; i < NUM_SIMULATION_ITERATIONS; i++) {
        // Do a random modification.
        {
            uint256 txid = txids[insecure_rand() % txids.size()]; // txid we're going to modify in this iteration.
            CCoins& coins = result[txid];
            CCoinsModifier entry = stack.back()->ModifyCoins(txid);
            BOOST_CHECK(coins == *entry);
            if (insecure_rand() % 5 == 0 || coins.IsPruned()) {
                if (coins.IsPruned()) {
                    added_an_entry = true;
                } else {
                    updated_an_entry = true;
                }
                coins.nVersion = insecure_rand();
                coins.vout.resize(1);
                coins.vout[0].nValue = insecure_rand();
                *entry = coins;
            } else {
                coins.Clear();
                entry->Clear();
                removed_an_entry = true;
            }
        }

        // Once every 1000 iterations and at the end, verify the full cache.
        if (insecure_rand() % 1000 == 1 || i == NUM_SIMULATION_ITERATIONS - 1) {
            for (std::map<uint256, CCoins>::iterator it = result.begin(); it != result.end(); it++) {
                const CCoins* coins = stack.back()->AccessCoins(it->first);
                if (coins) {
                    BOOST_CHECK(*coins == it->second);
                    found_an_entry = true;
                } else {
                    BOOST_CHECK(it->second.IsPruned());
                    missed_an_entry = true;
                }
            }
        }

        if (insecure_rand() % 100 == 0) {
            // Every 100 iterations, change the cache stack.
            if (stack.size() > 0 && insecure_rand() % 2 == 0) {
                stack.back()->Flush();
                delete stack.back();
                stack.pop_back();
            }
            if (stack.size() == 0 || (stack.size() < 4 && insecure_rand() % 2)) {
                CCoinsView* tip = &base;
                if (stack.size() > 0) {
                    tip = stack.back();
                } else {
                    removed_all_caches = true;
                }
                stack.push_back(new CCoinsViewCache(tip));
                if (stack.size() == 4) {
                    reached_4_caches = true;
                }
            }
        }
    }

    // Clean up the stack.
    while (stack.size() > 0) {
        delete stack.back();
        stack.pop_back();
    }

    // Verify coverage.
    BOOST_CHECK(removed_all_caches);
    BOOST_CHECK(reached_4_caches);
    BOOST_CHECK(added_an_entry);
    BOOST_CHECK(removed_an_entry);
    BOOST_CHECK(updated_an_entry);
    BOOST_CHECK(found_an_entry);
    BOOST_CHECK(missed_an_entry);
}

// Spend every output of tx, then undo the spends the way DisconnectBlock does
// (reverse order, through ApplyTxInUndo) and check the coins come back intact,
// including the coinbase/coinstake flags that only travel in the last undo.
static void CheckSpendAndUndoRoundTrip(const CTransaction& tx, int nHeight)
{
    CCoinsView base;
    CCoinsViewCache cache(&base);
    const uint256 txid = tx.GetHash();
    const CCoins coinsOrig(tx, nHeight);
    BOOST_CHECK_EQUAL(coinsOrig.IsCoinStake(), tx.IsCoinStake());
    BOOST_CHECK_EQUAL(coinsOrig.IsCoinBase(), tx.IsCoinBase());
    {
        CCoinsModifier coins = cache.ModifyCoins(txid);
        *coins = coinsOrig;
    }

    std::vector<std::pair<COutPoint, CTxInUndo> > vUndo;
    for (unsigned int n = 0; n < tx.vout.size(); n++) {
        const COutPoint out(txid, n);
        CTxInUndo undo;
        {
            CCoinsModifier coins = cache.ModifyCoins(txid);
            BOOST_REQUIRE(coins->Spend(out, undo));
        }
        // Round-trip through the on-disk format, as the undo would be read back.
        CDataStream ss(SER_DISK, CLIENT_VERSION);
        ss << undo;
        CTxInUndo undoRead;
        ss >> undoRead;
        vUndo.push_back(std::make_pair(out, undoRead));
    }

    const CCoins* spent = cache.AccessCoins(txid);
    BOOST_CHECK(spent == NULL || spent->IsPruned());
    const CTxInUndo& last = vUndo.back().second;
    BOOST_CHECK_EQUAL(last.nHeight, (unsigned int)nHeight);
    BOOST_CHECK_EQUAL(last.fCoinBase, tx.IsCoinBase());
    BOOST_CHECK_EQUAL(last.fCoinStake, tx.IsCoinStake());

    for (std::vector<std::pair<COutPoint, CTxInUndo> >::reverse_iterator it = vUndo.rbegin(); it != vUndo.rend(); ++it)
        BOOST_CHECK(ApplyTxInUndo(it->second, cache, it->first));

    const CCoins* restored = cache.AccessCoins(txid);
    BOOST_REQUIRE(restored != NULL);
    BOOST_CHECK_EQUAL(restored->IsCoinStake(), tx.IsCoinStake());
    BOOST_CHECK_EQUAL(restored->IsCoinBase(), tx.IsCoinBase());
    BOOST_CHECK_EQUAL(restored->nHeight, nHeight);
    BOOST_CHECK_EQUAL(restored->nVersion, tx.nVersion);
    BOOST_CHECK(*restored == coinsOrig);
}

BOOST_AUTO_TEST_CASE(coinstake_undo_restores_flag)
{
    // Coinstake: real prevout, empty marker output, then the stake outputs.
    CMutableTransaction stake;
    stake.vin.resize(1);
    stake.vin[0].prevout = COutPoint(GetRandHash(), 1);
    stake.vout.resize(3);
    stake.vout[0].SetEmpty();
    stake.vout[1] = CTxOut(600 * COIN, CScript() << OP_TRUE);
    stake.vout[2] = CTxOut(605 * COIN, CScript() << OP_TRUE);
    BOOST_REQUIRE(CTransaction(stake).IsCoinStake());
    CheckSpendAndUndoRoundTrip(stake, 1000);

    // Coinbase: same path, must keep working.
    CMutableTransaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    coinbase.vin[0].scriptSig = CScript() << 1000 << OP_0;
    coinbase.vout.resize(1);
    coinbase.vout[0] = CTxOut(10 * COIN, CScript() << OP_TRUE);
    BOOST_REQUIRE(CTransaction(coinbase).IsCoinBase());
    CheckSpendAndUndoRoundTrip(coinbase, 1000);
}

BOOST_AUTO_TEST_SUITE_END()
