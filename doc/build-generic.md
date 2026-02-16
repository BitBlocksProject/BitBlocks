GENERIC BUILD NOTES
====================
Some notes on how to build BitBlocks Core. You can either use system-installed dependencies (recommended on Ubuntu/Debian)
or the [depends](../depends/README.md) build system.

Required build tools and environment
------------------------------------
Building BitBlocks Core requires some essential build tools to be installed before. Please see
[build-unix](build-unix.md), [build-osx](build-osx.md) and [build-windows](build-windows.md) for details.

Building with system dependencies (Ubuntu/Debian)
------------------------------------------------
On Ubuntu and Debian you can install dependencies from packages. Berkeley DB 4.8 (required for the wallet) is provided
by the PIVX PPA, which supports current LTS releases including Ubuntu Noble (24.04):

```bash
sudo apt-get install software-properties-common
sudo add-apt-repository ppa:pivx/berkeley-db4
sudo apt-get update
sudo apt-get install libdb4.8-dev libdb4.8++-dev
```

Install the remaining build and library dependencies as described in [build-unix](build-unix.md), then run:

```bash
$ ./autogen.sh
$ ./configure
$ make -j$(nproc)
$ make install # optional
```

Building with the depends system
--------------------------------
BitBlocks inherited the `depends` folder from Bitcoin. If your system does not provide suitable packages (e.g. for
cross-compilation or older distros), you can build dependencies from source. These must be built before BitBlocks:

```bash
$ cd depends
$ make -j4 # Choose a good -j value, depending on the number of CPU cores available
$ cd ..
```

This will download and build all dependencies required to build BitBlocks Core. Please read the
[depends](../depends/README.md) documentation for supported hosts and configuration options. If no host is specified,
the depends system will default to your local host system.

Then build BitBlocks Core with:

```bash
$ ./autogen.sh
$ ./configure --prefix=`pwd`/depends/<host>
$ make
$ make install # optional
```

Please replace `<host>` with your local system's `host-platform-triplet`. The following triplets are usually valid:
- `i686-pc-linux-gnu` for Linux32
- `x86_64-pc-linux-gnu` for Linux64
- `x86_64-w64-mingw32` for Win64
- `x86_64-apple-darwin19` for macOS
- `arm-linux-gnueabihf` for Linux ARM 32 bit
- `aarch64-linux-gnu` for Linux ARM 64 bit

If you want to cross-compile for another platform, choose the appropriate `<host>` and make sure to build the
dependencies with the same host before.

If you want to build for the same host but different distro, add `--enable-glibc-back-compat LDFLAGS=-static-libstdc++` when calling `./configure`.


ccache
------
`./configure` of BitBlocks Core will autodetect the presence of ccache and enable use of it. To disable ccache, use
`./configure --prefix=<prefix> --disable-ccache`. When installed and enabled, [ccache](https://ccache.samba.org/) will
cache build results on source->object level.

The default maximum cache size is 5G, which might not be enough to cache multiple builds when switching Git branches
very often. It is advised to increase the maximum cache size:

```bash
$ ccache -M20G
```

Additional Configure Flags
--------------------------
A list of additional configure flags can be displayed with:

```bash
./configure --help
```
