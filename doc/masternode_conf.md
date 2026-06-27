Masternode Configuration
========================

`masternode.conf` lets one controller wallet manage one or more remote BitBlocks masternodes. Each masternode requires a collateral output of exactly `150,000 BBK` with at least `15` confirmations before start.

File locations
--------------

Place `masternode.conf` in the active network data directory for the controller wallet:

- Mainnet Linux: `~/.bitblocks/masternode.conf`
- Testnet Linux: `~/.bitblocks/testnet4/masternode.conf`
- Windows mainnet: `%APPDATA%\BitBlocks\masternode.conf`
- macOS mainnet: `~/Library/Application Support/BitBlocks/masternode.conf`

`bitblocks.conf` is read from the base data directory, while `masternode.conf` is network-specific.

Format
------

Each non-comment line has this format:

```text
alias ip:port masternode_private_key collateral_txid collateral_output_index
```

Mainnet masternodes should use port `58697`.

Example:

```text
mn1 203.0.113.10:58697 93HaYBVUCYjEMeeH1Y4sBGLALQZE1Yc1K64xiqgX37tGBDQL8Xg 2bcd3c84c84f87eaa86e4e56834c92927a07f9e18718810b92e0d0324456a67c 0
mn2 203.0.113.11:58697 93WaAb3htPJEV8E9aQcN23Jt97bPex7YvWfgMDTUdWJvzmrMqey aa9f1034d973377a5e733272c3d0eced1de22555ad45d6b24abadff8087948d4 1
```

Required values
---------------

- `alias`: local name used by the controller wallet.
- `ip:port`: public IP address and P2P port of the remote node.
- `masternode_private_key`: key generated for the remote masternode.
- `collateral_txid`: transaction id containing the `150,000 BBK` collateral output.
- `collateral_output_index`: output index for the collateral.

Common RPC commands
-------------------

```bash
bitblocks-cli createmasternodekey
bitblocks-cli getmasternodeoutputs
bitblocks-cli listmasternodeconf
bitblocks-cli walletpassphrase "your passphrase" 120
bitblocks-cli startmasternode alias false mn1
bitblocks-cli startmasternode many false
```

The `masternode` wrapper command is deprecated and kept for old scripts. Prefer direct RPC methods such as `createmasternodekey`, `getmasternodeoutputs`, `listmasternodeconf`, and `startmasternode`.

Remote node configuration
-------------------------

The remote node should have a `bitblocks.conf` similar to:

```ini
server=1
daemon=1
listen=1
txindex=1
masternode=1
masternodeaddr=203.0.113.10:58697
externalip=203.0.113.10:58697
masternodeprivkey=PASTE_MASTERNODE_PRIVATE_KEY_HERE
```

If `txindex=1` is added to an existing node, restart once with `-reindex` so the transaction index is built.

Restart the remote daemon after editing the file, wait for synchronization, then start the masternode from the controller wallet.
