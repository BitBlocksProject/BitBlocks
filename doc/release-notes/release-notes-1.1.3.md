BitBlocks Core version 1.1.3 is now available from:

  <https://github.com/bitblocks-project/bitblocks/releases>

This is a new patch version release, including various bug fixes, performance improvements, GUI enhancements, and updated translations.

Please report bugs using the issue tracker at github:

  <https://github.com/bitblocks-project/bitblocks/issues>

Compatibility
==============

BitBlocks Core is extensively tested on multiple operating systems using
the Linux kernel, macOS 10.8+, and Windows Vista and later.

BitBlocks Core should also work on most other Unix-like systems but is not
frequently tested on them.

Notable Changes
===============

### GUI Improvements
- Modernized GUI with card-based design, improved UX, and visual enhancements
- Implemented stable sort in TransactionFilterProxy using sortKey and hash

### Performance Improvements
- Optimized wallet rescan performance and prevented UI freezing

### Network and Consensus
- Updated mainnet checkpoints to block 3685080
- Added new DNS seeds
- Refactored: Separated PoW and PoS target spacing/timespan parameters
- Fixed: Corrected fShouldBan flag in Misbehaving()

### Build System
- Fixed Windows compilation with MinGW-w64
- Cleaned repository: removed autotools generated files
- Added .gitignore to prevent committing generated files

### Translations
- Completed pt_BR translations and fixed typo

### Bug Fixes
- Fixed block reward error

1.1.3 Change log
=================

Detailed release notes follow. This overview includes changes that affect
behavior, not code moves, refactors and string updates.

### GUI
- Modernized GUI with card-based design, improved UX, and visual enhancements
- Implemented stable sort in TransactionFilterProxy using sortKey and hash

### Wallet
- Optimized wallet rescan performance and prevented UI freezing

### Network
- Updated mainnet checkpoints to block 3685080
- Added new DNS seeds

### Consensus
- Refactored: Separated PoW and PoS target spacing/timespan parameters
- Fixed: Corrected fShouldBan flag in Misbehaving()
- Fixed block reward error (2022)

### Build System
- Fixed Windows compilation with MinGW-w64
- Cleaned repository: removed autotools generated files
- Added .gitignore to prevent committing generated files

### Translations
- Completed pt_BR translations and fixed typo

Credits
=======

Thanks to everyone who directly contributed to this release:
- Healseyn (Edson Guimarães)
