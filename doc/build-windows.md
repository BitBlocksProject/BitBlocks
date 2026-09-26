Windows Build Notes
===================

The recommended Windows build path is cross-compilation from Linux using the `depends` system and MinGW-w64. This keeps dependency versions reproducible and avoids relying on a manually configured Windows environment.

Build host packages
-------------------

On Ubuntu or Debian:

```bash
sudo apt-get update
sudo apt-get install build-essential libtool autotools-dev autoconf automake pkg-config bsdmainutils curl git ca-certificates mingw-w64 g++-mingw-w64-x86-64
```

Build dependencies
------------------

From the repository root:

```bash
cd depends
make HOST=x86_64-w64-mingw32 -j4
cd ..
```

The generated prefix directory is `depends/x86_64-w64-mingw32`.

Use `HOST=i686-w64-mingw32` if you need a 32-bit Windows build; the matching prefix is `depends/i686-w64-mingw32`.

Build BitBlocks Core
--------------------

```bash
./autogen.sh
CONFIG_SITE="$PWD/depends/x86_64-w64-mingw32/share/config.site" ./configure --prefix=/
make -j$(nproc)
```

Typical Windows binaries are produced under `src/` and `src/qt/`:

- `bitblocksd.exe`
- `bitblocks-cli.exe`
- `bitblocks-tx.exe`
- `bitblocks-qt.exe`

Release builds
--------------

Like PIVX's gitian builds, official Windows releases are cross-compiled inside
the pinned Ubuntu 20.04 container (MinGW-w64 posix threads, NSIS). With
Docker installed, run:

```bash
./contrib/release/docker-release.sh win64
```

This builds all pinned `depends`, compiles the binaries, strips them, checks
that they only import DLLs shipped with Windows, and writes to `release/`:

- `bitblocks-<version>-win64.zip`: `bitblocksd.exe`, `bitblocks-cli.exe`,
  `bitblocks-tx.exe` and `bitblocks-qt.exe`
- `bitblocks-<version>-win64-setup.exe`: NSIS installer with the GUI wallet,
  daemon and CLI

Each artifact gets a matching `.sha256` file. Run the script without
arguments to build the Linux release as well.

Installer builds
----------------

For a manual build, install NSIS so `makensis` is available when running
`configure`, then run the deploy target:

```bash
sudo apt-get install nsis
make deploy
```

The installer is written to the build directory as
`bitblocks-<version>-win64-setup.exe`.

Runtime data directory
----------------------

The default Windows data directory is `%APPDATA%\BitBlocks`. The default mainnet ports are P2P `58697` and RPC `59768`.

Further reading
---------------

- [Generic build notes](build-generic.md)
- [Depends build system](../depends/README.md)
- Releases: https://github.com/BitBlocksProject/BitBlocks/releases
