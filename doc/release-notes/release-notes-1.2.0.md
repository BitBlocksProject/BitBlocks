BitBlocks Core version 1.2.0 is now available from:

  <https://github.com/BitBlocksProject/BitBlocks/releases>

This is a new minor version release. It moves consensus signature
verification from OpenSSL to libsecp256k1, hardens proof-of-stake validation
against fake stake attacks, improves peer discovery, and updates the GUI and
the build dependencies. Upgrading is strongly recommended for all users.

Please report bugs using the issue tracker at github:

  <https://github.com/BitBlocksProject/BitBlocks/issues>

How to Upgrade
==============

Shut down the old wallet completely, then install the new version over it
(Windows) or replace the binaries (Linux). Your wallet and block data are kept.
Always keep a backup of `wallet.dat` before upgrading.

No reindex is required.

Compatibility
==============

Release binaries are provided for 64-bit Linux (x86_64) and 64-bit Windows.

BitBlocks Core should also build and run on most other Unix-like systems but
is not frequently tested on them.

This release does not change the consensus rules or the protocol version
(71001). 1.2.0 nodes stay fully compatible with 1.1.x nodes on the network.

Notable Changes
===============

### Consensus signature verification uses libsecp256k1

Signatures were previously verified with whichever OpenSSL the binary was
linked against. OpenSSL versions disagree on some non-canonical signature
encodings, so two nodes built with different OpenSSL versions could disagree
about whether a block is valid. Verification now uses the libsecp256k1
library vendored in the source tree, which is the same on every build.

This is not a consensus rule change and has no activation height: both
verifiers accept the same signatures on the existing chain. The node now logs
which verifier it uses at startup (`Signature verification: libsecp256k1`).
The OpenSSL path is kept only for testing and can be selected at build time
with `--enable-openssl-ecdsa`. See `doc/secp256k1-migration.md` for the full
analysis.

### Fake stake mitigation

Proof-of-stake validation has been hardened against peers sending invalid or
duplicated stake blocks:

- The coinstake prevout index is now validated before use.
- Peers sending proof-of-stake blocks with invalid stake signatures, or blocks
  that fail other checks the peer could verify itself, now receive a DoS
  score and are eventually banned. Failures that depend on local state (for
  example, still syncing) are never scored, so honest peers are not banned.
- Two different blocks that reuse the same stake at the same time are dropped
  locally. This check is applied only to blocks received from the network,
  never during `-reindex` or `-loadblock`.

### Peer discovery

- The mainnet DNS seeds and fixed seeds have been updated.
- The node now keeps querying the DNS seeds while it has fewer than two
  outbound peers, instead of querying only once at startup.
- If there is still no outbound peer after 60 seconds, the node connects
  directly to the seed nodes as a fallback.

This fixes nodes that previously stayed at zero connections after a failed
first lookup.

### Mainnet checkpoints

Checkpoints were added at blocks 4000000 and 4100000.

### Wallet

- On startup, the wallet now always rescans from its last known block.
  Previously it skipped the rescan when it was fewer than 10 blocks behind,
  which could miss transactions after restoring a backup.
- Wallet rescans no longer freeze the GUI, and a crash caused by a null
  dereference during rescan has been fixed.

### GUI

- Refreshed wallet artwork, navigation icons, and splash screen.
- The layout is optimized for 1366x768 displays.
- High-DPI scaling is enabled.
- The Inter font is now bundled, so the GUI looks the same on every system.
- The splash screen and the About dialog show the exact build version.
- Fixed the "last month" transaction filter in January.

### Build system and dependencies

- Qt was updated to 5.15.5, Boost to 1.81.0, and OpenSSL to 3.0.16.
- BIP70 payment requests are now optional and disabled by default
  (`--enable-bip70`). `bitblocks:` URIs keep working without it.
- The code builds with modern compilers and current versions of Boost.Asio,
  OpenSSL, Qt 5, and LevelDB.
- Added release packaging scripts for Linux and Windows
  (`contrib/release/`).
- Generated autotools files are no longer tracked in the repository.
  Run `./autogen.sh` before `./configure` when building from source.

1.2.0 Change log
=================

Detailed release notes follow. This overview includes changes that affect
behavior, not code moves, refactors and string updates.

### Consensus
- `88ceca4` consensus: use libsecp256k1 for signature verification
- `e7187fd` script: harden secp256k1 verification
- `0735a8c` key: update libsecp256k1 API and initialize verification
- `36e3c07` kernel: validate coinstake prevout index
- `6f15fe6` main: score invalid proof-of-stake blocks
- `03298d5` main: score invalid stake signatures
- `9624f11` main: reject duplicate network stakes
- `790495e` main: remove unused stake cache

### Network
- `3e95fad` Fix mainnet DNS and fixed seeds
- `7bf3a9f` net: retry DNS seeds and connect to seed fallbacks
- `006c1a3` chainparams: update mainnet checkpoints

### Wallet
- `080b669` init: always rescan from best block
- `2e67b66` wallet: fix rescan responsiveness and null dereference

### GUI
- `86943e5` qt: refresh wallet artwork and navigation icons
- `1b46783` qt: optimize wallet layout for 1366x768 displays
- `49a1753` qt: embed branded logo in splash background
- `13237c2` qt: enable high-DPI scaling
- `9ed85e8` qt: bundle Inter font
- `78e3efd` qt: show build version in UI
- `2f29366` qt: load stylesheet as UTF-8
- `5afc2ca` qt: remove startup fade-in
- `b6db0d4` qt: remove page transition opacity effect
- `6bb1d4f` qt: fix January last-month filter
- `9fd2050` qt: keep URI handling without BIP70

### Build System
- `4f62309` qt: update depends to Qt 5.15 and make BIP70 optional
- `f332cfb` depends: update Boost and OpenSSL
- `e2ce97b` depends: update OpenSSL and fix miniupnpc cross-build
- `565add1` depends: build Qt XCB platform with depends libraries
- `139fef0` build: link static Qt XCB plugin
- `878f95d` depends: add rpath-link for X11 libraries
- `c0dcaa6` build: detect Qt 5.8+ platform support libraries
- `5d2e19c` build: pass configure defines to moc
- `950d362` build: fix secp256k1 build and tests
- `63d1636` Update for modern C++ compatibility
- `8c7be4a` share: fix NSIS out-of-tree builds
- `a7d07de` contrib: add Linux and Windows release packaging
- `47d627b` build: bump version to 1.2.0
- `3a9909a` build: stop tracking generated files

### Tests
- `8a48d30` test: scan both signature verifiers

### Documentation
- `d51c073` doc: document verifier migration results
- `4c18f29` Update build DOC to use PIVX PPA for Berkeley DB 4.8

## Credits

Thanks to everyone who directly contributed to this release:
- Healseyn (Edson Guimarães)
