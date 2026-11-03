// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "txmempool.h"
#include "util.h"

#include <boost/test/unit_test.hpp>
#include <list>

BOOST_AUTO_TEST_SUITE(mempool_tests)

BOOST_AUTO_TEST_CASE(MempoolRemoveTest)
{
    // Test CTxMemPool::remove functionality

    // Parent transaction with three children,
    // and three grand-children:
    CMutableTransaction txParent;
    txParent.vin.resize(1);
    txParent.vin[0].scriptSig = CScript() << OP_11;
    txParent.vout.resize(3);
    for (int i = 0; i < 3; i++)
    {
        txParent.vout[i].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txParent.vout[i].nValue = 33000LL;
    }
    CMutableTransaction txChild[3];
    for (int i = 0; i < 3; i++)
    {
        txChild[i].vin.resize(1);
        txChild[i].vin[0].scriptSig = CScript() << OP_11;
        txChild[i].vin[0].prevout.hash = txParent.GetHash();
        txChild[i].vin[0].prevout.n = i;
        txChild[i].vout.resize(1);
        txChild[i].vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txChild[i].vout[0].nValue = 11000LL;
    }
    CMutableTransaction txGrandChild[3];
    for (int i = 0; i < 3; i++)
    {
        txGrandChild[i].vin.resize(1);
        txGrandChild[i].vin[0].scriptSig = CScript() << OP_11;
        txGrandChild[i].vin[0].prevout.hash = txChild[i].GetHash();
        txGrandChild[i].vin[0].prevout.n = 0;
        txGrandChild[i].vout.resize(1);
        txGrandChild[i].vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txGrandChild[i].vout[0].nValue = 11000LL;
    }


    CTxMemPool testPool(CFeeRate(0));
    std::list<CTransaction> removed;

    // Nothing in pool, remove should do nothing:
    testPool.remove(txParent, removed, true);
    BOOST_CHECK_EQUAL(removed.size(), 0);

    // Just the parent:
    testPool.addUnchecked(txParent.GetHash(), CTxMemPoolEntry(txParent, 0, 0, 0.0, 1));
    testPool.remove(txParent, removed, true);
    BOOST_CHECK_EQUAL(removed.size(), 1);
    removed.clear();
    
    // Parent, children, grandchildren:
    testPool.addUnchecked(txParent.GetHash(), CTxMemPoolEntry(txParent, 0, 0, 0.0, 1));
    for (int i = 0; i < 3; i++)
    {
        testPool.addUnchecked(txChild[i].GetHash(), CTxMemPoolEntry(txChild[i], 0, 0, 0.0, 1));
        testPool.addUnchecked(txGrandChild[i].GetHash(), CTxMemPoolEntry(txGrandChild[i], 0, 0, 0.0, 1));
    }
    // Remove Child[0], GrandChild[0] should be removed:
    testPool.remove(txChild[0], removed, true);
    BOOST_CHECK_EQUAL(removed.size(), 2);
    removed.clear();
    // ... make sure grandchild and child are gone:
    testPool.remove(txGrandChild[0], removed, true);
    BOOST_CHECK_EQUAL(removed.size(), 0);
    testPool.remove(txChild[0], removed, true);
    BOOST_CHECK_EQUAL(removed.size(), 0);
    // Remove parent, all children/grandchildren should go:
    testPool.remove(txParent, removed, true);
    BOOST_CHECK_EQUAL(removed.size(), 5);
    BOOST_CHECK_EQUAL(testPool.size(), 0);
    removed.clear();

    // Add children and grandchildren, but NOT the parent (simulate the parent being in a block)
    for (int i = 0; i < 3; i++)
    {
        testPool.addUnchecked(txChild[i].GetHash(), CTxMemPoolEntry(txChild[i], 0, 0, 0.0, 1));
        testPool.addUnchecked(txGrandChild[i].GetHash(), CTxMemPoolEntry(txGrandChild[i], 0, 0, 0.0, 1));
    }
    // Now remove the parent, as might happen if a block-re-org occurs but the parent cannot be
    // put into the mempool (maybe because it is non-standard):
    testPool.remove(txParent, removed, true);
    BOOST_CHECK_EQUAL(removed.size(), 6);
    BOOST_CHECK_EQUAL(testPool.size(), 0);
    removed.clear();
}

static CMutableTransaction MakeChainTx(const uint256& prevHash, uint32_t prevN, int nOutputs)
{
    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vin[0].scriptSig = CScript() << OP_11;
    tx.vin[0].prevout.hash = prevHash;
    tx.vin[0].prevout.n = prevN;
    tx.vout.resize(nOutputs);
    for (int i = 0; i < nOutputs; i++) {
        tx.vout[i].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        tx.vout[i].nValue = 11000LL;
    }
    return tx;
}

BOOST_AUTO_TEST_CASE(MempoolExpireTest)
{
    // Old parent with a young child, plus an unrelated young transaction
    CMutableTransaction txOld = MakeChainTx(uint256(1), 0, 1);
    CMutableTransaction txChild = MakeChainTx(txOld.GetHash(), 0, 1);
    CMutableTransaction txYoung = MakeChainTx(uint256(2), 0, 1);

    CTxMemPool testPool(CFeeRate(0));
    testPool.addUnchecked(txOld.GetHash(), CTxMemPoolEntry(txOld, 0, 1000, 0.0, 1));
    testPool.addUnchecked(txChild.GetHash(), CTxMemPoolEntry(txChild, 0, 5000, 0.0, 1));
    testPool.addUnchecked(txYoung.GetHash(), CTxMemPoolEntry(txYoung, 0, 5000, 0.0, 1));

    // Nothing entered before time 1000
    BOOST_CHECK_EQUAL(testPool.Expire(1000), 0);
    BOOST_CHECK_EQUAL(testPool.size(), 3);

    // The old parent expires and takes its child with it
    BOOST_CHECK_EQUAL(testPool.Expire(2000), 2);
    BOOST_CHECK_EQUAL(testPool.size(), 1);
    BOOST_CHECK(testPool.exists(txYoung.GetHash()));
    BOOST_CHECK_EQUAL(testPool.GetTotalTxSize(), ::GetSerializeSize(txYoung, SER_NETWORK, PROTOCOL_VERSION));

    BOOST_CHECK_EQUAL(testPool.Expire(6000), 1);
    BOOST_CHECK_EQUAL(testPool.size(), 0);
    BOOST_CHECK_EQUAL(testPool.GetTotalTxSize(), 0);
}

BOOST_AUTO_TEST_CASE(MempoolFreeTxSizeTest)
{
    // Relay fee of 1000 per kB: a transaction of about 100 bytes needs ~100
    CTxMemPool testPool(CFeeRate(1000));
    CMutableTransaction txFree = MakeChainTx(uint256(1), 0, 1);
    CMutableTransaction txPaying = MakeChainTx(uint256(2), 0, 1);
    unsigned int nFreeSize = ::GetSerializeSize(txFree, SER_NETWORK, PROTOCOL_VERSION);
    unsigned int nPayingSize = ::GetSerializeSize(txPaying, SER_NETWORK, PROTOCOL_VERSION);

    BOOST_CHECK(testPool.IsFreeTx(0, nFreeSize));
    BOOST_CHECK(!testPool.IsFreeTx(1000, nPayingSize));

    testPool.addUnchecked(txFree.GetHash(), CTxMemPoolEntry(txFree, 0, 0, 0.0, 1));
    testPool.addUnchecked(txPaying.GetHash(), CTxMemPoolEntry(txPaying, 1000, 0, 0.0, 1));
    BOOST_CHECK_EQUAL(testPool.GetTotalTxSize(), nFreeSize + nPayingSize);
    BOOST_CHECK_EQUAL(testPool.GetTotalFreeTxSize(), nFreeSize);

    std::list<CTransaction> removed;
    testPool.remove(txPaying, removed, false);
    BOOST_CHECK_EQUAL(testPool.GetTotalFreeTxSize(), nFreeSize);
    testPool.remove(txFree, removed, false);
    BOOST_CHECK_EQUAL(testPool.GetTotalFreeTxSize(), 0);
    BOOST_CHECK_EQUAL(testPool.GetTotalTxSize(), 0);

    testPool.addUnchecked(txFree.GetHash(), CTxMemPoolEntry(txFree, 0, 0, 0.0, 1));
    testPool.clear();
    BOOST_CHECK_EQUAL(testPool.GetTotalFreeTxSize(), 0);
}

BOOST_AUTO_TEST_CASE(AllowFreeThresholdTest)
{
    // Priority is sum(value * confirmations) / modified size. For a 250 byte
    // (modified size) transaction the threshold is 5760 BBK-blocks.
    const double nSize = 250;
    BOOST_CHECK(!AllowFree(AllowFreeThreshold()));
    BOOST_CHECK(!AllowFree(1.0 * COIN * 576 / nSize));       // the old threshold is no longer enough
    BOOST_CHECK(!AllowFree(100.0 * COIN * 57 / nSize));      // 100 BBK for 57 blocks
    BOOST_CHECK(AllowFree(100.0 * COIN * 58 / nSize));       // 100 BBK for 58 blocks (~1 hour)
    BOOST_CHECK(!AllowFree(1.0 * COIN * 1440 / nSize));      // 1 BBK for one day
    BOOST_CHECK(!AllowFree(1.0 * COIN * 4 * 1440 / nSize));  // 1 BBK for exactly four days
    BOOST_CHECK(AllowFree(1.0 * COIN * (4 * 1440 + 1) / nSize)); // ... and one block more
}

BOOST_AUTO_TEST_SUITE_END()
