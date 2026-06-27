BitBlocks Core Documentation
============================

BitBlocks Core is the reference wallet and node software for the BitBlocks network. It stores and validates the blockchain, relays transactions and blocks, and provides the command-line, RPC, and Qt interfaces used by operators and wallets.

Official links
--------------

- Website: https://www.bitblockscrypto.com/
- Source code: https://github.com/BitBlocksProject/BitBlocks
- Releases: https://github.com/BitBlocksProject/BitBlocks/releases
- Issues: https://github.com/BitBlocksProject/BitBlocks/issues
- Explorer: https://explorer.bitblockscrypto.com
- Community: https://discord.gg/bitblocks

Network defaults
----------------

| Network | P2P port | RPC port | Data directory suffix |
| ------- | -------- | -------- | --------------------- |
| Mainnet | 58697 | 59768 | default |
| Testnet | 39795 | 39799 | testnet4 |
| Regtest | 39793 | 39799 | regtest |

Default data directories:

- Linux: `~/.bitblocks`
- Windows: `%APPDATA%\BitBlocks`
- macOS: `~/Library/Application Support/BitBlocks`

Running BitBlocks Core
----------------------

Download a release from https://github.com/BitBlocksProject/BitBlocks/releases or build from source. Common binaries are:

- `bitblocks-qt`: graphical wallet
- `bitblocksd`: background daemon
- `bitblocks-cli`: command-line RPC client
- `bitblocks-tx`: transaction utility built from source; it may not be included in every installer package

Typical daemon usage:

```bash
bitblocksd -daemon
bitblocks-cli getblockchaininfo
bitblocks-cli stop
```

Building
--------

- [Generic build notes](build-generic.md)
- [Unix build notes](build-unix.md)
- [Windows build notes](build-windows.md)
- [macOS build notes](build-osx.md)
- [macOS packaging notes](README_osx.md)
- [Windows notes](README_windows.txt)
- [Gitian build notes](gitian-building.md)

Operations
----------

- [Bootstrap and synchronization](bootstrap.md)
- [Masternode configuration](masternode_conf.md)
- [Start-many masternode guide](guide-startmany.md)
- [Masternode budget commands](masternode-budget.md)
- [SwiftTX instant locks](swifttx.md)
- [Init scripts and service examples](init.md)
- [Tor support](tor.md)
- [ZMQ interface](zmq.md)
- [Unauthenticated REST interface](REST-interface.md)
- [Dnsseed policy](dnsseed-policy.md)
- [Data files](files.md)

License
-------

BitBlocks Core is distributed under the MIT software license. See [COPYING](../COPYING) for details.
