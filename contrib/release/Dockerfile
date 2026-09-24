# Release build environment for Linux and Windows.
#
# Like PIVX's gitian/guix builds, release binaries are compiled on an old,
# pinned distribution. Linux binaries then only require that distribution's
# glibc (2.31 on Ubuntu 20.04) and run on Ubuntu 20.04+, Debian 11+ and newer;
# Windows binaries are cross-compiled with its MinGW-w64 toolchain.
FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive \
    LC_ALL=C.UTF-8 \
    TZ=UTC

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        autoconf \
        automake \
        autotools-dev \
        binutils \
        bsdmainutils \
        bzip2 \
        ca-certificates \
        curl \
        file \
        g++ \
        g++-mingw-w64-x86-64 \
        gcc \
        git \
        libtool \
        make \
        nsis \
        patch \
        perl \
        pkg-config \
        python3 \
        xz-utils \
        zip && \
    rm -rf /var/lib/apt/lists/* && \
    # C++11 threading needs the posix MinGW variant (as in PIVX gitian-win).
    update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix && \
    update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix

WORKDIR /work
