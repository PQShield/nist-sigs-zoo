CC  ?= cc
AR  ?= ar

.DEFAULT_GOAL := all

REF   := ref
BUILD := build

# ------- Platform detection -------
AVX512_DETECT  := $(shell $(CC) -O0 -march=native -dM -E - < /dev/null 2>/dev/null | grep __AVX512F__)
AVX2_DETECT    := $(shell $(CC) -O0 -march=native -dM -E - < /dev/null 2>/dev/null | grep __AVX2__)

ifneq ($(AVX512_DETECT),)
  KECCAK_PLATFORM := avx512
  KECCAK_CFLAGS   := -mavx512vl -mavx512f
else ifneq ($(AVX2_DETECT),)
  KECCAK_PLATFORM := avx2
  KECCAK_CFLAGS   := -mavx2
else
  KECCAK_PLATFORM := opt64
  KECCAK_CFLAGS   :=
endif

# ------- SHA3 sources -------
SHA3     := $(REF)/sha3
SHA3_INC := -I$(SHA3) -I$(SHA3)/$(KECCAK_PLATFORM)

SHA3_COMMON_SRCS := \
    $(SHA3)/KeccakHash.c $(SHA3)/KeccakSponge.c \
    $(SHA3)/KeccakHashtimes4.c $(SHA3)/KeccakSpongetimes4.c

ifeq ($(KECCAK_PLATFORM),avx512)
  SHA3_PLAT_SRCS := \
      $(SHA3)/avx512/KeccakP-1600-AVX512.S \
      $(SHA3)/avx512/KeccakP-1600-times4-SIMD512.c
else ifeq ($(KECCAK_PLATFORM),avx2)
  SHA3_PLAT_SRCS := \
      $(SHA3)/avx2/KeccakP-1600-AVX2.S \
      $(SHA3)/avx2/KeccakP-1600-times4-SIMD256.c
else
  SHA3_PLAT_SRCS := \
      $(SHA3)/opt64/KeccakP-1600-opt64.c \
      $(SHA3)/opt64/KeccakP-1600-times4-on1.c
endif

SHA3_SRCS := $(SHA3_COMMON_SRCS) $(SHA3_PLAT_SRCS)
SHA3_OBJS := $(patsubst $(SHA3)/%,$(BUILD)/sha3/%.o,$(SHA3_SRCS))
SHA3_LIB  := $(BUILD)/libhash.a
SHA3_CFLAGS := -O2 -march=native -fPIC -std=gnu99 -DNO_MISALIGNED_ACCESSES $(KECCAK_CFLAGS) $(SHA3_INC)

# ------- All per-variant sources (rijndael included so platform flags stay consistent) -------
VARIANT_SRCS := \
    $(addprefix $(REF)/rijndael/, \
        rijndael_ref.c rijndael_table.c rijndael_aes_ni.c \
        rijndael_ct64.c rijndael_external.c) \
    $(REF)/xof.c $(REF)/prg.c $(REF)/expand_mq.c \
    $(REF)/keygen.c $(REF)/sign.c $(REF)/sign_memopt.c \
    $(addprefix $(REF)/ggm_tree/, \
        ggm_tree.c ggm_tree_incr_x1_base.c ggm_tree_incr_x2.c \
        ggm_tree_incr_x4.c ggm_tree_incr_x1_adv.c ggm_tree_incr_batch.c) \
    $(addprefix $(REF)/blc/, \
        blc_default.c blc_memopt.c blc_memopt_x1.c \
        blc_memopt_x2.c blc_memopt_x4.c) \
    $(addprefix $(REF)/piop/, \
        piop_default.c piop_memopt.c piop_bitslice.c)

INCLUDES := \
    $(SHA3_INC) \
    -I$(REF)/rijndael -I$(REF)/ggm_tree -I$(REF)/blc \
    -I$(REF)/piop -I$(REF)/fields -I$(REF)/fields_bitsliced \
    -I$(REF)

CFLAGS_BASE := -O2 -march=native -fPIC -std=c99 \
    -DNDEBUG -DNO_ALLOC_PROBE \
    -DUSE_PRG_CACHE -DUSE_PIOP_CACHE -DUSE_XOF_X4 -DUSE_ENC_X8 \
    $(KECCAK_CFLAGS)

# ------- Variant rule template -------
# $(1) = file stem         (e.g. mqom_cat1_gf2_fast_r3)
# $(2) = SECURITY          (128/192/256)
# $(3) = MQOM2_PARAM_BASE_FIELD (1/4/8)
# $(4) = TRADEOFF          (0=fast, 1=short)
# $(5) = NBROUNDS          (3/5)

PARAM_SETS :=

define MQOM_RULES
PARAM_SETS += $(1)
$(BUILD)/$(1).so: $(1)_shim.c $$(VARIANT_SRCS) $$(SHA3_LIB) | $$(BUILD)
	$$(CC) $$(CFLAGS_BASE) \
	    -DMQOM2_PARAM_SECURITY=$(2) -DMQOM2_PARAM_BASE_FIELD=$(3) \
	    -DMQOM2_PARAM_TRADEOFF=$(4) -DMQOM2_PARAM_NBROUNDS=$(5) \
	    $$(INCLUDES) -shared \
	    $(1)_shim.c $$(VARIANT_SRCS) $$(SHA3_LIB) \
	    -o $$@
endef

# cat1 variants
$(eval $(call MQOM_RULES,mqom_cat1_gf2_fast_r3,128,1,0,3))
$(eval $(call MQOM_RULES,mqom_cat1_gf2_fast_r5,128,1,0,5))
$(eval $(call MQOM_RULES,mqom_cat1_gf2_short_r3,128,1,1,3))
$(eval $(call MQOM_RULES,mqom_cat1_gf2_short_r5,128,1,1,5))
$(eval $(call MQOM_RULES,mqom_cat1_gf16_fast_r3,128,4,0,3))
$(eval $(call MQOM_RULES,mqom_cat1_gf16_fast_r5,128,4,0,5))
$(eval $(call MQOM_RULES,mqom_cat1_gf16_short_r3,128,4,1,3))
$(eval $(call MQOM_RULES,mqom_cat1_gf16_short_r5,128,4,1,5))
$(eval $(call MQOM_RULES,mqom_cat1_gf256_fast_r3,128,8,0,3))
$(eval $(call MQOM_RULES,mqom_cat1_gf256_fast_r5,128,8,0,5))
$(eval $(call MQOM_RULES,mqom_cat1_gf256_short_r3,128,8,1,3))
$(eval $(call MQOM_RULES,mqom_cat1_gf256_short_r5,128,8,1,5))
# cat3 variants
$(eval $(call MQOM_RULES,mqom_cat3_gf2_fast_r3,192,1,0,3))
$(eval $(call MQOM_RULES,mqom_cat3_gf2_fast_r5,192,1,0,5))
$(eval $(call MQOM_RULES,mqom_cat3_gf2_short_r3,192,1,1,3))
$(eval $(call MQOM_RULES,mqom_cat3_gf2_short_r5,192,1,1,5))
$(eval $(call MQOM_RULES,mqom_cat3_gf16_fast_r3,192,4,0,3))
$(eval $(call MQOM_RULES,mqom_cat3_gf16_fast_r5,192,4,0,5))
$(eval $(call MQOM_RULES,mqom_cat3_gf16_short_r3,192,4,1,3))
$(eval $(call MQOM_RULES,mqom_cat3_gf16_short_r5,192,4,1,5))
$(eval $(call MQOM_RULES,mqom_cat3_gf256_fast_r3,192,8,0,3))
$(eval $(call MQOM_RULES,mqom_cat3_gf256_fast_r5,192,8,0,5))
$(eval $(call MQOM_RULES,mqom_cat3_gf256_short_r3,192,8,1,3))
$(eval $(call MQOM_RULES,mqom_cat3_gf256_short_r5,192,8,1,5))
# cat5 variants
$(eval $(call MQOM_RULES,mqom_cat5_gf2_fast_r3,256,1,0,3))
$(eval $(call MQOM_RULES,mqom_cat5_gf2_fast_r5,256,1,0,5))
$(eval $(call MQOM_RULES,mqom_cat5_gf2_short_r3,256,1,1,3))
$(eval $(call MQOM_RULES,mqom_cat5_gf2_short_r5,256,1,1,5))
$(eval $(call MQOM_RULES,mqom_cat5_gf16_fast_r3,256,4,0,3))
$(eval $(call MQOM_RULES,mqom_cat5_gf16_fast_r5,256,4,0,5))
$(eval $(call MQOM_RULES,mqom_cat5_gf16_short_r3,256,4,1,3))
$(eval $(call MQOM_RULES,mqom_cat5_gf16_short_r5,256,4,1,5))
$(eval $(call MQOM_RULES,mqom_cat5_gf256_fast_r3,256,8,0,3))
$(eval $(call MQOM_RULES,mqom_cat5_gf256_fast_r5,256,8,0,5))
$(eval $(call MQOM_RULES,mqom_cat5_gf256_short_r3,256,8,1,3))
$(eval $(call MQOM_RULES,mqom_cat5_gf256_short_r5,256,8,1,5))

TARGETS    := $(addprefix $(BUILD)/, $(addsuffix .so, $(PARAM_SETS)))
SHIM_FILES := $(addsuffix _shim.c, $(PARAM_SETS))

.PHONY: all clean check-submodule

all: check-submodule $(SHIM_FILES) $(TARGETS)

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

check-submodule:
	@test -f $(REF)/sign.c || { \
	    echo "ERROR: bench/schemes/mqom/ref/ is empty."; \
	    echo "Run: git submodule update --init bench/schemes/mqom/ref"; \
	    exit 1; }

# ------- SHA3 library -------
$(BUILD)/sha3:
	mkdir -p $@

$(BUILD)/sha3/%.c.o: $(SHA3)/%.c | $(BUILD)/sha3
	@mkdir -p $(dir $@)
	$(CC) $(SHA3_CFLAGS) -c $< -o $@

$(BUILD)/sha3/%.S.o: $(SHA3)/%.S | $(BUILD)/sha3
	@mkdir -p $(dir $@)
	$(CC) $(KECCAK_CFLAGS) -fPIC -c $< -o $@

$(SHA3_LIB): $(SHA3_OBJS) | $(BUILD)
	$(AR) rcs $@ $^

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
