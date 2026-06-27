Generic Build Notes
===================

BitBlocks Core uses the Autotools build system. The normal build flow is:

```bash
./autogen.sh
./configure
make -j$(nproc)
make install # optional
```

Build documentation
-------------------

- [Unix build notes](build-unix.md)
- [Windows build notes](build-windows.md)
- [macOS build notes](build-osx.md)
- [Depends build system](../depends/README.md)

Required tools
--------------

Install a C++ compiler, Autotools, pkg-config, and the development libraries for OpenSSL and Boost. Wallet builds also require Berkeley DB 4.8 for compatibility with existing wallet files.

Ubuntu and Debian example:

```bash
sudo apt-get update
sudo apt-get install build-essential libtool autotools-dev autoconf automake pkg-config libssl-dev libboost-all-dev
```

Berkeley DB 4.8
---------------

Wallet compatibility depends on Berkeley DB 4.8. One commonly used packaging source is the external `ppa:pivx/berkeley-db4` repository:

```bash
sudo apt-get install software-properties-common
sudo add-apt-repository ppa:pivx/berkeley-db4
sudo apt-get update
sudo apt-get install libdb4.8-dev libdb4.8++-dev
```

If you do not need compatibility with existing BDB 4.8 wallet files, you can build with a newer Berkeley DB by passing `--with-incompatible-bdb` to `./configure`.

Building with depends
---------------------

The `depends` tree can build pinned dependencies for local and cross builds:

```bash
cd depends
make -j4
cd ..
./autogen.sh
./configure --prefix="$PWD/depends/$(./depends/config.guess)"
make -j$(nproc)
```

For cross-compilation, pass a host triplet to `make`, then use the generated prefix with `./configure`. See [Windows build notes](build-windows.md) for a Windows example.

Useful configure flags
----------------------

```bash
./configure --without-gui      # daemon and CLI only
./configure --disable-wallet   # no wallet support
./configure --with-incompatible-bdb
```

Source and releases
-------------------

- Source: https://github.com/BitBlocksProject/BitBlocks
- Releases: https://github.com/BitBlocksProject/BitBlocks/releases
