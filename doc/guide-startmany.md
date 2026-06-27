Start-Many Masternode Guide
===========================

This guide describes the common controller-wallet setup for starting several BitBlocks masternodes. Each masternode requires a separate collateral output of exactly `150,000 BBK` and at least `15` confirmations before start.

Overview
--------

For each masternode you need:

- A remote VPS or server running `bitblocksd`.
- A public IP address reachable on port `58697`.
- One `150,000 BBK` collateral output in the controller wallet.
- One masternode private key.
- One line in the controller wallet `masternode.conf`.

Prepare the controller wallet
-----------------------------

Create a receiving address for each masternode collateral and send exactly `150,000 BBK` to each address. Wait for at least `15` confirmations before starting the masternode.

Generate keys and outputs:

```bash
bitblocks-cli createmasternodekey
bitblocks-cli getmasternodeoutputs
```

If the controller wallet is encrypted, unlock it before starting masternodes:

```bash
bitblocks-cli walletpassphrase "your passphrase" 120
```

Configure `masternode.conf`
---------------------------

Add one line per masternode in the controller wallet's active network data directory:

```text
mn1 203.0.113.10:58697 MASTERNODE_PRIVATE_KEY TXID OUTPUT_INDEX
mn2 203.0.113.11:58697 MASTERNODE_PRIVATE_KEY TXID OUTPUT_INDEX
```

Use descriptive aliases. The alias is only local to the controller wallet.

Configure each remote node
--------------------------

On each remote server, create or edit `bitblocks.conf` in the base data directory:

```ini
rpcuser=bitblocks_rpc
rpcpassword=change_this_to_a_strong_password
server=1
daemon=1
listen=1
txindex=1
masternode=1
masternodeaddr=203.0.113.10:58697
externalip=203.0.113.10:58697
masternodeprivkey=MASTERNODE_PRIVATE_KEY
```

If `txindex=1` is added to an existing node, restart once with `-reindex`.

Start or restart the remote daemon:

```bash
bitblocksd -daemon
bitblocks-cli getblockchaininfo
```

Start masternodes from the controller
-------------------------------------

After the remote nodes are synchronized:

```bash
bitblocks-cli listmasternodeconf
bitblocks-cli startmasternode many false
```

To start a single alias:

```bash
bitblocks-cli startmasternode alias false mn1
```

Verification
------------

From the controller wallet:

```bash
bitblocks-cli listmasternodeconf
bitblocks-cli listmasternodes
```

From the remote masternode daemon:

```bash
bitblocks-cli getmasternodestatus
```

You can also verify the collateral and masternode activity with the official explorer: https://explorer.bitblockscrypto.com
