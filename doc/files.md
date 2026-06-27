Data Files
==========

BitBlocks Core stores blockchain, wallet, and runtime files in its data directory.

Default data directories
------------------------

- Linux: `~/.bitblocks`
- Windows: `%APPDATA%\BitBlocks`
- macOS: `~/Library/Application Support/BitBlocks`
- Testnet: `testnet4` inside the selected data directory
- Regtest: `regtest` inside the selected data directory

Configuration placement
-----------------------

- `bitblocks.conf` lives in the base data directory, for example `~/.bitblocks/bitblocks.conf`.
- `masternode.conf` lives in the active network data directory, for example `~/.bitblocks/masternode.conf` on mainnet or `~/.bitblocks/testnet4/masternode.conf` on testnet.

Common files and directories
----------------------------

| Path | Purpose |
| ---- | ------- |
| `blocks/blk*.dat` | Raw block flat files. |
| `blocks/rev*.dat` | Undo data for block disconnection. |
| `blocks/index/` | Block index database. |
| `chainstate/` | Current UTXO set database. |
| `database/` | Berkeley DB environment files for wallet support. |
| `debug.log` | Main runtime log. |
| `peers.dat` | Known peer addresses. |
| `wallet.dat` | Default wallet file when wallet support is enabled. |
| `backups/` | Automatic wallet backup files. |
| `mncache.dat` | Masternode list cache. |
| `budget.dat` | Budget proposal cache. |
| `fee_estimates.dat` | Fee estimate persistence. |
| `.lock` | Process lock file for the data directory. |
| `bitblocksd.pid` | Daemon PID file when running with a PID file. |
| `bootstrap.dat` | Optional one-time local block import file. |
| `bootstrap.dat.old` | Imported bootstrap file after successful rename. |
| `bitblocks.conf` | Optional node configuration file. |
| `masternode.conf` | Optional controller-wallet masternode configuration. |

Backups
-------

Back up wallet files before upgrades, rescans, or moving data directories. Stop BitBlocks Core before copying `wallet.dat` to avoid inconsistent backups.
