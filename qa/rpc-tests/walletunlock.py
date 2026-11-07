#!/usr/bin/env python2
# Copyright (c) 2026 The BitBlocks developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test that walletpassphrase always checks the passphrase.
#
# A wallet unlocked for staking only must not become fully unlocked with a
# wrong passphrase, and a wrong passphrase must leave an unlocked wallet as it
# was.
#

from test_framework import BitcoinTestFramework
from bitcoinrpc.authproxy import JSONRPCException
from util import *

PASSPHRASE = "test"
WRONG = "wrong"

RPC_WALLET_UNLOCK_NEEDED = -13
RPC_WALLET_PASSPHRASE_INCORRECT = -14
RPC_WALLET_ALREADY_UNLOCKED = -17

def assert_rpc_error(code, fun, *args):
    try:
        fun(*args)
    except JSONRPCException as e:
        assert_equal(e.error["code"], code)
        return
    raise AssertionError("RPC call succeeded, expected error %d" % code)

class WalletUnlockTest(BitcoinTestFramework):

    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 1)

    def setup_network(self):
        self.nodes = start_nodes(1, self.options.tmpdir)
        self.is_network_split = False

    def run_test(self):
        addr = self.nodes[0].getnewaddress()

        print("Encrypt the wallet and restart")
        self.nodes[0].encryptwallet(PASSPHRASE)
        bitcoind_processes[0].wait()
        self.nodes[0] = start_node(0, self.options.tmpdir)
        node = self.nodes[0]
        assert_rpc_error(RPC_WALLET_UNLOCK_NEEDED, node.dumpprivkey, addr)

        print("Unlocked for staking only: private keys stay out of reach")
        node.walletpassphrase(PASSPHRASE, 600, True)
        assert_rpc_error(RPC_WALLET_UNLOCK_NEEDED, node.dumpprivkey, addr)

        print("A wrong passphrase can't turn it into a full unlock")
        assert_rpc_error(RPC_WALLET_PASSPHRASE_INCORRECT, node.walletpassphrase, WRONG, 600, False)
        assert_rpc_error(RPC_WALLET_UNLOCK_NEEDED, node.dumpprivkey, addr)
        assert_rpc_error(RPC_WALLET_ALREADY_UNLOCKED, node.walletpassphrase, WRONG, 600, True)

        print("The right passphrase does")
        node.walletpassphrase(PASSPHRASE, 600, False)
        node.dumpprivkey(addr)

        print("A wrong passphrase leaves a fully unlocked wallet as it was")
        assert_rpc_error(RPC_WALLET_PASSPHRASE_INCORRECT, node.walletpassphrase, WRONG, 600, True)
        node.dumpprivkey(addr)

        print("walletlock locks it again")
        node.walletlock()
        assert_rpc_error(RPC_WALLET_UNLOCK_NEEDED, node.dumpprivkey, addr)

if __name__ == '__main__':
    WalletUnlockTest().main()
