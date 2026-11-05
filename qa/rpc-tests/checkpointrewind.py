#!/usr/bin/env python2
# Copyright (c) 2026 The BitBlocks developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test the startup rewind to a checkpoint and -resync.
#
# A node that followed its own branch for more than the reorganization depth
# limit (100 blocks) can't return to the main chain by itself. Given a
# checkpoint above the split (-testcheckpoint, regtest only), it must undo its
# branch on startup and then sync the main chain. A branch deeper than
# -maxcheckpointrewind must stop bitblocksd and ask for -resync instead.
#
# Regtest mining needs the temporary nMaxMoneyOut patch (see the regtest notes).
#

from test_framework import BitcoinTestFramework
from util import *
import os
import subprocess

SPLIT = 10          # blocks shared by every node
MAIN_BLOCKS = 160   # node0's chain past the split
FORK_BLOCKS = 130   # each forked node's own branch past the split: more than 100
# Every node is 127.0.0.1: without this, the deep-fork blocks they refuse from
# each other would get 127.0.0.1 banned on all of them.
# -checkblockindex=0: on regtest the block index check asserts as soon as a node
# mines after a restart (an index entry at height 1 with no status), on dev too.
BASE_ARGS = ["-whitelist=127.0.0.1", "-checkblockindex=0"]

def wait_until(predicate, timeout=60):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if predicate():
            return True
        time.sleep(0.5)
    return False

class CheckpointRewindTest(BitcoinTestFramework):

    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 4)

    def setup_network(self):
        self.nodes = start_nodes(4, self.options.tmpdir, [BASE_ARGS] * 4)
        self.is_network_split = False

    def restart(self, i, extra_args=None):
        stop_node(self.nodes[i], i)
        self.nodes[i] = start_node(i, self.options.tmpdir, BASE_ARGS + (extra_args or []))
        assert_equal(self.nodes[i].getconnectioncount(), 0)

    def connect_to_main(self, i):
        connect_nodes(self.nodes[i], 0)

    def run_test(self):
        main = self.nodes[0]

        print("Mine %d shared blocks and give them to every node" % SPLIT)
        main.setgenerate(True, SPLIT)
        for i in range(1, 4):
            self.connect_to_main(i)
        assert(wait_until(lambda: all(n.getblockcount() == SPLIT for n in self.nodes)))
        for i in range(4):
            self.restart(i)
        main = self.nodes[0]

        print("Split: node0 mines %d blocks, nodes 1-3 mine %d each on their own" % (MAIN_BLOCKS, FORK_BLOCKS))
        main.setgenerate(True, MAIN_BLOCKS)
        for i in range(1, 4):
            self.nodes[i].setgenerate(True, FORK_BLOCKS)
            assert_equal(self.nodes[i].getblockcount(), SPLIT + FORK_BLOCKS)
        main_tip = main.getbestblockhash()
        main_height = main.getblockcount()
        fork_balance = self.nodes[1].getbalance()
        assert(fork_balance > 0)

        print("A forked node can't reorganize back: the fork is deeper than 100 blocks")
        self.connect_to_main(1)
        time.sleep(10)
        assert_equal(self.nodes[1].getblockcount(), SPLIT + FORK_BLOCKS)

        print("Checkpoint at split+1: node1 rewinds on startup and syncs the main chain")
        checkpoint = "-testcheckpoint=%d:%s" % (SPLIT + 1, main.getblockhash(SPLIT + 1))
        self.restart(1, [checkpoint])
        assert_equal(self.nodes[1].getblockcount(), SPLIT)
        self.connect_to_main(1)
        assert(wait_until(lambda: self.nodes[1].getbestblockhash() == main_tip))
        print("  node1 wallet: %s before, %s after" % (fork_balance, self.nodes[1].getbalance()))
        assert_equal(self.nodes[1].getbalance(), 0)  # its fork's coinbases are gone

        print("Restarting with the same checkpoint changes nothing, on node1 and node0")
        self.restart(1, [checkpoint])
        assert_equal(self.nodes[1].getbestblockhash(), main_tip)
        self.restart(0, [checkpoint])
        main = self.nodes[0]
        assert_equal(main.getbestblockhash(), main_tip)

        print("Checkpoint 20 blocks past the split: node2 rewinds to it and reorganizes the rest")
        late = "-testcheckpoint=%d:%s" % (SPLIT + 21, main.getblockhash(SPLIT + 21))
        self.restart(2, [late])
        assert_equal(self.nodes[2].getblockcount(), SPLIT + 20)
        self.connect_to_main(2)
        assert(wait_until(lambda: self.nodes[2].getbestblockhash() == main_tip))

        print("Fork deeper than -maxcheckpointrewind: bitblocksd stops and asks for -resync")
        stop_node(self.nodes[3], 3)
        datadir = os.path.join(self.options.tmpdir, "node3")
        args = [os.getenv("BITCOIND", "bitblocksd"), "-datadir=" + datadir, "-keypool=1", "-discover=0",
                checkpoint, "-maxcheckpointrewind=50"] + BASE_ARGS
        devnull = open(os.devnull, "w")
        process = subprocess.Popen(args, stdout=devnull, stderr=devnull)
        assert(wait_until(lambda: process.poll() is not None))
        devnull.close()
        assert(process.returncode != 0)
        with open(log_filename(self.options.tmpdir, 3, "debug.log")) as log:
            assert("-resync" in log.read())

        print("-resync deletes node3's chain and syncs the main one from scratch")
        self.nodes[3] = start_node(3, self.options.tmpdir, BASE_ARGS + [checkpoint, "-maxcheckpointrewind=50", "-resync"])
        assert_equal(self.nodes[3].getblockcount(), 0)
        self.connect_to_main(3)
        assert(wait_until(lambda: self.nodes[3].getbestblockhash() == main_tip))
        assert_equal(self.nodes[3].getblockcount(), main_height)

if __name__ == '__main__':
    CheckpointRewindTest().main()
