include ../../../bench-common/flags.mk
.DEFAULT_GOAL := all

# QR-UOV Round 1: the NIST round-1 submission package (QR-UOV-submission.zip, files
# dated 2023-07-04), fetched into build/dl and unpacked into build/src (.source: URL on
# line 1, sha256 on line 2).  Not github.com/qruov/round1: its avx2/avx512/qruov_config.src
# equal the package, but its portable64 and ref are a later revision (SUPERCOP glue,
# new Fql_L*.h), not the submitted code.
CC      ?= cc
BUILD    = build
PKG_URL  := $(shell sed -n 1p .source)
PKG_ZIP  := $(BUILD)/dl/QR-UOV-submission.zip
PKG_DIR  := $(BUILD)/src/QR_UOV
UNZIPPED := $(BUILD)/src/.unzipped

# The package has Reference_Implementation/ref, Optimized_Implementation/portable64 and
# Alternative_Implementation/{avx2,avx512}.  We benchmark the Optimized_Implementation,
# portable64: it is the fastest in the specification (v1.0 Tables 4-6: e.g. keygen/sign
# 16.7/13.4 Mcycles for (127,156,54,3) vs 30.9/24.8 for avx2 and 92.2/78.7 for avx512),
# and with our flags it is also 2-4x faster than avx2 here.  IMPL=avx2 / IMPL=avx512 /
# IMPL=ref still build for comparison.
IMPL ?= portable64
ifeq ($(IMPL),avx512)
  SRCDIR      := $(PKG_DIR)/Alternative_Implementation
  IMPL_CFLAGS := -mavx2 -mavx512f -mavx512dq
else ifeq ($(IMPL),avx2)
  SRCDIR      := $(PKG_DIR)/Alternative_Implementation
  IMPL_CFLAGS := -mavx2
else ifeq ($(IMPL),ref)
  SRCDIR      := $(PKG_DIR)/Reference_Implementation
  IMPL_CFLAGS :=
else
  SRCDIR      := $(PKG_DIR)/Optimized_Implementation
  IMPL_CFLAGS :=
endif

# OpenSSL: upstream's mgf.c (the SHAKE-based MGF that Round 1 uses for all its
# pseudo-random expansion) reaches into OpenSSL 3.1 internals: it redeclares struct
# evp_md_ctx_st, struct evp_md_st and KECCAK1600_CTX and calls the libcrypto-internal
# 4-argument SHA3_squeeze (hence upstream's -Wl,-Bstatic -lcrypto).  OpenSSL 3.2+ changed
# all of these (KECCAK1600_CTX moved block_size behind buf, evp_md_st dropped `lock` and
# gained dsqueeze, SHA3_squeeze gained a 5th argument), so it cannot use the system
# OpenSSL 3.5.  We build a pinned OpenSSL 3.1.8 (the last 3.1 release; the three structs
# are identical to the 3.1.0 that upstream's comments name) as a static PIC libcrypto and
# link it into each .so with its symbols hidden.  OpenSSL is built with its own flags (a
# dependency, like the system libcrypto of the other OpenSSL users); it takes a few
# minutes.  .source-openssl holds the tarball URL (line 1) and sha256 (line 2).
OSSL_URL := $(shell sed -n 1p .source-openssl)
OSSL_TGZ := $(BUILD)/dl/openssl-3.1.8.tar.gz
OSSL_DIR := $(BUILD)/openssl-3.1.8
OSSL_LIB := $(OSSL_DIR)/libcrypto.a
OSSL_CFLAGS  := -I$(OSSL_DIR)/include
OSSL_LDFLAGS := $(OSSL_LIB) -Wl,--exclude-libs,ALL -lpthread

# Upstream's own CFLAGS (all four Makefiles) are -O3 -fomit-frame-pointer
# -Wno-unused-result -Wno-aggressive-loop-optimizations -I. -fopenmp (avx2/avx512 add
# -march=native); we use the global flag set (flags.mk).  -fopenmp is dropped: it
# parallelises the matrix loops over threads, and the harness measures single-threaded
# code (the pragmas are then ignored, hence -Wno-unknown-pragmas).
CFLAGS := $(IMPL_CFLAGS) -Wall -Wno-unused-result -Wno-aggressive-loop-optimizations \
          -Wno-deprecated-declarations -Wno-unused-function -Wno-unknown-pragmas \
          -I$(SRCDIR)/$(IMPL) $(OSSL_CFLAGS) $(BENCH_CFLAGS) -fPIC

# rng.c is excluded (the shim provides randombytes() via getrandom() instead of the
# NIST KAT DRBG); PQCgenKAT_sign.c has a main().
SRCS := $(addprefix $(SRCDIR)/$(IMPL)/, Fql.c mgf.c qruov.c sign.c matrix.c)

# Note: the package's qruov_config.src (all three copies) builds qruov3q7L10v1100m140 with
# -DQRUOV_v=1150 (signature 508 bytes), while the specification lists (7, 1100, 140, 10)
# with a 489-byte signature; the package KATs were generated with v = 1150.  We build the
# code as submitted.
PARAMS := qruov1q127L3v156m54  qruov1q31L3v165m60   qruov1q31L10v600m70  qruov1q7L10v740m100 \
          qruov3q127L3v228m78  qruov3q31L3v246m87   qruov3q31L10v890m100 qruov3q7L10v1100m140 \
          qruov5q127L3v306m105 qruov5q31L3v324m114  qruov5q31L10v1120m120 qruov5q7L10v1490m190

SOS        := $(addprefix $(BUILD)/, $(addsuffix .so, $(PARAMS)))
SHIM_FILES := $(addsuffix _shim.c, $(PARAMS))

.PHONY: all clean distclean

all: $(SHIM_FILES) $(SOS)

$(SHIM_FILES) &: shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(PKG_ZIP): .source
	mkdir -p $(@D)
	curl -fsSL --retry 3 -o $@.tmp '$(PKG_URL)'
	cd $(@D) && sed -n 2p ../../.source | sed 's/\.zip$$/.zip.tmp/' | sha256sum -c - \
	    || { echo "SHA-256 mismatch for $@" >&2; rm -f $(@F).tmp; exit 1; }
	mv $@.tmp $@

# Unpack only the implementations (the zip also holds ~200 MB of KATs); the top-level
# directory "QR_UOV - no IVs/QR_UOV" is renamed to build/src/QR_UOV (no spaces).
$(UNZIPPED): $(PKG_ZIP)
	rm -rf $(BUILD)/src
	mkdir -p $(BUILD)/src
	python3 -c 'import sys, zipfile; z = zipfile.ZipFile(sys.argv[1]); \
	    z.extractall(sys.argv[2], [n for n in z.namelist() \
	        if "_Implementation/" in n and not n.startswith("__MACOSX")])' \
	    $(PKG_ZIP) $(BUILD)/src
	mv "$(BUILD)/src/QR_UOV - no IVs/QR_UOV" $(PKG_DIR)
	rmdir "$(BUILD)/src/QR_UOV - no IVs"
	touch $@

$(OSSL_TGZ): .source-openssl
	mkdir -p $(@D)
	curl -fsSL --retry 3 -o $@.tmp '$(OSSL_URL)'
	cd $(@D) && sed -n 2p ../../.source-openssl | sed 's/\.tar\.gz$$/.tar.gz.tmp/' | sha256sum -c - \
	    || { echo "SHA-256 mismatch for $@" >&2; rm -f $(@F).tmp; exit 1; }
	mv $@.tmp $@

$(OSSL_LIB): $(OSSL_TGZ)
	rm -rf $(OSSL_DIR)
	tar -C $(BUILD) -xzf $(OSSL_TGZ)
	cd $(OSSL_DIR) && ./Configure linux-x86_64 no-shared no-tests no-module -fPIC \
	    CC='$(CC)' >/dev/null
	$(MAKE) -C $(OSSL_DIR) build_generated >/dev/null
	$(MAKE) -C $(OSSL_DIR) libcrypto.a >/dev/null 2>&1

# Per-parameter-set headers, generated as upstream's Makefile does: qruov_config.h from
# the set's line of qruov_config.src, then api.h (sizes) from it.
$(BUILD)/$(IMPL)/cfg/%/qruov_config.h: $(UNZIPPED)
	mkdir -p $(@D)
	$(CC) $$(grep -e '-DQRUOV_dir=$* ' $(SRCDIR)/qruov_config.src) -DQRUOV_PLATFORM=$(IMPL) \
	    -DQRUOV_CONFIG_H_GEN -o $(@D)/config_gen $(SRCDIR)/$(IMPL)/qruov_config_h_gen.c
	$(@D)/config_gen > $@.tmp && mv $@.tmp $@

$(BUILD)/$(IMPL)/cfg/%/api.h: $(BUILD)/$(IMPL)/cfg/%/qruov_config.h $(OSSL_LIB)
	$(CC) $(IMPL_CFLAGS) -DAPI_H_GEN -I$(@D) -I$(SRCDIR)/$(IMPL) $(OSSL_CFLAGS) \
	    -o $(@D)/api_gen $(SRCDIR)/$(IMPL)/api_h_gen.c
	$(@D)/api_gen > $@.tmp && mv $@.tmp $@

$(BUILD)/%.so: %_shim.c $(UNZIPPED) $(BUILD)/$(IMPL)/cfg/%/api.h $(OSSL_LIB)
	$(CC) $(CFLAGS) -I$(BUILD)/$(IMPL)/cfg/$* -shared $< $(SRCS) $(OSSL_LDFLAGS) -lm -o $@

.PRECIOUS: $(BUILD)/$(IMPL)/cfg/%/qruov_config.h $(BUILD)/$(IMPL)/cfg/%/api.h

# clean keeps build/dl (the container/VM build may have no network) and the pinned
# OpenSSL build; distclean removes them too.
clean:
	rm -rf $(BUILD)/src $(BUILD)/avx2 $(BUILD)/avx512 $(BUILD)/portable64 $(BUILD)/ref $(BUILD)/*.so
	rm -f $(SHIM_FILES)

distclean: clean
	rm -rf $(BUILD)
