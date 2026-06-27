ZMQ Interface
=============

BitBlocks Core can publish block, transaction, and SwiftTX lock notifications over ZeroMQ when built with ZMQ support.

Configuration
-------------

Add one or more endpoints to `bitblocks.conf`:

```ini
zmqpubhashblock=tcp://127.0.0.1:28339
zmqpubhashtx=tcp://127.0.0.1:28339
zmqpubrawblock=tcp://127.0.0.1:28339
zmqpubrawtx=tcp://127.0.0.1:28339
zmqpubhashtxlock=tcp://127.0.0.1:28339
zmqpubrawtxlock=tcp://127.0.0.1:28339
```

Restart the daemon after changing ZMQ settings:

```bash
bitblocksd -daemon
```

Notifications
-------------

Common topics:

- `hashblock`: block hash.
- `hashtx`: transaction hash.
- `rawblock`: serialized block.
- `rawtx`: serialized transaction.
- `hashtxlock`: SwiftTX-locked transaction hash.
- `rawtxlock`: serialized SwiftTX-locked transaction.

Security
--------

ZMQ sockets do not include authentication by default. Bind to localhost unless the network path is trusted and protected.

Testing
-------

This codebase does not expose an RPC for listing active ZMQ publishers. Confirm publishers by checking the daemon command-line/configuration and by subscribing with a ZMQ client.
