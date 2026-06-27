Dnsseed Policy
==============

BitBlocks DNS seeds help new mainnet nodes discover peers. Seed operators should provide stable, well-maintained infrastructure and avoid returning stale or malicious peers.

Current mainnet seeds
---------------------

The source tree configures these seeds:

```text
seed1.bitblockscrypto.com
seed2.bitblockscrypto.com
seed3.bitblockscrypto.com
seed4.bitblockscrypto.com
seed5.bitblockscrypto.com
```

Other networks
--------------

Testnet currently ships with no active DNS seeds. Operators should use `-addnode`, `-connect`, or manual peer discovery when bootstrapping a testnet node.

Regtest and unit-test networks do not use DNS seeds.

Fixed seeds
-----------

The source tree may also include hardcoded fixed seed addresses for network bootstrapping. Changes to fixed seeds and DNS seeds should be reviewed together during release preparation.

Operator expectations
---------------------

Seed operators should:

- Crawl the BitBlocks network regularly.
- Return peers that recently served valid blocks.
- Filter private, unroutable, and misbehaving addresses.
- Monitor seed availability.
- Coordinate changes through https://github.com/BitBlocksProject/BitBlocks/issues.

Adding or removing a seed
-------------------------

Changes to hardcoded DNS seeds require a source change, review, and release. Proposed seeds should include:

- Operator contact.
- Hostname.
- Monitoring approach.
- Expected geographic and network diversity.
