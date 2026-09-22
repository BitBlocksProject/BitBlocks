package=openssl
$(package)_version=1.1.1w
$(package)_download_path=https://www.openssl.org/source
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=cf3098950cb4d853ad95c0841f1f9c6d3dc102dccfcacd521d93925208b76ac8

# 1.0.1k went end-of-life in 2016 and, more immediately, does not have
# ECDSA_SIG_get0()/ECDSA_SIG_set0(), which src/ecwrapper.cpp has required since
# the source moved to the OpenSSL 1.1 API. The Windows cross build could not
# compile at all while this was pinned there.
#
# 1.1.0 replaced the build system, so this is not a version bump: Makefile.org
# and util/mkbuildinf.pl are gone (the old preprocess step patched both),
# INSTALL_PREFIX became DESTDIR, install_sw became install_dev, and many of the
# old no-* switches no longer exist -- Configure fails outright on an option it
# does not recognise, so the list below is deliberately short.

define $(package)_set_vars
$(package)_config_env=AR="$($(package)_ar)" RANLIB="$($(package)_ranlib)" CC="$($(package)_cc)"
$(package)_config_opts=--prefix=$(host_prefix) --openssldir=$(host_prefix)/etc/openssl
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
