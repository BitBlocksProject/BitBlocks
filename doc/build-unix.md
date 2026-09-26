Unix Build Notes
================

These notes cover building BitBlocks Core on Linux and other Unix-like systems. Wallet-compatible builds should use Berkeley DB 4.8.

Dependencies
------------

Required host packages on Ubuntu/Debian:

```bash
sudo apt-get update
sudo apt-get install git build-essential libtool autotools-dev autoconf automake pkg-config bsdmainutils     curl ca-certificates libssl-dev libboost-all-dev
```

Optional packages:

```bash
sudo apt-get install libminiupnpc-dev libqrencode-dev protobuf-compiler libprotobuf-dev qtbase5-dev qttools5-dev qttools5-dev-tools
```

Berkeley DB 4.8
---------------

The desktop wallet currently uses Berkeley DB 4.8. To keep `wallet.dat` portable between desktop and Ubuntu builds, build Berkeley DB 4.8 through the repository `depends` system instead of using distribution Berkeley DB 5.x packages.

The `depends` package is `db-4.8.30.NC` and builds `libdb_cxx-4.8.a`.

Clean Ubuntu VPS daemon build
-----------------------------

On a fresh Ubuntu VPS, install the headless build dependencies:

```bash
sudo apt-get update
sudo apt-get install git build-essential libtool autotools-dev autoconf automake pkg-config bsdmainutils     curl ca-certificates libssl-dev libboost-all-dev     libminiupnpc-dev protobuf-compiler libprotobuf-dev
```

Clone and build Berkeley DB 4.8 with the rest of the pinned dependencies:

```bash
git clone https://github.com/BitBlocksProject/BitBlocks.git
cd BitBlocks
cd depends
make -j$(nproc)
cd ..
```

Build the daemon:

```bash
./autogen.sh
./configure --prefix="$PWD/depends/x86_64-linux" --without-gui
make -j$(nproc)
sudo make install
```

If your `depends/` directory was created with another host name, replace `x86_64-linux` with the generated directory name.

Create a minimal configuration:

```bash
mkdir -p ~/.bitblocks
cat > ~/.bitblocks/bitblocks.conf <<'EOF'
rpcuser=bitblocks_rpc
rpcpassword=change_this_to_a_long_random_password
rpcport=59768
port=58697
server=1
daemon=1
listen=1
EOF
chmod 600 ~/.bitblocks/bitblocks.conf
```

Start and inspect the node:

```bash
bitblocksd -daemon
grep "Using BerkeleyDB version" ~/.bitblocks/debug.log
bitblocks-cli getblockchaininfo
bitblocks-cli getnetworkinfo
```

Build
-----

If Berkeley DB 4.8 is already available through your chosen prefix:

```bash
./autogen.sh
./configure --prefix="$PWD/depends/x86_64-linux"
make -j$(nproc)
make check
make install # optional
```

Daemon-only build:

```bash
./configure --prefix="$PWD/depends/x86_64-linux" --without-gui
make -j$(nproc)
```

Useful targets
--------------

```bash
make check              # unit tests
src/test/test_bitblocks # direct unit test binary
```

Output binaries
---------------

- `src/bitblocksd`
- `src/bitblocks-cli`
- `src/bitblocks-tx`
- `src/qt/bitblocks-qt` when Qt is enabled

Linux release tarball
---------------------

Like PIVX's gitian/guix builds, release binaries are compiled inside a pinned
Ubuntu 20.04 container so they only require glibc 2.31 and run on Ubuntu
20.04+, Debian 11+ and other current distributions. Building directly on a
newer host (for example Ubuntu 24.04) would raise that requirement to the
host's glibc. With Docker installed, run:

```bash
./contrib/release/docker-release.sh linux
```

Without arguments the script builds both the Linux and the Windows release
(see [build-windows.md](build-windows.md)).

The script builds all pinned `depends`, compiles optimized binaries, strips
symbols, audits runtime linkage and the maximum glibc symbol version, and
writes `release/bitblocks-<version>-<host>.tar.gz` plus a matching `.sha256`
file. Depends builds are cached in the `bitblocks-release-depends` Docker
volume and downloaded sources in `depends/sources`. The version string is
taken from git: a clean checkout of a tag produces `v<tag>`, otherwise the
commit hash (or `MAIN` for uncommitted changes).

The archive contains the daemon, command-line tools, and Qt wallet. Boost,
Berkeley DB, OpenSSL, libevent, ZeroMQ, Qt, protobuf, QRencode and libstdc++
are linked into the binaries. As with PIVX Linux releases, glibc and standard
desktop libraries remain supplied by the distribution; on minimal systems the
Qt wallet needs:

```bash
sudo apt-get install libfontconfig1 libx11-xcb1 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-randr0 libxcb-render-util0 libxcb-xinerama0 libxcb-xkb1 libxkbcommon-x11-0
```

`contrib/release/package.sh` does the same build directly on the current
machine (useful for testing, but the result requires the host's glibc). Set
`JOBS` to limit parallel compilation, `HOST` to select another target
supported by `depends`, or `MAX_GLIBC` to change the allowed glibc version.

Runtime defaults
----------------

- Mainnet P2P port: `58697`
- Mainnet RPC port: `59768`
- Data directory: `~/.bitblocks`

Troubleshooting
---------------

- Always use absolute paths when passing custom dependency prefixes to `./configure`.
- If `configure` cannot find Berkeley DB, verify that `depends/` was built and that `--prefix` points to the generated host directory.
- If `configure` reports a Berkeley DB version other than 4.8, you are using a system library instead of the `depends` build.
- If Qt is not detected, build without the GUI using `--without-gui` or install the Qt development packages.
