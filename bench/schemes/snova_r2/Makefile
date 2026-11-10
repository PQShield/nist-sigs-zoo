.DEFAULT_GOAL := all
include ../../../bench-common/flags.mk
CC      ?= cc
BUILD    = build
SRC      = ref/src

# SNOVA Round 2 (v2.0), PQCLAB-SNOVA/SNOVA commit 1318290 "NIST Round2 version"; its src/
# is identical to the NIST round-2 package (Optimized_Implementation/src and
# Alternative_Implementation/avx2/src are the same code, only the platform differs).
#
# Variant: ESK (expanded secret key, sk_is_seed=0) with AES-CTR public-key expansion
# (PK_EXPAND_SHAKE=0), the defaults of upstream's src/makefile, and the variant of the
# spec's Table 7 ("Sign ESK", AES rows), which data/schemes/SNOVA.yaml v2.0 quotes.
# OPTIMISATION=2 is the AVX2 implementation, src/makefile's default on an AVX2 machine
# (and the platform of the spec's benchmarks); FIXED_ABQ=2 is upstream's default.
BASE_CFLAGS := -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare \
               -DPK_EXPAND_SHAKE=0 -DOPTIMISATION=2 -DFIXED_ABQ=2 \
               $(BENCH_CFLAGS) -fPIC

# v o l (q = 16).
PARAMS := \
    snova_r2_37_17_2 snova_r2_25_8_3 snova_r2_24_5_4 \
    snova_r2_56_25_2 snova_r2_49_11_3 snova_r2_37_8_4 snova_r2_24_5_5 \
    snova_r2_75_33_2 snova_r2_66_15_3 snova_r2_60_10_4 snova_r2_29_6_5

SOS        := $(addprefix $(BUILD)/, $(addsuffix .so, $(PARAMS)))
SHIM_FILES := $(addsuffix _shim.c, $(PARAMS))

# Per-set flags from the name: snova_r2_<v>_<o>_<l>.
snova_w = $(subst _, ,$(1))
pf = -Dv_SNOVA=$(word 3,$(call snova_w,$(1))) -Do_SNOVA=$(word 4,$(call snova_w,$(1))) \
     -Dl_SNOVA=$(word 5,$(call snova_w,$(1))) -Dsk_is_seed=0 \
     -DCRYPTO_ALGNAME='"SNOVA_$(word 3,$(call snova_w,$(1)))_$(word 4,$(call snova_w,$(1)))_$(word 5,$(call snova_w,$(1)))"'

# Symmetric library, as upstream's src/makefile builds it on an AVX2 machine (rng.c
# excluded: the shim provides randombytes via getrandom()). Upstream adds the 8-way
# Keccak only when -march=native has AVX-512; snova_shake_opt.c picks it via __AVX512F__.
# Here it is always in the static library, compiled with -mavx512f so it builds anywhere;
# the linker only pulls it in when snova_shake_opt.c references it, i.e. on AVX-512.
SYM_SRCS := shake/KeccakHash.c shake/SimpleFIPS202.c shake/KeccakP-1600-opt64.c \
            shake/KeccakSponge.c shake/snova_shake_ref.c shake/snova_shake_opt.c \
            aes/aes_c.c aes/snova_aes.c \
            shake/KeccakP-1600-times4-SIMD256.c aes/aes128_ni.c aes/aes256_ni.c \
            shake/KeccakP-1600-times8-SIMD512.c
SYM_OBJS := $(patsubst %.c,$(BUILD)/sym/%.o,$(SYM_SRCS))
SYM_LIB  := $(BUILD)/sym/libsnovasym.a
$(BUILD)/sym/shake/KeccakP-1600-times8-SIMD512.o: XFLAGS := -mavx512f

.PHONY: all clean check_ref

all: check_ref $(SHIM_FILES) $(SOS)

check_ref:
	@if [ ! -f $(SRC)/snova.h ]; then \
	    echo "ERROR: bench/schemes/snova_r2/ref/ not populated." >&2; \
	    echo "Run: git submodule update --init bench/schemes/snova_r2/ref" >&2; \
	    exit 1; \
	fi

$(SHIM_FILES) &: shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(SYM_OBJS): $(BUILD)/sym/%.o: $(SRC)/%.c
	@mkdir -p $(@D)
	$(CC) -Wall -Wextra -Wno-unused-parameter $(XFLAGS) $(BENCH_CFLAGS) -fPIC -c $< -o $@

$(SYM_LIB): $(SYM_OBJS)
	ar rcs $@ $^

define SNOVA_RULES
cflags_$(1) := $(BASE_CFLAGS) $(call pf,$(1)) -I$(SRC)
objs_$(1)   := $(BUILD)/$(1)/snova.o $(BUILD)/$(1)/ct_functions.o $(BUILD)/$(1)/sign.o

$$(objs_$(1)): $(BUILD)/$(1)/%.o: $(SRC)/%.c
	@mkdir -p $$(@D)
	$$(CC) $$(cflags_$(1)) -c $$< -o $$@

$(BUILD)/$(1).so: $(1)_shim.c $$(objs_$(1)) $(SYM_LIB)
	$$(CC) $$(cflags_$(1)) -shared $$< $$(objs_$(1)) $(SYM_LIB) -o $$@
endef

$(foreach P,$(PARAMS),$(eval $(call SNOVA_RULES,$(P))))

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
