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

The generated prefix directory is normalized to `depends/x86_64-mingw32`.

Use `HOST=i686-w64-mingw32` if you need a 32-bit Windows build; the matching prefix is `depends/i686-mingw32`.

Build BitBlocks Core
--------------------

```bash
./autogen.sh
./configure --prefix="$PWD/depends/x86_64-mingw32" --host=x86_64-w64-mingw32
make -j$(nproc)
```

Typical Windows binaries are produced under `src/` and `src/qt/`:

- `bitblocksd.exe`
- `bitblocks-cli.exe`
- `bitblocks-tx.exe`
- `bitblocks-qt.exe`

Installer builds
----------------

The repository includes NSIS installer metadata under `share/`. After cross-compiling, install NSIS so `makensis` is available, then run the deploy target:

```bash
sudo apt-get install nsis
make deploy
```

The installer includes the GUI wallet, daemon, and CLI. `bitblocks-tx.exe` is built from source but may not be included in the NSIS installer package.

Release branding note
---------------------

Some installer metadata in `share/setup.nsi.in` still uses legacy project URLs. Update that source file before producing public installers if release branding must use `bitblockscrypto.com` everywhere.

Runtime data directory
----------------------

The default Windows data directory is `%APPDATA%\BitBlocks`. The default mainnet ports are P2P `58697` and RPC `59768`.

Further reading
---------------

- [Generic build notes](build-generic.md)
- [Depends build system](../depends/README.md)
- Releases: https://github.com/BitBlocksProject/BitBlocks/releases
