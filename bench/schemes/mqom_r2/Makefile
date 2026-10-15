include ../../../bench-common/flags.mk
CC  ?= cc
AR  ?= ar

.DEFAULT_GOAL := all

# MQOM v2.0.0 = the NIST round-2 submission (ref/ = mqom/mqom-v2 tag v2.0.0, 377f5ce;
# its C sources and per-set parameter headers are byte-identical to
# Optimized_Implementation_AVX2/ and _GFNI/ of mqom-submission-round2.zip).
REF   := ref
BUILD := build

# ------- Backend selection (as the submission package's Optimized_Implementation_* Makefiles) -------
# Both optimised packages build with KECCAK_AVX2=1 RIJNDAEL_AES_NI=1 and differ only in
# FIELDS_AVX2 vs FIELDS_GFNI. The GFNI field code needs GFNI + AVX-512F/BW/VPOPCNTDQ
# (fields/fields_gfni.h; it also uses AVX-512VL/VBMI intrinsics, which upstream gets from
# -march=native): use it when the host has them, else the AVX2 package.
HAVE_GFNI_AVX512 := $(shell python3 ../../cpu_features.py gfni avx512f avx512bw avx512vl avx512vbmi avx512_vpopcntdq)
ifeq ($(HAVE_GFNI_AVX512),1)
  FIELDS_CFLAGS := -DFIELDS_GFNI -mgfni -mavx512f -mavx512bw -mavx512vl -mavx512vbmi -mavx512vpopcntdq
else
  FIELDS_CFLAGS := -DFIELDS_AVX2
endif

# ------- SHA3 (XKCP, avx2 platform as upstream's sha3/Makefile with PLATFORM=avx2) -------
SHA3     := $(REF)/sha3
SHA3_INC := -I$(SHA3) -I$(SHA3)/avx2
SHA3_SRCS := $(SHA3)/KeccakHash.c $(SHA3)/KeccakSponge.c \
    $(SHA3)/KeccakHashtimes4.c $(SHA3)/KeccakSpongetimes4.c \
    $(SHA3)/avx2/KeccakP-1600-AVX2.S $(SHA3)/avx2/KeccakP-1600-times4-SIMD256.c
SHA3_OBJS := $(patsubst $(SHA3)/%,$(BUILD)/sha3/%.o,$(SHA3_SRCS))
SHA3_LIB  := $(BUILD)/libhash.a
# upstream sha3/Makefile: -std=gnu99 -mavx2 (its -O3 is replaced by the global flags)
SHA3_CFLAGS := -std=gnu99 -mavx2 $(SHA3_INC) $(BENCH_CFLAGS) -fPIC

# ------- MQOM2 sources (upstream Makefile's RIJNDAEL_SRC_FILES + MQOM2_SRC_FILES) -------
SRCS := $(addprefix $(REF)/rijndael/, rijndael_ref.c rijndael_table.c rijndael_aes_ni.c) \
    $(addprefix $(REF)/, xof.c prg.c ggm_tree.c expand_mq.c keygen.c blc.c piop.c sign.c)

INCLUDES := $(SHA3_INC) -I$(REF)/rijndael -I$(REF)/fields -I$(REF)

# Upstream has no -std (GCC's default; gnu17 kept for GCC 15, whose default is C23).
# sign.c defines crypto_sign_signature/crypto_sign_verify with unsigned long long
# lengths; they are renamed here and wrapped by the shim (which #undefs the names).
CFLAGS_BASE := -std=gnu17 -mavx2 -maes $(FIELDS_CFLAGS) -DRIJNDAEL_AES_NI \
    -Dcrypto_sign_signature=mqom2_crypto_sign_signature \
    -Dcrypto_sign_verify=mqom2_crypto_sign_verify \
    $(BENCH_CFLAGS) -fPIC

# ------- Variant rule template -------
# $(1) = file stem, $(2) = MQOM2_PARAM_SECURITY (128/192/256),
# $(3) = MQOM2_PARAM_BASE_FIELD (1=gf2, 8=gf256), $(4) = MQOM2_PARAM_TRADEOFF (0=fast, 1=short),
# $(5) = MQOM2_PARAM_NBROUNDS (3/5)
PARAM_SETS :=

define MQOM_RULES
PARAM_SETS += $(1)
$(BUILD)/$(1).so: $(1)_shim.c $$(SRCS) $$(SHA3_LIB) | $$(BUILD)
	$$(CC) $$(CFLAGS_BASE) \
	    -DMQOM2_PARAM_SECURITY=$(2) -DMQOM2_PARAM_BASE_FIELD=$(3) \
	    -DMQOM2_PARAM_TRADEOFF=$(4) -DMQOM2_PARAM_NBROUNDS=$(5) \
	    $$(INCLUDES) -shared \
	    $(1)_shim.c $$(SRCS) $$(SHA3_LIB) \
	    -o $$@
endef

$(eval $(call MQOM_RULES,mqom2_cat1_gf2_fast_r3,128,1,0,3))
$(eval $(call MQOM_RULES,mqom2_cat1_gf2_fast_r5,128,1,0,5))
$(eval $(call MQOM_RULES,mqom2_cat1_gf2_short_r3,128,1,1,3))
$(eval $(call MQOM_RULES,mqom2_cat1_gf2_short_r5,128,1,1,5))
$(eval $(call MQOM_RULES,mqom2_cat1_gf256_fast_r3,128,8,0,3))
$(eval $(call MQOM_RULES,mqom2_cat1_gf256_fast_r5,128,8,0,5))
$(eval $(call MQOM_RULES,mqom2_cat1_gf256_short_r3,128,8,1,3))
$(eval $(call MQOM_RULES,mqom2_cat1_gf256_short_r5,128,8,1,5))
$(eval $(call MQOM_RULES,mqom2_cat3_gf2_fast_r3,192,1,0,3))
$(eval $(call MQOM_RULES,mqom2_cat3_gf2_fast_r5,192,1,0,5))
$(eval $(call MQOM_RULES,mqom2_cat3_gf2_short_r3,192,1,1,3))
$(eval $(call MQOM_RULES,mqom2_cat3_gf2_short_r5,192,1,1,5))
$(eval $(call MQOM_RULES,mqom2_cat3_gf256_fast_r3,192,8,0,3))
$(eval $(call MQOM_RULES,mqom2_cat3_gf256_fast_r5,192,8,0,5))
$(eval $(call MQOM_RULES,mqom2_cat3_gf256_short_r3,192,8,1,3))
$(eval $(call MQOM_RULES,mqom2_cat3_gf256_short_r5,192,8,1,5))
$(eval $(call MQOM_RULES,mqom2_cat5_gf2_fast_r3,256,1,0,3))
$(eval $(call MQOM_RULES,mqom2_cat5_gf2_fast_r5,256,1,0,5))
$(eval $(call MQOM_RULES,mqom2_cat5_gf2_short_r3,256,1,1,3))
$(eval $(call MQOM_RULES,mqom2_cat5_gf2_short_r5,256,1,1,5))
$(eval $(call MQOM_RULES,mqom2_cat5_gf256_fast_r3,256,8,0,3))
$(eval $(call MQOM_RULES,mqom2_cat5_gf256_fast_r5,256,8,0,5))
$(eval $(call MQOM_RULES,mqom2_cat5_gf256_short_r3,256,8,1,3))
$(eval $(call MQOM_RULES,mqom2_cat5_gf256_short_r5,256,8,1,5))

TARGETS    := $(addprefix $(BUILD)/, $(addsuffix .so, $(PARAM_SETS)))
SHIM_FILES := $(addsuffix _shim.c, $(PARAM_SETS))

.PHONY: all clean check-submodule

all: check-submodule $(SHIM_FILES) $(TARGETS)

$(SHIM_FILES) &: shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

check-submodule:
	@test -f $(REF)/sign.c || { \
	    echo "ERROR: bench/schemes/mqom_r2/ref/ is empty."; \
	    echo "Run: git submodule update --init bench/schemes/mqom_r2/ref"; \
	    exit 1; }

$(BUILD)/sha3/%.c.o: $(SHA3)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(SHA3_CFLAGS) -c $< -o $@

$(BUILD)/sha3/%.S.o: $(SHA3)/%.S
	@mkdir -p $(dir $@)
	$(CC) $(SHA3_CFLAGS) -c $< -o $@

$(SHA3_LIB): $(SHA3_OBJS) | $(BUILD)
	$(AR) rcs $@ $^

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
