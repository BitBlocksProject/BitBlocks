Free transactions and anti-spam policy
=======================================

BitBlocks transactions are free: the wallet sends them without a fee whenever
it can. Earlier releases achieved this by accepting and relaying every
zero-fee transaction without any limit, and by an unlimited mempool. That let
anyone flood the network at no cost (exhausting node memory, bandwidth and
CPU) and is fixed here. Transactions stay free for normal use; spam now costs
*coin age* instead of a fee.

All changes below are relay, mempool, mining and wallet **policy**. Block
validity is unchanged: there is no fork and no activation height, and upgraded
nodes accept every block that older nodes accept.

How free transactions work
--------------------------

A transaction is free when it pays less than the relay fee
(`-minrelaytxfee`, 0.0001 BBK/kB). A free transaction is relayed and mined
only if:

- it reaches the free priority threshold. Priority is
  `sum(input value * input confirmations) / modified size`, where the
  modified size leaves out most of each input. A typical payment (one input,
  two outputs) needs about 1800 BBK-blocks of coin age: 1000 BBK held for
  about 2 minutes, 100 BBK for about 18 minutes, or 1 BBK for about 30 hours.
  Spending coins uses their age up, so sending many free transactions requires
  holding a lot of coins for a long time: keeping up 200 free transactions a
  minute takes roughly 200,000 BBK.
  (The threshold is ten times the Bitcoin value because BitBlocks blocks come
  every 60 seconds instead of every 10 minutes.)
- it spends only confirmed outputs (no chains of unconfirmed free
  transactions);
- it is smaller than 49 KB;
- the node's free-relay rate limit is not exceeded (`-limitfreerelay`).

Each block reserves 250 KB (`-blockprioritysize`) for free transactions,
roughly 1000 transactions a minute. The rest of the block goes to transactions
paying a fee, which are always available as a fallback.

Wallet behaviour
----------------

- `-sendfreetransactions` now defaults to on.
- When the selected coins cannot be sent for free yet (too young, unconfirmed
  change, or a transaction larger than 10 KB), the wallet adds the minimum
  fee (0.0001 BBK/kB, a fraction of a cent).
  - The GUI asks first: pay the fee and send now, or wait. It shows an
    estimate of how long until the coins can be sent for free.
  - RPC calls (`sendtoaddress`, `sendmany`, ...) pay the minimum fee without
    prompting, so services such as the BBK bridge keep working unchanged.
- `-maxtxfee` now defaults to 1 BBK. It was 0, which forced every fee to
  zero and made the wallet create zero-fee transactions that low-priority
  coins could not get mined with.
- Transactions of the wallet that expire from the mempool are put back and
  rebroadcast automatically.

New and changed options
-----------------------

| Option | Default | Notes |
|---|---|---|
| `-maxmempool=<MB>` | 100 | New. Relayed transactions are refused when the mempool is full; free transactions may use at most half of it. Actual memory use is about three times this value. |
| `-mempoolexpiry=<hours>` | 24 | New. Unconfirmed transactions older than this are dropped. |
| `-limitfreerelay=<n>` | 500 | Now enforced again (was 15, and ignored). In thousands of bytes per minute. |
| `-relaypriority` | 1 | Now enforced again. |
| `-blockprioritysize=<n>` | 250000 | Was 50000. |
| `-sendfreetransactions` | 1 | Was 0. |
| `-maxtxfee=<amt>` | 1 | Was 0. |

Standard transactions are now limited to 100 KB (was 1 MB).

Rollout
-------

Nodes enforcing the new policy refuse zero-fee transactions without enough
priority, which older wallets create (they always send with a zero fee).
Upgrade in this order:

1. Exchanges, the bridge and other services running `bitblocksd`.
2. Users' wallets.
3. Stakers and masternodes. Protection against spam is complete once most of
   them run the new version.

During the transition, a low-priority zero-fee transaction from an old wallet
may still be relayed by old nodes and mined by old stakers, but upgraded nodes
will not relay it. Upgrading the sending wallet fixes this.
