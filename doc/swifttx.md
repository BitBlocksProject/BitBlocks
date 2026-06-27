# SwiftTX

SwiftTX is BitBlocks Core's instant transaction locking feature. It uses masternode participation to reduce the time a recipient needs to wait before treating a transaction as settled.

Using SwiftTX
-------------

The wallet RPC entry point for an instant send is `sendtoaddressix`:

```bash
bitblocks-cli help sendtoaddressix
bitblocks-cli sendtoaddressix "BExampleAddress" 1.0
```

Regular `sendtoaddress` and `sendmany` create normal wallet transactions and do not request SwiftTX locking in this codebase.

Configuration
-------------

SwiftTX can be controlled with runtime options:

```bash
bitblocksd -help | grep -i swifttx
```

Important options include `-enableswifttx` and `-swifttxdepth`.

Confirmation depth
------------------

BitBlocks Core uses a default SwiftTX depth of `5`, with a maximum accepted depth of `60`.

Operational notes
-----------------

- SwiftTX relies on a healthy masternode network.
- Wallets should still handle normal confirmation flows when instant locking is unavailable.
- Exchanges and services should set policies based on their own risk tolerance.

Explorer
--------

Use the official explorer for public transaction lookup: https://explorer.bitblockscrypto.com
