macOS Build Notes
=================

BitBlocks Core can be built on macOS with local dependencies or cross-built through the `depends` system. Wallet-compatible builds should use Berkeley DB 4.8, matching the desktop wallet environment.

Install tools
-------------

Install Xcode command line tools:

```bash
xcode-select --install
```

Install Homebrew host packages:

```bash
brew install automake boost miniupnpc openssl pkg-config protobuf qrencode qt@5
```

Build with Berkeley DB 4.8
--------------------------

Use either the repository `depends` build or a local Berkeley DB 4.8 install. Set `BDB_PREFIX` to the Berkeley DB 4.8 prefix:

```bash
export BDB_PREFIX="/path/to/berkeley-db-4.8"
export OPENSSL_PREFIX="$(brew --prefix openssl)"
export QT_PREFIX="$(brew --prefix qt@5)"

./autogen.sh
./configure BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" BDB_CFLAGS="-I${BDB_PREFIX}/include" OPENSSL_LIBS="-L${OPENSSL_PREFIX}/lib -lssl -lcrypto" OPENSSL_CFLAGS="-I${OPENSSL_PREFIX}/include" PKG_CONFIG_PATH="${QT_PREFIX}/lib/pkgconfig:${OPENSSL_PREFIX}/lib/pkgconfig"
make -j$(sysctl -n hw.ncpu)
```

Daemon-only build:

```bash
export BDB_PREFIX="/path/to/berkeley-db-4.8"
export OPENSSL_PREFIX="$(brew --prefix openssl)"

./autogen.sh
./configure --without-gui BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" BDB_CFLAGS="-I${BDB_PREFIX}/include" OPENSSL_LIBS="-L${OPENSSL_PREFIX}/lib -lssl -lcrypto" OPENSSL_CFLAGS="-I${OPENSSL_PREFIX}/include" PKG_CONFIG_PATH="${OPENSSL_PREFIX}/lib/pkgconfig"
make -j$(sysctl -n hw.ncpu)
```

Application bundle
------------------

When the Qt build succeeds, the GUI binary is available as `src/qt/bitblocks-qt`. Packaging a signed `.app` bundle for distribution requires the maintainer's Apple Developer certificate and is covered in [README_osx.md](README_osx.md).

Runtime data directory
----------------------

The default macOS data directory is:

```text
~/Library/Application Support/BitBlocks
```

Further reading
---------------

- [Generic build notes](build-generic.md)
- [macOS packaging notes](README_osx.md)
- Releases: https://github.com/BitBlocksProject/BitBlocks/releases
