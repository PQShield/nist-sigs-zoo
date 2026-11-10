include ../../../bench-common/flags.mk
.DEFAULT_GOAL := all

# QR-UOV Round 2: github.com/qruov/round2 (README: released 2024-12-13).  Its src/ equals
# the NIST round-2 package qr-uov-submission-round2.zip except the License files; the
# later commits only add LICENSE.txt.  Same twelve parameter sets as the specification.
CC      ?= cc
BUILD    = build
REFDIR   = ref
SRCDIR   = $(REFDIR)/src

# Upstream ships ref, portable64 (the package's Optimized_Implementation) and avx2 /
# avx512 (Alternative_Implementation, default avx2).  The specification (v2.0 Tables 6,
# 10, 12, AES PRG) has avx2 and avx512 about equal and both faster than portable64
# (keygen 8.6 / 8.5 vs 21.5 Mcycles for (127,156,54,3)); avx512 is slightly ahead on the
# large sets.  So: avx512 (AVX-512BW + VNNI vpdpbusd) when the CPU has it, else avx2.
HAVE_AVX512 := $(shell python3 ../../cpu_features.py avx512f avx512bw avx512_vnni)
ifeq ($(HAVE_AVX512),1)
  IMPL        := avx512
  IMPL_CFLAGS := -mavx2 -mavx512f -mavx512bw -mavx512vnni
else
  IMPL        := avx2
  IMPL_CFLAGS := -mavx2
endif
# PRG for the public seed: AES-CTR (upstream default; the "a" builds of its Makefile)
# rather than SHAKE (-DQRUOV_PRG_SHAKE).

# OpenSSL (system libcrypto, as the Round 3 dir): AES-CTR and SHAKE via EVP.
# mgf.c is not compiled: it reaches into OpenSSL 3.1 internals (struct evp_md_ctx_st,
# KECCAK1600_CTX, the libcrypto-internal SHA3_squeeze; hence upstream's
# -Wl,-Bstatic -lcrypto), which OpenSSL 3.2+ changed, but in Round 2 its MGF_* functions
# are only reached through the unused static PRG_*_mgf helpers of qruov.c, so it is dead
# code.  (Round 1 does use it; see ../qruov_r1/Makefile.)
OSSL_PREFIX := $(shell pkg-config --variable=prefix openssl 2>/dev/null)
ifeq ($(OSSL_PREFIX),)
  OSSL_PREFIX := /usr
endif
# Debian/Ubuntu keep libcrypto in a multiarch dir, not $(OSSL_PREFIX)/lib.
OSSL_LIBDIR := $(shell pkg-config --variable=libdir openssl 2>/dev/null)
ifeq ($(OSSL_LIBDIR),)
  OSSL_LIBDIR := $(OSSL_PREFIX)/lib
endif
OSSL_CFLAGS  := -I$(OSSL_PREFIX)/include
OSSL_LDFLAGS := -L$(OSSL_LIBDIR) -lcrypto -Wl,-rpath,$(OSSL_LIBDIR)

# Upstream's own CFLAGS are -march=native -mtune=native -O3 -fomit-frame-pointer -fwrapv
# -fPIC -fPIE -fopenmp; we use the global flag set (flags.mk).  Exception: -fwrapv is kept
# because upstream builds (and its KATs were generated) with wrapping signed arithmetic.
# -fopenmp is dropped: the Round 2 avx2/avx512 code has no OpenMP pragmas.
CFLAGS := $(IMPL_CFLAGS) -fwrapv -Wall -Wno-deprecated-declarations -Wno-unused-result \
          -Wno-unused-parameter -Wno-unused-function \
          -I$(SRCDIR)/$(IMPL) $(OSSL_CFLAGS) $(BENCH_CFLAGS) -fPIC

# rng.c is excluded (the shim provides randombytes() via getrandom() instead of the
# NIST KAT DRBG); PQCgenKAT_sign.c has a main(); mgf.c see above.
SRCS := $(addprefix $(SRCDIR)/$(IMPL)/, Fql.c qruov.c sign.c matrix.c)

PARAMS := qruov1q127L3v156m54  qruov1q31L3v165m60   qruov1q31L10v600m70  qruov1q7L10v740m100 \
          qruov3q127L3v228m78  qruov3q31L3v246m87   qruov3q31L10v890m100 qruov3q7L10v1100m140 \
          qruov5q127L3v306m105 qruov5q31L3v324m114  qruov5q31L10v1120m120 qruov5q7L10v1490m190

SOS        := $(addprefix $(BUILD)/, $(addsuffix .so, $(PARAMS)))
SHIM_FILES := $(addsuffix _shim.c, $(PARAMS))

.PHONY: all clean check_ref

all: check_ref $(SHIM_FILES) $(SOS)

check_ref:
	@if [ ! -f $(SRCDIR)/$(IMPL)/sign.c ]; then \
	    echo "ERROR: bench/schemes/qruov_r2/ref/ not populated." >&2; \
	    echo "Run: git submodule update --init bench/schemes/qruov_r2/ref" >&2; \
	    exit 1; \
	fi

$(SHIM_FILES) &: shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

# Per-parameter-set headers, generated as upstream's Makefile does: qruov_config.h from
# the set's line of qruov_config.src, then api.h (sizes) from it.
$(BUILD)/cfg/%/qruov_config.h: $(SRCDIR)/qruov_config.src $(SRCDIR)/$(IMPL)/qruov_config_h_gen.c
	mkdir -p $(@D)
	$(CC) $$(grep -e '-DQRUOV_dir=$* ' $(SRCDIR)/qruov_config.src) -DQRUOV_PLATFORM=$(IMPL) \
	    -DQRUOV_CONFIG_H_GEN -o $(@D)/config_gen $(SRCDIR)/$(IMPL)/qruov_config_h_gen.c
	$(@D)/config_gen > $@.tmp && mv $@.tmp $@

$(BUILD)/cfg/%/api.h: $(BUILD)/cfg/%/qruov_config.h $(SRCDIR)/$(IMPL)/api_h_gen.c
	$(CC) $(IMPL_CFLAGS) -DAPI_H_GEN -I$(@D) -I$(SRCDIR)/$(IMPL) $(OSSL_CFLAGS) \
	    -o $(@D)/api_gen $(SRCDIR)/$(IMPL)/api_h_gen.c
	$(@D)/api_gen > $@.tmp && mv $@.tmp $@

$(BUILD)/%.so: %_shim.c $(SRCS) $(BUILD)/cfg/%/api.h
	$(CC) $(CFLAGS) -I$(BUILD)/cfg/$* -shared $< $(SRCS) $(OSSL_LDFLAGS) -lm -o $@

.PRECIOUS: $(BUILD)/cfg/%/qruov_config.h $(BUILD)/cfg/%/api.h

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
