Consensus verifier migration: OpenSSL to libsecp256k1
=====================================================

Why
---

Signature verification for the chain runs through `CPubKey::Verify()` in
`src/pubkey.cpp`. Until this migration it called `CECKey` in
`src/ecwrapper.cpp`, which is OpenSSL. That makes a consensus rule depend on
whichever OpenSSL the binary happened to be linked against:

* the published Windows binary statically links **OpenSSL 1.0.1k**
* a build on Ubuntu 24.04 links **OpenSSL 3.0.x**

Two nodes on the same network can therefore disagree about whether a block is
valid. This is not hypothetical: the OpenSSL 1.0.0p/1.0.1k fix for
CVE-2014-8275 changed which non-canonical DER signatures were accepted and
forced an emergency Bitcoin release in 2015. The residue of that episode is
still commented in `src/ecwrapper.cpp`.

`libsecp256k1` is vendored in `src/secp256k1/`, is built from source with the
rest of the tree, and has a fixed version. Using it removes the external
dependency from the consensus path.

This is **not** a consensus rule change and has **no activation height**. The
two verifiers are intended to accept exactly the same signatures and public
keys; anything else is a bug to be resolved before shipping, not a rule to be
deployed.

Equivalence
-----------

The relevant acceptance sets were compared by reading both implementations:

* **Public key encodings.** `secp256k1_eckey_pubkey_parse()`
  (`src/secp256k1/src/eckey_impl.h`) accepts `0x02`/`0x03` compressed,
  `0x04` uncompressed, and `0x06`/`0x07` hybrid with a y-parity check. That is
  the same set `o2i_ECPublicKey()` accepts. `CPubKey::GetLen()` lets all five
  header bytes through, and `SCRIPT_VERIFY_STRICTENC` -- which would reject
  hybrids -- is a standardness flag, not a consensus one.
* **High-S.** Neither `secp256k1_ecdsa_sig_verify()` nor `ECDSA_verify()`
  rejects high-S. Low-S enforcement stays where it was, in
  `IsLowDERSignature()`.
* **DER encoding.** The two parsers differ in edge cases, but BIP66/DERSIG is
  active on mainnet, so `IsValidSignatureEncoding()` -- our own code, identical
  in both configurations -- validates the DER shape before either parser sees
  it.

The remaining question is empirical: does anything in our actual chain history
fall in a case where they differ? That is what the scan below answers.

Pre-flight scan
---------------

`src/test/verifier_scan_tests.cpp` walks the raw block files and runs both
libraries over every signature and public key it finds. It is skipped unless
a datadir is pointed at:

    BBK_BLOCKS_DIR=~/.bitblocks/blocks src/test/test_bitblocks \
        --run_test=verifier_scan_tests

Any of the following blocks the migration:

* a public key or signature the two libraries disagree about
* a hybrid (`0x06`/`0x07`) public key that is actually spent
* an off-curve key in a bare-pubkey output

Result, mainnet blocks up to 2026-09-22 (20 blk files, 2.7 GB, from a synced
Windows 1.1.3 wallet):

    blocks=4102732 txs=8907136 signatures=9394365 pubkeys=9478573
    hybrid_spent=0 hybrid_in_output=0 bad_der=5504 offcurve_p2pk=0
    pubkey_disagreements=0 sig_disagreements=0

Nothing blocks the migration. The block count exceeds the chain height because
the block files also hold orphans and side-chain blocks, which makes the scan
conservative rather than incomplete.

`bad_der=5504` is not a finding: `LooksLikeSig()` classifies any push of 8..73
bytes starting with `0x30` as a signature candidate, and these are data pushes
that merely begin with that byte. In a 200-item sample none was in the 68..73
byte range of a real ECDSA signature; sizes ran from 8 to 27 bytes. They were
fed to both parsers regardless, and both agreed on every one.

One limit worth stating: this compares libsecp256k1 against the OpenSSL on the
machine running the scan (3.0.13 here), not against the 1.0.1k statically
linked into the published Windows binary. Removing that variable is the point
of the migration; the scan cannot retire it by itself.

Patches to the vendored libsecp256k1
------------------------------------

The copy in `src/secp256k1/` is from 2014 and carries local changes. Keep this
list current; anything not listed here should be identical to upstream.

* `src/ecdsa_impl.h`, `secp256k1_ecdsa_sig_parse()`: added a `size < 6` bounds
  check. The function indexes `sig[0]`, `sig[3]` and `sig[lenr+5]` before
  consulting `size`, and `secp256k1_ecdsa_verify()` does not guard `siglen`
  either, so a short signature was read out of bounds. Such a signature does
  reach `CPubKey::Verify()`: `CheckSignatureEncoding()` only runs
  `IsValidSignatureEncoding()` when DERSIG, LOW_S or STRICTENC is set, and the
  mandatory consensus flags are P2SH only.

Known limitations of the vendored copy
--------------------------------------

* `build-aux/m4/bitcoin_secp.m4` is **missing** from this tree and from git.
  As a result `SECP_INT128_CHECK`, `SECP_64BIT_ASM_CHECK` and `SECP_GMP_CHECK`
  survive in the pre-generated `src/secp256k1/configure` as literals that do
  nothing, backend autodetection always fell through to the 32-bit field and
  scalar, and running `src/secp256k1/autogen.sh` now fails under autoconf 2.71.
  The parent `configure.ac` therefore passes `--with-field` and `--with-scalar`
  explicitly. **Do not run `src/secp256k1/autogen.sh`** without restoring that
  m4 file first.
* The exported API is the 2014 global-state one (`secp256k1_start()` /
  `secp256k1_stop()`, no context objects). `ECC_Start()` must be called before
  anything verifies; see `src/key.cpp`.

Building either verifier
------------------------

libsecp256k1 is the default. The OpenSSL path is kept compilable for
differential testing and for rollback:

    ./configure                          # libsecp256k1 (default)
    ./configure --enable-openssl-ecdsa   # the old OpenSSL verifier

OpenSSL remains a dependency regardless: it is still used for TLS in
`bitblocks-cli`, for hashing, and for `bip38`.

Acceptance criterion for a release
----------------------------------

A full reindex with both verifiers must reach the same best block:

    src/bitblocksd -reindex -checkpoints=0 -datadir=<copy A>
    src/bitblocks-cli -datadir=<copy A> getbestblockhash

`-checkpoints=0` is required. `fScriptChecks` in `ConnectBlock()` is
`pindex->nHeight >= Checkpoints::GetTotalBlocksEstimate()`, and the highest
mainnet checkpoint is 3,685,080, so without it script verification is skipped
for nearly the whole chain and the run proves nothing.

Repeat with a binary built `--enable-openssl-ecdsa` against a second copy of
the datadir and compare the two hashes. This must be done on Linux and on
Windows; the Windows half is blocked until the `depends` tree is fixed, since
it pins OpenSSL 1.0.1k while the current source needs `ECDSA_SIG_get0` from
OpenSSL 1.1.0 or newer.

Result, Linux, 2026-09-22
-------------------------

Two independent reindexes over the same block files, network disabled
(`-maxconnections=0 -dnsseed=0 -listen=0`), each from an empty chainstate:

* **A, OpenSSL verifier**, stopped at height 878,415. This binary predates the
  startup log line below, so its identity rests on being byte-identical
  (md5 `afaa3496db04cdfad29b82f69acdfc3b`) to a build made with
  `--enable-openssl-ecdsa`.
* **B, libsecp256k1**, `v1.1.3.0-886a988`, which states
  `Signature verification: libsecp256k1` in its own debug.log.

Block hashes at eight heights -- 100k, 250k, 400k, 500k, 600k, 700k, 800k and
878,415 -- are identical between the two runs. Neither log contains a
`bad-txns`, `mandatory-script-verify` or `InvalidChainFound` entry; the only
ERROR lines in either are a missing peers.dat on a fresh datadir and a
deliberately duplicated startup that the datadir lock refused.

Coverage: height 878,415 accumulates 2,093,206 of the chain's 8,907,136
transactions, so roughly 23% of all transactions were verified end to end by
both libraries. The run was truncated by choice rather than carried to the
tip. The remaining ~77% is covered by the scan above, which compared both
libraries over every signature and public key in the chain, but at the parse
level only -- it has no sighashes, so it cannot compare verification verdicts.

Closing that gap without a second full pass is possible: one reindex in which
`CPubKey::Verify()` calls both libraries and logs any disagreement gives
whole-chain coverage of the verification verdict in a single run, and names
the offending transaction instead of only reporting that two hashes differ.

Unit test suite, both verifiers
-------------------------------

The full suite was built and run twice from the same source, differing only in
the verifier -- confirmed by disassembly: `CPubKey::Verify()` calls
`secp256k1_ecdsa_verify` in one binary and `CECKey::Verify` in the other.

    libsecp256k1   216110 failures
    OpenSSL 3.0.13 216170 failures

**No failure is unique to libsecp256k1.** Every one of its 8 distinct
script/transaction failures also occurs with OpenSSL. 38 distinct failures
occur only with OpenSSL, and they are all the same shape:

    P2PK with too much / too little R padding, but no DERSIG
    P2PK with too much S padding, but no DERSIG
    BIP66 examples 1, 2, 7 and 8, without DERSIG

That is, signatures whose DER encoding is non-canonical in ways consensus
still accepts when DERSIG is not being enforced. OpenSSL 3.0 rejects them;
libsecp256k1 accepts them, which is the historically correct behavior.

This is the divergence that motivated the migration, finally measured rather
than argued: on these vectors it is the OpenSSL build that deviates, and a
node built today on a current distribution is the one at risk. It does not
contradict the chain scan -- no signature of this shape exists in our chain,
which is why the scan found zero disagreements and why the OpenSSL reindex
reached height 878,415 without rejecting a block.

The 216,002 remaining failures are `subsidy_limit_test`, the Bitcoin block
subsidy test never adapted to this fork's reward schedule, plus 85 base58 and
8 key vectors still carrying PIVX-era prefixes. They fail identically under
both verifiers and predate this work; they were simply never seen, because the
test suite did not build in this tree.
