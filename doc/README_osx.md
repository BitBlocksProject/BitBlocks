macOS Packaging Notes
=====================

This document covers maintainer-oriented macOS packaging notes. For local source builds, see [build-osx.md](build-osx.md).

Runtime defaults
----------------

- GUI binary: `bitblocks-qt`
- Daemon binary: `bitblocksd`
- Data directory: `~/Library/Application Support/BitBlocks`
- Mainnet P2P port: `58697`
- Mainnet RPC port: `59768`

Unsigned local builds
---------------------

Local development builds do not need Apple code signing. You can run the GUI directly from `src/qt/bitblocks-qt` or package a local app bundle for private testing.

Packaging targets
-----------------

The repository's top-level makefile defines packaging targets for the Qt application bundle and disk image:

```bash
make appbundle
make deploy
```

`make deploy` produces the release disk image when the macOS packaging prerequisites are available.

Signed release builds
---------------------

Public macOS releases should be built from a signed tag and signed by a project maintainer with access to the release certificate. The `contrib/macdeploy/` scripts provide detached-signature helpers used by the packaging flow.

At a high level:

1. Build the release candidate from a signed tag.
2. Create the `.app` bundle with `make appbundle` or the deploy target.
3. Create detached signatures with `contrib/macdeploy/detached-sig-create.sh` when using a detached signing flow.
4. Apply signatures and produce the `.dmg` with `make deploy`.
5. Publish checksums and binaries at https://github.com/BitBlocksProject/BitBlocks/releases.

Cross-builds
------------

The `depends` system supports macOS host triplets when the required SDK is available. This tree expects an SDK path containing `MacOSX10.11.sdk` for the configured darwin host.

```bash
export SDK_PATH=/path/to/macos/SDKs
cd depends
make HOST=x86_64-apple-darwin11 -j4
cd ..
./autogen.sh
./configure --prefix="$PWD/depends/x86_64-darwin" --host=x86_64-apple-darwin11
make -j$(nproc)
```

Troubleshooting
---------------

- If Qt tools are missing, check `PKG_CONFIG_PATH` and the selected Qt version.
- If Berkeley DB is not detected, verify that the Berkeley DB 4.8 include and library paths are passed to `configure`.
- If Gatekeeper blocks an unsigned local app, run the command-line binary directly or sign it with a local development identity.
