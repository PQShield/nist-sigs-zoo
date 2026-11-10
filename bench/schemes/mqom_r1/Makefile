include ../../../bench-common/flags.mk
CC  ?= cc
AR  ?= ar

.DEFAULT_GOAL := all

# MQOM v1.0 (NIST round 1) is published only as the NIST submission package:
# .source holds its URL (line 1) and sha256 (line 2); it is fetched into build/dl and
# unpacked into build/src. Each parameter set has its own self-contained directory under
# Optimized_Implementation/ (the only optimised implementation shipped; AVX2 Keccak x4).
BUILD    := build
URL      := $(shell sed -n 1p .source)
ZIP      := $(BUILD)/dl/mqom-submission.zip
OPT      := $(BUILD)/src/submission_package/Optimized_Implementation
UNZIPPED := $(BUILD)/src/.unzipped

# ------- SHA3 (XKCP, avx2 platform; sha3/ is identical in every parameter-set directory) -------
SHA3     := $(OPT)/mqom_cat1_gf31_fast/sha3
SHA3_SRCS := KeccakHash.c KeccakSponge.c KeccakSpongetimes4.c KeccakHashtimes4.c \
    avx2/KeccakP-1600-AVX2.S avx2/KeccakP-1600-times4-SIMD256.c
SHA3_OBJS := $(addprefix $(BUILD)/sha3/,$(addsuffix .o,$(SHA3_SRCS)))
SHA3_LIB  := $(BUILD)/libhash.a
# upstream sha3/Makefile with PLATFORM=avx2: -std=gnu99 -mavx (its -O3 -march=native is
# replaced by the global flags; -mavx2 so the AVX2 code also compile-tests elsewhere)
SHA3_CFLAGS := -std=gnu99 -mavx -mavx2 -I$(SHA3) -I$(SHA3)/avx2 $(BENCH_CFLAGS) -fPIC

# ------- MQOM sources (upstream Makefile: SYM_OBJ ARITH_OBJ MPC_OBJ CORE_OBJ) -------
# $(1) = field (gf31/gf251)
MQOM_SRCS = rnd.c hash.c xof.c $(1)-matrix.c $(1).c \
    mpc.c witness.c serialization-specific.c precomputed.c \
    keygen.c sign.c views.c commit.c sign-mpcith-hypercube.c tree.c

# Upstream ALL_FLAGS: -O3 -flto -fPIC -std=c11 -march=native -W... -DPARAM_HYPERCUBE_7R
# -DPARAM_<field> -DPARAM_<level> -DPARAM_RND_EXPANSION_X4 -DHASHX4 -DXOFX4 -DPRGX4 -DNDEBUG -mavx.
# We keep -std, -mavx and the -D flags (no -flto, see bench/CLAUDE.md "Compiler flags").
# sign.c defines crypto_sign_signature/crypto_sign_verify with unsigned long long
# lengths; they are renamed here and wrapped by the shim (which #undefs the names).
CFLAGS_BASE := -std=c11 -mavx -mavx2 \
    -DPARAM_HYPERCUBE_7R -DPARAM_RND_EXPANSION_X4 -DHASHX4 -DXOFX4 -DPRGX4 \
    -Dcrypto_sign_signature=mqom1_crypto_sign_signature \
    -Dcrypto_sign_verify=mqom1_crypto_sign_verify \
    $(BENCH_CFLAGS) -fPIC

# ------- Variant rule template -------
# $(1) = file stem = upstream directory, $(2) = field (gf31/gf251), $(3) = level (L1/L3/L5)
PARAM_SETS :=

define MQOM_RULES
PARAM_SETS += $(1)
$(BUILD)/$(1).so: $(1)_shim.c $$(SHA3_LIB) | $$(BUILD)
	$$(CC) $$(CFLAGS_BASE) -DPARAM_$(shell echo $(2) | tr a-z A-Z) -DPARAM_$(3) \
	    -I$$(OPT)/$(1) -I$$(SHA3) -I$$(SHA3)/avx2 -I../.. -shared \
	    $(1)_shim.c $$(addprefix $$(OPT)/$(1)/,$$(call MQOM_SRCS,$(2))) $$(SHA3_LIB) \
	    -o $$@
endef

$(eval $(call MQOM_RULES,mqom_cat1_gf31_short,gf31,L1))
$(eval $(call MQOM_RULES,mqom_cat1_gf31_fast,gf31,L1))
$(eval $(call MQOM_RULES,mqom_cat1_gf251_short,gf251,L1))
$(eval $(call MQOM_RULES,mqom_cat1_gf251_fast,gf251,L1))
$(eval $(call MQOM_RULES,mqom_cat3_gf31_short,gf31,L3))
$(eval $(call MQOM_RULES,mqom_cat3_gf31_fast,gf31,L3))
$(eval $(call MQOM_RULES,mqom_cat3_gf251_short,gf251,L3))
$(eval $(call MQOM_RULES,mqom_cat3_gf251_fast,gf251,L3))
$(eval $(call MQOM_RULES,mqom_cat5_gf31_short,gf31,L5))
$(eval $(call MQOM_RULES,mqom_cat5_gf31_fast,gf31,L5))
$(eval $(call MQOM_RULES,mqom_cat5_gf251_short,gf251,L5))
$(eval $(call MQOM_RULES,mqom_cat5_gf251_fast,gf251,L5))

TARGETS    := $(addprefix $(BUILD)/, $(addsuffix .so, $(PARAM_SETS)))
SHIM_FILES := $(addsuffix _shim.c, $(PARAM_SETS))

.PHONY: all clean fetch

all: $(SHIM_FILES) $(TARGETS)

fetch: $(ZIP)

$(SHIM_FILES) &: shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(ZIP): .source
	mkdir -p $(@D)
	curl -fsSL --retry 3 -o $@.tmp '$(URL)'
	cd $(@D) && sed -n 2p ../../.source | sed 's/mqom-submission.zip$$/mqom-submission.zip.tmp/' | sha256sum -c - \
	    || { echo "SHA-256 mismatch for $@" >&2; rm -f $(@F).tmp; exit 1; }
	mv $@.tmp $@

$(UNZIPPED): $(ZIP)
	rm -rf $(BUILD)/src
	mkdir -p $(BUILD)/src
	python3 -m zipfile -e $(ZIP) $(BUILD)/src
	touch $@


$(BUILD)/sha3/%.c.o: $(UNZIPPED)
	@mkdir -p $(dir $@)
	$(CC) $(SHA3_CFLAGS) -c $(SHA3)/$*.c -o $@

$(BUILD)/sha3/%.S.o: $(UNZIPPED)
	@mkdir -p $(dir $@)
	$(CC) $(SHA3_CFLAGS) -c $(SHA3)/$*.S -o $@

$(SHA3_LIB): $(SHA3_OBJS) | $(BUILD)
	$(AR) rcs $@ $^

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)/src $(BUILD)/sha3 $(BUILD)/*.so $(BUILD)/*.a
	rm -f $(SHIM_FILES)
