include ../../../bench-common/flags.mk
# LESS v2.1.1, AVX2 Optimized_Implementation. Upstream:
#   https://github.com/less-sig/LESS  tag v2.1.1, commit 6383430 (2026-07-13), which
#   implements ePrint 2025/1424. Same parameter sets and sizes as v2.0 (NIST round 2,
#   commit 9ec6915); the canonical-form code and so the KATs changed.
# Mirrors Optimized_Implementation/avx2/CMakeLists.txt, Release build type (its default):
# -std=gnu99 -DUSE_AVX2 -O3 -DNDEBUG -mavx2 -mavx -mbmi2 -mbmi -flto -ftree-vectorize
# -funroll-loops. We keep the language, define and ISA flags and use the global flag set
# (flags.mk) instead of upstream's optimisation flags.
.DEFAULT_GOAL := all

CC     ?= cc
BUILD  := build
REF    := ref
AVX2   := $(REF)/Optimized_Implementation/avx2
LIB    := $(AVX2)/lib

CFLAGS := -std=gnu99 -DUSE_AVX2 -Wall -Wextra -Wpedantic -Wno-type-limits \
          -mavx2 -mavx -mbmi2 -mbmi $(BENCH_CFLAGS) -fPIC
# KeccakP-1600-AVX2.s has no .note.GNU-stack section, so ld marks the .so as needing an
# executable stack, which newer glibc refuses to dlopen ("cannot enable executable stack").
CFLAGS += -Wa,--noexecstack
INCFLAGS := -I$(AVX2)/include

# lib/*.c are symlinks into Optimized_Implementation/common/lib (sign.c deliberately omitted).
SRCS := \
    $(LIB)/codes.c $(LIB)/fips202.c $(LIB)/keccakf1600.c $(LIB)/LESS.c \
    $(LIB)/monomial.c $(LIB)/rng.c $(LIB)/seedtree.c $(LIB)/utils.c \
    $(LIB)/canonical.c $(LIB)/sort.c $(LIB)/transpose.c $(LIB)/transpose_avx2.c \
    $(LIB)/KeccakP-1600-AVX2.s

PARAMS := \
    less_252_192 \
    less_252_68 \
    less_252_45 \
    less_400_220 \
    less_400_102 \
    less_548_345 \
    less_548_137

SOS        := $(addprefix $(BUILD)/, $(addsuffix .so, $(PARAMS)))
SHIM_FILES := $(addsuffix _shim.c, $(PARAMS))

.PHONY: all clean check_ref

all: check_ref $(SHIM_FILES) $(SOS)

check_ref:
	@if [ ! -f $(AVX2)/CMakeLists.txt ]; then \
	    echo "ERROR: bench/schemes/less/ref/ not populated." >&2; \
	    echo "Run: git submodule update --init bench/schemes/less/ref" >&2; \
	    exit 1; \
	fi

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(BUILD):
	mkdir -p $@

# Every source depends on parameters.h, so compile all of it per variant into the .so.
# $(1)=file  $(2)=CATEGORY  $(3)=TARGET
define VARIANT_RULES
$(BUILD)/$(1).so: $(1)_shim.c $(SRCS) | $(BUILD)
	$(CC) $(CFLAGS) -DCATEGORY=$(2) -DTARGET=$(3) $(INCFLAGS) -I../.. -shared -o $$@ \
	    $(1)_shim.c $(SRCS)
endef

$(eval $(call VARIANT_RULES,less_252_192,252,192))
$(eval $(call VARIANT_RULES,less_252_68,252,68))
$(eval $(call VARIANT_RULES,less_252_45,252,45))
$(eval $(call VARIANT_RULES,less_400_220,400,220))
$(eval $(call VARIANT_RULES,less_400_102,400,102))
$(eval $(call VARIANT_RULES,less_548_345,548,345))
$(eval $(call VARIANT_RULES,less_548_137,548,137))

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
