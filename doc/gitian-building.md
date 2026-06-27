Gitian Building Notes
=====================

This repository does not currently ship a complete Gitian release setup. There is no in-tree Gitian build script or descriptor directory, so maintainers should not treat this file as an executable release recipe.

Current reproducible-build status
---------------------------------

The supported in-tree reproducibility aid is the `depends` build system:

- Native dependency builds for Linux.
- Cross-dependency builds for Windows.
- Cross-dependency builds for macOS when the required SDK is available.

See:

- [Generic build notes](build-generic.md)
- [Windows build notes](build-windows.md)
- [macOS packaging notes](README_osx.md)
- [Depends build system](../depends/README.md)

If Gitian is restored
---------------------

A future Gitian setup should add and document:

- Gitian descriptor files.
- A maintainer build script.
- Signature repositories and verification steps.
- Exact base image or distribution versions.
- Expected output names and checksums.

Until those files exist, use this document as a placeholder and rely on signed tags, `depends`, checksums, and independent maintainer builds for release verification.

Release locations
-----------------

- Source: https://github.com/BitBlocksProject/BitBlocks
- Releases: https://github.com/BitBlocksProject/BitBlocks/releases
