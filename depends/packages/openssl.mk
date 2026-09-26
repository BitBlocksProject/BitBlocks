package=openssl
$(package)_version=3.0.16
$(package)_download_path=https://github.com/openssl/openssl/releases/download/openssl-$($(package)_version)
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=57e03c50feab5d31b152af2b764f10379aecd8ee92f16c985983ce4a99f7ef86

# src/bignum.h calls BN_check_prime(), which OpenSSL added in 3.0, so anything
# older cannot build this tree at all -- 1.0.1k could not even reach that far,
# lacking the ECDSA_SIG_get0()/set0() that src/ecwrapper.cpp needs.
#
# 3.0 keeps the 1.1 build system, so relative to 1.0.x: Makefile.org and
# util/mkbuildinf.pl that the old preprocess step patched are gone,
# INSTALL_PREFIX became DESTDIR, install_sw became install_dev, and Configure
# fails outright on an unrecognised no-* switch, so the option list below is
# deliberately short. --libdir=lib keeps it out of lib64 on x86_64 linux.
#
# Note this is still only a build dependency for hashing, RNG, wallet AES,
# base64, X.509 and RPC TLS. Consensus signature verification moved to
# libsecp256k1; see doc/secp256k1-migration.md.

define $(package)_set_vars
$(package)_config_env=AR="$($(package)_ar)" RANLIB="$($(package)_ranlib)" CC="$($(package)_cc)"
$(package)_config_opts=--prefix=$(host_prefix) --openssldir=$(host_prefix)/etc/openssl --libdir=lib
$(package)_config_opts+=no-asm
$(package)_config_opts+=no-comp
$(package)_config_opts+=no-shared
$(package)_config_opts+=no-ssl3
$(package)_config_opts+=no-tests
$(package)_config_opts+=no-weak-ssl-ciphers
$(package)_config_opts+=no-zlib
$(package)_config_opts+=$($(package)_cflags) $($(package)_cppflags)
$(package)_config_opts_linux=-fPIC
$(package)_config_opts_x86_64_linux=linux-x86_64
$(package)_config_opts_i686_linux=linux-generic32
$(package)_config_opts_arm_linux=linux-generic32
$(package)_config_opts_aarch64_linux=linux-generic64
$(package)_config_opts_x86_64_darwin=darwin64-x86_64-cc
$(package)_config_opts_x86_64_mingw32=mingw64
$(package)_config_opts_i686_mingw32=mingw
endef

define $(package)_config_cmds
  ./Configure $($(package)_config_opts)
endef

define $(package)_build_cmds
  $(MAKE) -j1 build_libs
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) -j1 install_dev
endef

define $(package)_postprocess_cmds
  rm -rf share bin etc
endef
