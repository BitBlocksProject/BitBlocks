Unauthenticated REST Interface
==============================

BitBlocks Core can expose a small unauthenticated REST interface when REST support is enabled. The interface is served by the RPC HTTP server, so mainnet examples use RPC port `59768` unless you configured another `-rpcport`.

Enable REST
-----------

Start the node with REST enabled:

```bash
bitblocksd -daemon -rest
```

Available paths
---------------

This tree registers REST handlers for transactions and blocks:

```text
/rest/tx/<txid>.<bin|hex|json>
/rest/block/<blockhash>.<bin|hex|json>
/rest/block/notxdetails/<blockhash>.<bin|hex|json>
```

Examples
--------

```bash
curl http://127.0.0.1:59768/rest/block/HEX_BLOCK_HASH.json
curl http://127.0.0.1:59768/rest/block/notxdetails/HEX_BLOCK_HASH.json
curl http://127.0.0.1:59768/rest/tx/HEX_TXID.json
```

Use RPC for chain summary data:

```bash
bitblocks-cli getblockchaininfo
```

Security
--------

The REST interface is unauthenticated. Bind RPC to localhost unless you have a carefully controlled reverse proxy or firewall:

```ini
rpcbind=127.0.0.1
rpcallowip=127.0.0.1
```

Do not expose RPC or REST directly to the public internet.
