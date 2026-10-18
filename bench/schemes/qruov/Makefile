.DEFAULT_GOAL := all

CC      ?= cc
BUILD    = build
REFDIR   = ref

# Upstream ships ref / opt / avx2; avx2 is the fastest x86-64 build and the one
# the Round 3 specification benchmarks (spec Table 5).  Round 3 dropped the
# Round 2 avx512 implementation.
IMPL            := avx2
IMPL_CFLAGS     := -mavx2 -mbmi2 -maes -DPRIM_AES_BACKEND_X86AESNI=1
# PRG for the public seed: AES-128-CTR (upstream default, spec Table 5) or SHAKE128.
PRG_CFLAGS      := -DPRG_IS_AES=1

# OpenSSL: needed for SHA3/SHAKE (EVP).  Only the shared library is required;
# prim.h falls back to a cached EVP_DigestFinalXOF when EVP_DigestSqueeze
# (OpenSSL 3.3+) is unavailable.
OSSL_PREFIX := $(shell brew --prefix openssl 2>/dev/null)
ifeq ($(OSSL_PREFIX),)
  OSSL_PREFIX := $(shell pkg-config --variable=prefix openssl 2>/dev/null)
endif
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

# Upstream's own CFLAGS are -O3 -flto=auto; -march=native matches the SUPERCOP
# methodology the spec's numbers were produced with.
CFLAGS := -O3 -flto=auto -march=native -mtune=native $(IMPL_CFLAGS) $(PRG_CFLAGS) \
          -fomit-frame-pointer -fPIC -DNDEBUG -Wall \
          -Wno-unused-parameter -Wno-unused-result \
          -I$(REFDIR)/src -I$(REFDIR)/src/$(IMPL) $(OSSL_CFLAGS)

# rng.c is excluded — the shim provides randombytes() via getrandom() instead of
# the NIST KAT DRBG.  PQCgenKAT_sign.c has a main().
SRCS := $(REFDIR)/src/sign.c $(REFDIR)/src/qruov_common.c $(REFDIR)/src/qrop.c \
        $(REFDIR)/src/prg.c $(REFDIR)/src/prim.c $(REFDIR)/src/linsys.c \
        $(REFDIR)/src/$(IMPL)/qruov.c $(REFDIR)/src/$(IMPL)/emi_transform.c \
        $(REFDIR)/src/$(IMPL)/emi_matrix.c $(REFDIR)/src/$(IMPL)/emi_transform_gen.c \
        $(REFDIR)/src/$(IMPL)/helpers.c $(REFDIR)/src/$(IMPL)/kernel.c \
        $(REFDIR)/src/$(IMPL)/x86aesni.c

# The Round 3 specification proposes six parameter sets, all with q = 127
# (spec Tables 1 and 4).  Upstream's build system still accepts the Round 2
# q = 31 / q = 7 sets, but they are no longer part of the submission.
PARAMS := qruov1q127L3v156m54  qruov1q127L10v540m60  \
          qruov3q127L3v228m78  qruov3q127L10v820m90  \
          qruov5q127L3v306m105 qruov5q127L10v1040m110

# Parameter selection is a single -D; Round 3 has no generated config headers.
pf_qruov1q127L3v156m54    := -DQRUOV_PARAM_1q127L3
pf_qruov1q127L10v540m60   := -DQRUOV_PARAM_1q127L10
pf_qruov3q127L3v228m78    := -DQRUOV_PARAM_3q127L3
pf_qruov3q127L10v820m90   := -DQRUOV_PARAM_3q127L10
pf_qruov5q127L3v306m105   := -DQRUOV_PARAM_5q127L3
pf_qruov5q127L10v1040m110 := -DQRUOV_PARAM_5q127L10

SOS        := $(addprefix $(BUILD)/, $(addsuffix .so, $(PARAMS)))
SHIM_FILES := $(addsuffix _shim.c, $(PARAMS))

.PHONY: all clean check_ref

all: check_ref $(SHIM_FILES) $(SOS)

check_ref:
	@if [ ! -f $(REFDIR)/src/sign.c ]; then \
	    echo "ERROR: bench/schemes/qruov/ref/ not populated." >&2; \
	    echo "Run: git submodule update --init bench/schemes/qruov/ref" >&2; \
	    exit 1; \
	fi

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(BUILD):
	mkdir -p $@

# Everything is compiled per parameter set straight into the .so: the parameter
# -D changes every compilation unit, so there is nothing to share between them.
$(BUILD)/%.so: %_shim.c $(SRCS) | $(BUILD)
	$(CC) $(CFLAGS) $(pf_$*) -shared $< $(SRCS) $(OSSL_LDFLAGS) -o $@

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
