BitBlocks Core version 1.3.0 is now available from:

  <https://github.com/BitBlocksProject/BitBlocks/releases>

This is a new minor version release. It lets nodes that ended up on a fork
return to the main chain on their own, adds a `-resync` option, replaces the
unlimited zero-fee relay with an anti-spam policy that keeps transactions
free, and fixes several security and stability bugs. Upgrading is strongly
recommended for all users, and especially for masternode operators and
stakers.

Please report bugs using the issue tracker at github:

  <https://github.com/BitBlocksProject/BitBlocks/issues>

How to Upgrade
==============

Shut down the old wallet completely, then install the new version over it
(Windows) or replace the binaries (Linux). Your wallet and block data are kept.
Always keep a backup of `wallet.dat` before upgrading.

No reindex is required. A node that is on one of the known forks undoes the
forked blocks automatically on the first start (see below); this can take a
few minutes and shows "Returning to the correct block chain..." while it runs.

Compatibility
==============

Release binaries are provided for 64-bit Linux (x86_64) and 64-bit Windows.

BitBlocks Core should also build and run on most other Unix-like systems but
is not frequently tested on them.

This release does not change the protocol version (71001) and has no hard
fork or activation height: 1.3.0 nodes stay compatible with 1.2.x and 1.1.x
nodes. Blocks built on the main chain are judged exactly as before. The
changes to block checks only apply rules that older versions already enforced
elsewhere or only in some situations (see "Block validation" below).

Relay policy is stricter than in 1.2.0. Transactions created by a 1.2.0
wallet are always zero-fee, even when the coins are too young to qualify for
free relay; 1.3.0 nodes refuse those transactions, so they only propagate
through older nodes. Upgrade wallets that send transactions.

Notable Changes
===============

### Returning from forks

Several nodes were left stuck on minority forks of the main chain, unable to
reorganize back to it. This release fixes the causes and recovers nodes that
are already stuck:

- **Fork-aware stake checks.** Stake kernels of blocks from another branch
  were checked against our own chain past the fork point, so honest blocks of
  the majority chain failed validation and a node on a minority fork never
  learned that the main chain had more work. The stake modifier and the stake
  input are now taken from the block's own branch. For blocks on top of our
  tip the result is identical to before.
- **Checkpoints after the known forks.** New checkpoints at blocks 3814154,
  4143878 and 4159094, the first main-chain blocks after the splits found on
  the network.
- **Automatic rewind on startup.** If the local chain disagrees with a
  checkpoint, the node undoes it back to the split on startup and then syncs
  the main chain as usual. This can be disabled with `-checkpointrewind=0`.
  Forks more than 50000 blocks past the checkpoint are not undone
  automatically: the GUI offers to download the block chain again, and
  `bitblocksd` stops with a message suggesting `-resync`.
- **SwiftTX locks no longer reject blocks.** A block conflicting with a
  local SwiftTX lock was marked invalid for good, leaving the node stuck on
  its own branch, and the spork key needed to undo that is no longer
  available. Locks still protect the mempool. No node was ever required to
  reject these blocks, so this is not a consensus change.
- **SwiftTX inputs are only locked by complete locks.** Before, a rejected
  lock request, which carries no masternode signatures, already locked its
  inputs, so any peer could make a node reject honest transactions and
  blocks.
- **Masternode pings must reference the active chain.** Pings referencing a
  block outside our active chain are no longer accepted, so masternodes
  running on a fork stop being shown as enabled.

### New `-resync` option

`-resync` deletes the local block chain (the `blocks` and `chainstate`
folders) and downloads it again from the network on startup. The wallet, its
backups, `peers.dat` and the masternode caches are kept. The GUI offers the
same action as a "Resync" button in the Repair tab of the Tools window. Use
it once, not in `bitblocks.conf`.

### Free transactions and anti-spam policy

BitBlocks transactions stay free, but 1.2.0 accepted and relayed every
zero-fee transaction without limit and had an unlimited mempool, which let
anyone flood the network at no cost. Spam now costs coin age instead of a fee.
See `doc/free-transactions.md` for the details.

- A free transaction must reach the free priority threshold (a typical
  payment needs about 1800 BBK-blocks of coin age: 1000 BBK held for about
  2 minutes, 100 BBK for about 18 minutes, or 1 BBK for about 30 hours), spend
  only confirmed outputs and be smaller than 49 KB. Free relay is rate
  limited (`-limitfreerelay`, now 500 kB per minute).
- Each block reserves 250 KB for free transactions (`-blockprioritysize`,
  was 50 KB).
- The mempool is capped at 100 MB (`-maxmempool`), of which free
  transactions may use half. Transactions expire after 24 hours
  (`-mempoolexpiry`).
- Standard transactions are limited to 100 KB (was 1 MB).
- Signatures must use low S values to be relayed and mined. Block validity is
  unchanged.

Wallet behaviour:

- `-sendfreetransactions` now defaults to on, and the wallet decides whether
  a transaction can be free with exactly the rules nodes apply.
- When the coins cannot be sent for free yet, the wallet adds the minimum
  fee (0.0001 BBK/kB). The GUI asks first whether to pay the fee or wait, and
  estimates how long until the coins can be sent for free. RPC calls such as
  `sendtoaddress` pay the minimum fee without asking.
- `-maxtxfee` now defaults to 1 BBK. It was 0, which forced every fee to zero.
- Wallet transactions that expire from the mempool are put back and
  rebroadcast automatically.

### Block validation

- Proof-of-work blocks after the last proof-of-work height (and
  proof-of-stake blocks before it) are now rejected as soon as they arrive,
  not only when they are connected. A coinbase-only proof-of-work block on
  top of the tip could crash a synced node.
- Block value is now always limited to the expected reward. While the
  masternode list was still syncing, the first 100 blocks of each budget
  cycle skipped the check. The budget is 0, so the limit is the regular
  reward.
- After a reorganization, outputs of fully spent coinstakes lost their
  coinstake flag when the block spending them was undone. They then skipped
  the maturity check, and undoing the coinstake block later failed.
- Invalid blocks with an unknown parent are rejected before they are answered
  with a `getblocks`, and repeated orphan blocks from the same peer are
  throttled.

### Security fixes

- `walletpassphrase` always checks the passphrase. Before, when the wallet
  was unlocked for staking only, any passphrase turned it into a full unlock.
- Fixed a buffer overflow when reading a domain name reply from a SOCKS5
  proxy (CVE-2017-18350).
- Block locators larger than 101 entries are refused and the peer is
  disconnected.
- `dumpwallet` refuses to overwrite an existing file.
- The legacy P2P alert system is disabled by default (`-alerts=1` turns it
  back on), as Bitcoin Core and PIVX did (CVE-2016-10724, CVE-2016-10725).

### Staking

- Coins that were spent or locked (for example a masternode collateral)
  after the staking coin set was built are skipped, instead of producing
  invalid blocks or conflicts with pending payments.
- The stake coin set is rebuilt at every new block.
- The staker checks for stakeable coins again while it runs, so coins that
  reach the minimum age after startup start staking without a restart.

### Other changes

- The default number of automatic wallet backups is now 30
  (`-createwalletbackups`, up to 100).

New and changed options
=======================

| Option | Change |
|---|---|
| `-resync` | New. Delete the block chain and download it again on startup. |
| `-checkpointrewind` | New, default 1. Undo a chain that left a checkpoint back to the split on startup. |
| `-maxmempool=<n>` | New, default 100 (MB). |
| `-mempoolexpiry=<n>` | New, default 24 (hours). |
| `-sendfreetransactions` | Default changed from 0 to 1. |
| `-maxtxfee` | Default changed from 0 to 1 BBK. |
| `-blockprioritysize` | Default changed from 50000 to 250000 bytes. |
| `-limitfreerelay` | Default changed from 15 to 500 (kB per minute). |
| `-alerts` | Default changed from 1 to 0. |
| `-createwalletbackups` | Default changed from 10 to 30, maximum from 10 to 100. |
| `-testcheckpoint`, `-maxcheckpointrewind` | New, regtest only, for tests. |

1.3.0 Change log
=================

Detailed release notes follow. This overview includes changes that affect
behavior, not code moves, refactors and string updates.

### Consensus and block validation
- `4fe0640` kernel: follow the block's own chain for stake checks on forks
- `88d8d47` main: reject PoW blocks after the last PoW height in CheckBlock
- `24b6153` masternode-payments: cap block value while masternode sync is incomplete
- `a2fc221` main: restore the coinstake flag when undoing a spent transaction
- `c8db6b4` main: stop rejecting blocks that conflict with SwiftTX locks
- `4876a61` swifttx: only lock inputs once a lock is complete
- `a9c73a9` chainparams: add checkpoints after the known forks
- `9cac087` main: rewind to the first mismatching checkpoint on startup
- `dc81c68` main: don't answer invalid orphan blocks with getblocks

### Mempool and relay policy
- `b2fc781` policy: limit standard transaction size to 100 KB
- `f58331f` mempool: cap total size and the share used by free transactions
- `8ba87a4` mempool: expire transactions after 24 hours
- `6b77425` policy: restore fee, priority and free-relay limits for mempool acceptance
- `a044e7c` policy: scale free-transaction priority threshold for 60-second blocks
- `e13255e` miner: enlarge the free-transaction area to 250 KB
- `12e5ab9` policy: require low-S signatures for relay and mining

### Network
- `447d6c9` net: fix SOCKS5 domain-name length overflow (CVE-2017-18350)
- `4095131` net: limit block locator size to 101 hashes
- `a0c2a28` net: disable the legacy alert system by default
- `63103bd` masternode: reject pings referencing blocks outside the active chain

### Wallet and staking
- `68158fd` wallet: always check the passphrase in Unlock
- `4023979` wallet: send zero-fee transactions by default when coins are old enough
- `f57b647` wallet: compute free-transaction priority exactly like the relay check
- `a04656a` wallet: skip spent or locked coins when staking
- `d8eb234` miner: recheck for stakeable coins while the staker is running
- `3b950d7` rpc: refuse to overwrite an existing file in dumpwallet
- `abc3ff2` config: increase default number of automatic wallet backups to 30

### Startup
- `1f19a19` init: add -resync to delete the block chain and sync from scratch

### GUI
- `0c4c364` qt: ask before paying a fee when coins are not yet eligible for free sending

### Build System
- `110d147` build: bump version to 1.3.0

### Tests
- `32171a0` test: cover mempool limits, expiry and free-tx priority
- `64e684f` test: add regtest -testcheckpoint and a checkpoint rewind test

### Documentation
- `39d4d63` doc: describe the zero-fee anti-spam policy
- `8b62855` doc: correct free-transaction waiting times

## Credits

Thanks to everyone who directly contributed to this release:
- Healseyn (Edson Guimarães)
