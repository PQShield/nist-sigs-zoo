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

BASE_CFLAGS := -O3 -fPIC -DNDEBUG -Wall -Wno-unused-parameter -Wno-sign-compare \
               -mavx2 -maes \
               -D_BLAS_AVX2_ -D_MUL_WITH_MULTAB_ \
               $(OSSL_CFLAGS)

INC := -I$(SRC_DIR) -I$(SRC_DIR)/ref -I$(SRC_DIR)/amd64 \
       -I$(SRC_DIR)/ssse3 -I$(SRC_DIR)/avx2 \
       -I$(UTIL_DIR) -I$(UTIL_DIR)/x86aesni

# AVX2 source set. utils_randombytes.c excluded (shim provides randombytes).
# aes128_4r_ffs.c excluded (config.h auto-selects AESNI when _BLAS_AVX2_ is set).
# fips202.c excluded (config.h auto-selects _UTILS_OPENSSL_ for hash).
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
    $(UTIL_DIR)/x86aesni/x86aesni.c

PARAMS := \
    uov_Ip_pkc uov_Ip_classic \
    uov_Is_pkc uov_Is_classic \
    uov_III_pkc uov_III_classic \
    uov_V_pkc uov_V_classic

SOS        := $(addprefix $(BUILD)/, $(addsuffix .so, $(PARAMS)))
SHIM_FILES := $(addsuffix _shim.c, $(PARAMS))

pf_uov_Ip_pkc      := -D_OV256_112_44 -D_OV_PKC
pf_uov_Ip_classic  := -D_OV256_112_44 -D_OV_CLASSIC
pf_uov_Is_pkc      := -D_OV16_160_64  -D_OV_PKC
pf_uov_Is_classic  := -D_OV16_160_64  -D_OV_CLASSIC
pf_uov_III_pkc     := -D_OV256_184_72 -D_OV_PKC
pf_uov_III_classic := -D_OV256_184_72 -D_OV_CLASSIC
pf_uov_V_pkc       := -D_OV256_244_96 -D_OV_PKC
pf_uov_V_classic   := -D_OV256_244_96 -D_OV_CLASSIC

.PHONY: all clean check_ref

all: check_ref $(SHIM_FILES) $(SOS)

check_ref:
	@if [ ! -f $(SRC_DIR)/sign.c ]; then \
	    echo "ERROR: bench/schemes/uov/ref/ not populated." >&2; \
	    echo "Run: git submodule update --init bench/schemes/uov/ref" >&2; \
	    exit 1; \
	fi

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(BUILD):
	mkdir -p $@

# ---------------------------------------------------------------------------
# Rule template — instantiated once per param set via $(eval $(call ...))
# $(1) = param name (e.g. uov_Ip_pkc)
# ---------------------------------------------------------------------------

define UOV_RULES

cflags_$(1) := $(BASE_CFLAGS) $(pf_$(1)) $(INC)
objs_$(1)   := $(patsubst ref/%.c,$(BUILD)/$(1)/%.o,$(SRCS))
lib_$(1)    := $(BUILD)/$(1)/libuov.a

# Create output directory (and all needed subdirs) per object.
$$(objs_$(1)): $(BUILD)/$(1)/%.o : ref/%.c
	mkdir -p $$(dir $$@)
	$$(CC) $$(cflags_$(1)) -c $$< -o $$@

$$(lib_$(1)): $$(objs_$(1))
	ar rcs $$@ $$^

$(BUILD)/$(1).so: $(1)_shim.c $$(lib_$(1)) | $(BUILD)
	$$(CC) $$(cflags_$(1)) -shared $$< $$(lib_$(1)) $(OSSL_LDFLAGS) -o $$@

endef

$(foreach P,$(PARAMS),$(eval $(call UOV_RULES,$(P))))

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
