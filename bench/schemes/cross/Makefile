# CROSS v2.2 (AVX2 Optimized_Implementation). Upstream:
#   https://github.com/CROSS-signature/CROSS-implementation  tag v2.2 = b2a84251c838
# Mirrors Additional_Implementations/Benchmarking/CMakeLists.txt: Optimized_Implementation
# sources + Reference_Implementation common sources (minus sign.c, which only has the
# combined API), standalone SHA-3 (no libkeccak), -O3 -march=native, C11.
.DEFAULT_GOAL := all

CC     ?= cc
BUILD  := build
REF    := ref
OPT    := $(REF)/Optimized_Implementation
COM    := $(REF)/Reference_Implementation

# SKIP_ASSERT: upstream asserts (aborts) on invalid signatures in CROSS_verify unless
# this is set; the harness checks that tampered signatures are rejected.
CFLAGS := -std=c11 -O3 -march=native -mtune=native -Wall -pedantic -fPIC -fno-semantic-interposition -DNDEBUG -DSKIP_ASSERT
INCFLAGS := -I$(OPT)/include -I$(COM)/include

SRCS := \
    $(OPT)/lib/fips202x4.c \
    $(OPT)/lib/KeccakP-1600-times4-SIMD256.c \
    $(OPT)/lib/merkle.c \
    $(OPT)/lib/seedtree.c \
    $(OPT)/lib/CROSS.c \
    $(COM)/lib/csprng_hash.c \
    $(COM)/lib/pack_unpack.c \
    $(COM)/lib/keccakf1600.c \
    $(COM)/lib/fips202.c

PARAMS := \
    cross_rsdp_1_fast \
    cross_rsdp_1_balanced \
    cross_rsdp_1_small \
    cross_rsdpg_1_fast \
    cross_rsdpg_1_balanced \
    cross_rsdpg_1_small \
    cross_rsdp_3_fast \
    cross_rsdp_3_balanced \
    cross_rsdp_3_small \
    cross_rsdpg_3_fast \
    cross_rsdpg_3_balanced \
    cross_rsdpg_3_small \
    cross_rsdp_5_fast \
    cross_rsdp_5_balanced \
    cross_rsdp_5_small \
    cross_rsdpg_5_fast \
    cross_rsdpg_5_balanced \
    cross_rsdpg_5_small

SOS        := $(addprefix $(BUILD)/, $(addsuffix .so, $(PARAMS)))
SHIM_FILES := $(addsuffix _shim.c, $(PARAMS))

.PHONY: all clean check_ref

all: check_ref $(SHIM_FILES) $(SOS)

check_ref:
	@if [ ! -f $(OPT)/lib/CROSS.c ]; then \
	    echo "ERROR: bench/schemes/cross/ref/ not populated." >&2; \
	    echo "Run: git submodule update --init bench/schemes/cross/ref" >&2; \
	    exit 1; \
	fi

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(BUILD):
	mkdir -p $@

# Every source depends on parameters.h, so compile all of it per variant into the .so.
# $(1)=file  $(2)=defines
define VARIANT_RULES
$(BUILD)/$(1).so: $(1)_shim.c $(SRCS) | $(BUILD)
	$(CC) $(CFLAGS) $(2) $(INCFLAGS) -I../.. -shared -o $$@ $(1)_shim.c $(SRCS)
endef

$(eval $(call VARIANT_RULES,cross_rsdp_1_fast,-DCATEGORY_1=1 -DRSDP=1 -DSPEED=1 -DNO_TREES=1))
$(eval $(call VARIANT_RULES,cross_rsdp_1_balanced,-DCATEGORY_1=1 -DRSDP=1 -DBALANCED=1))
$(eval $(call VARIANT_RULES,cross_rsdp_1_small,-DCATEGORY_1=1 -DRSDP=1 -DSIG_SIZE=1))
$(eval $(call VARIANT_RULES,cross_rsdpg_1_fast,-DCATEGORY_1=1 -DRSDPG=1 -DSPEED=1 -DNO_TREES=1))
$(eval $(call VARIANT_RULES,cross_rsdpg_1_balanced,-DCATEGORY_1=1 -DRSDPG=1 -DBALANCED=1))
$(eval $(call VARIANT_RULES,cross_rsdpg_1_small,-DCATEGORY_1=1 -DRSDPG=1 -DSIG_SIZE=1))
$(eval $(call VARIANT_RULES,cross_rsdp_3_fast,-DCATEGORY_3=1 -DRSDP=1 -DSPEED=1 -DNO_TREES=1))
$(eval $(call VARIANT_RULES,cross_rsdp_3_balanced,-DCATEGORY_3=1 -DRSDP=1 -DBALANCED=1))
$(eval $(call VARIANT_RULES,cross_rsdp_3_small,-DCATEGORY_3=1 -DRSDP=1 -DSIG_SIZE=1))
$(eval $(call VARIANT_RULES,cross_rsdpg_3_fast,-DCATEGORY_3=1 -DRSDPG=1 -DSPEED=1 -DNO_TREES=1))
$(eval $(call VARIANT_RULES,cross_rsdpg_3_balanced,-DCATEGORY_3=1 -DRSDPG=1 -DBALANCED=1))
$(eval $(call VARIANT_RULES,cross_rsdpg_3_small,-DCATEGORY_3=1 -DRSDPG=1 -DSIG_SIZE=1))
$(eval $(call VARIANT_RULES,cross_rsdp_5_fast,-DCATEGORY_5=1 -DRSDP=1 -DSPEED=1 -DNO_TREES=1))
$(eval $(call VARIANT_RULES,cross_rsdp_5_balanced,-DCATEGORY_5=1 -DRSDP=1 -DBALANCED=1))
$(eval $(call VARIANT_RULES,cross_rsdp_5_small,-DCATEGORY_5=1 -DRSDP=1 -DSIG_SIZE=1))
$(eval $(call VARIANT_RULES,cross_rsdpg_5_fast,-DCATEGORY_5=1 -DRSDPG=1 -DSPEED=1 -DNO_TREES=1))
$(eval $(call VARIANT_RULES,cross_rsdpg_5_balanced,-DCATEGORY_5=1 -DRSDPG=1 -DBALANCED=1))
$(eval $(call VARIANT_RULES,cross_rsdpg_5_small,-DCATEGORY_5=1 -DRSDPG=1 -DSIG_SIZE=1))

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
