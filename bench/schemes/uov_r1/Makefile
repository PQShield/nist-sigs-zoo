# UOV Round 1 (NIST on-ramp round-1 submission, YAML version 'v1.0').
# ref/ = pqov/pqov @ 95aa9264d371213586bc6e29718a1636b02eef74 (2023-05-24, 'update
#   Makefiles for files in the nist submission'; src/ and utils/ equal the round-1
#   NIST package's Optimized_Implementation/avx2 except the per-set params.h/config.h
#   defaults, which the -D flags below set).
include ../../../bench-common/flags.mk
CC      ?= cc
BUILD    = build
SRC_DIR  = ref/src
UTIL_DIR = ref/utils

# OpenSSL: config.h auto-enables _UTILS_OPENSSL_ for SHAKE; link against it.
OSSL_PREFIX := $(shell brew --prefix openssl 2>/dev/null)
ifeq ($(OSSL_PREFIX),)
  OSSL_PREFIX := $(shell pkg-config --variable=prefix openssl 2>/dev/null)
endif
ifeq ($(OSSL_PREFIX),)
  OSSL_PREFIX := /usr
endif
OSSL_CFLAGS  = -I$(OSSL_PREFIX)/include
OSSL_LDFLAGS = -L$(OSSL_PREFIX)/lib -lcrypto -Wl,-rpath,$(OSSL_PREFIX)/lib

# Upstream's PROJ=avx2, the fastest x86 implementation of this version (no gfni yet;
# its Makefile's default PROJ is ref). Flags as upstream's avx2 branch.
PROJ_CFLAGS   := -mavx2 -maes -D_BLAS_AVX2_ -D_MUL_WITH_MULTAB_ -D_UTILS_AESNI_
MULTAB_GF16   :=
PROJ_INC      :=
PROJ_SRCS     :=

WARN_CFLAGS := -Wall -Wno-unused-parameter -Wno-sign-compare

INC := -I$(SRC_DIR) -I$(SRC_DIR)/ref -I$(SRC_DIR)/amd64 \
       -I$(SRC_DIR)/ssse3 -I$(SRC_DIR)/avx2 $(PROJ_INC) \
       -I$(UTIL_DIR) -I$(UTIL_DIR)/x86aesni

# This version has no namespacing and only the old combined NIST API; rename its
# API symbols in upstream's objects (not the shim) so the shim can export the plain
# bench names. The shim wraps uov_crypto_sign / uov_crypto_sign_open.
RENAME := -Dcrypto_sign_keypair=uov_crypto_sign_keypair \
          -Dcrypto_sign=uov_crypto_sign -Dcrypto_sign_open=uov_crypto_sign_open

# Upstream's PROJ source set (src/{ref,amd64,ssse3,avx2}, utils/x86aesni,
# src/*.c, utils/*.c), minus: utils_randombytes.c (the shim provides randombytes),
# aes128_4r_ffs.c (config.h selects AES-NI when _BLAS_AVX2_ is set) and fips202.c
# (config.h selects _UTILS_OPENSSL_ for the hash).
SRCS := \
    $(SRC_DIR)/blas_matrix.c \
    $(SRC_DIR)/ov.c \
    $(SRC_DIR)/ov_keypair.c \
    $(SRC_DIR)/ov_keypair_computation.c \
    $(SRC_DIR)/ov_publicmap.c \
    $(SRC_DIR)/parallel_matrix_op.c \
    $(SRC_DIR)/sign.c \
    $(SRC_DIR)/ref/blas_matrix_ref.c \
    $(SRC_DIR)/ssse3/blas_matrix_sse.c \
    $(SRC_DIR)/ssse3/gf16_tabs.c \
    $(SRC_DIR)/avx2/blas_matrix_avx2.c \
    $(SRC_DIR)/avx2/gf256_tabs.c \
    $(UTIL_DIR)/utils.c \
    $(UTIL_DIR)/utils_hash.c \
    $(UTIL_DIR)/utils_prng.c \
    $(UTIL_DIR)/x86aesni/x86aesni.c \
    $(PROJ_SRCS)

P := uov_r1
PARAMS := \
    $(P)_Ip_pkc $(P)_Ip_classic \
    $(P)_Is_pkc $(P)_Is_classic \
    $(P)_III_pkc $(P)_III_classic \
    $(P)_V_pkc $(P)_V_classic

SOS        := $(addprefix $(BUILD)/, $(addsuffix .so, $(PARAMS)))
SHIM_FILES := $(addsuffix _shim.c, $(PARAMS))

# Parameter sets of this version (upstream PARAM=3/1/4/5); variant pkc = VARIANT=2,
# classic = VARIANT=1.
pf_$(P)_Ip_pkc      := -D_OV256_112_44 -D_OV_PKC
pf_$(P)_Ip_classic  := -D_OV256_112_44 -D_OV_CLASSIC
pf_$(P)_Is_pkc      := -D_OV16_160_64  -D_OV_PKC     $(MULTAB_GF16)
pf_$(P)_Is_classic  := -D_OV16_160_64  -D_OV_CLASSIC $(MULTAB_GF16)
pf_$(P)_III_pkc     := -D_OV256_184_72 -D_OV_PKC
pf_$(P)_III_classic := -D_OV256_184_72 -D_OV_CLASSIC
pf_$(P)_V_pkc       := -D_OV256_244_96 -D_OV_PKC
pf_$(P)_V_classic   := -D_OV256_244_96 -D_OV_CLASSIC

.PHONY: all clean check_ref

all: check_ref $(SHIM_FILES) $(SOS)

check_ref:
	@if [ ! -f $(SRC_DIR)/sign.c ]; then \
	    echo "ERROR: bench/schemes/uov_r1/ref/ not populated." >&2; \
	    echo "Run: git submodule update --init bench/schemes/uov_r1/ref" >&2; \
	    exit 1; \
	fi

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(BUILD):
	mkdir -p $@

# $(1) = param name (e.g. uov_r1_Ip_pkc)
define UOV_RULES

cflags_$(1) := $(WARN_CFLAGS) $(PROJ_CFLAGS) $(pf_$(1)) $(INC) $(OSSL_CFLAGS)
objs_$(1)   := $(patsubst ref/%.c,$(BUILD)/$(1)/%.o,$(SRCS))
lib_$(1)    := $(BUILD)/$(1)/libuov.a

$$(objs_$(1)): $(BUILD)/$(1)/%.o : ref/%.c
	mkdir -p $$(dir $$@)
	$$(CC) $$(cflags_$(1)) $(RENAME) $(BENCH_CFLAGS) -fPIC -c $$< -o $$@

$$(lib_$(1)): $$(objs_$(1))
	ar rcs $$@ $$^

$(BUILD)/$(1).so: $(1)_shim.c $$(lib_$(1)) | $(BUILD)
	$$(CC) $$(cflags_$(1)) $(BENCH_CFLAGS) -fPIC -shared $$< $$(lib_$(1)) $(OSSL_LDFLAGS) -o $$@

endef

$(foreach P_,$(PARAMS),$(eval $(call UOV_RULES,$(P_))))

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
