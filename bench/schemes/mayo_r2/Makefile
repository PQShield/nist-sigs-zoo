# MAYO, NIST on-ramp round 2: PQCMayo/MAYO-C tag `round2` (11ff0f0, 2025-02-06). Its code
# is identical to Optimized_Implementation and Additional_Implementations/AVX2 of the NIST
# round-2 package mayo-submission-round2.tar.gz (only README/META/target.cmake differ).
include ../../../bench-common/flags.mk
.DEFAULT_GOAL := all
CC      ?= cc
# Upstream's CMake adds -O3 -funroll-loops -march=native; we use the global flag set
# (flags.mk). Upstream's default build type at this tag is "opt" (portable C); the
# "avx2" build type (also the default of the NIST package's AVX2 directory) is its fastest
# x86 backend at round 2 (GFNI/AVX-512 came with round 3), so we build that one.
STD     := -Wall -Wextra -std=c99
BUILD   := build
REF     := ref
SRC     := $(REF)/src

INCFLAGS := -I$(REF)/include -I$(SRC) -I$(SRC)/common -I$(SRC)/AVX2 -I$(SRC)/generic
DEFINES    := -DMAYO_BUILD_TYPE_AVX2 -DMAYO_AVX -DENABLE_AESNI
SIMD_FLAGS := -mavx2 -maes -mssse3
CFLAGS  := $(STD) $(SIMD_FLAGS) $(DEFINES) $(INCFLAGS) $(BENCH_CFLAGS) -fPIC

SOS := $(BUILD)/mayo_r2_1.so $(BUILD)/mayo_r2_2.so $(BUILD)/mayo_r2_3.so $(BUILD)/mayo_r2_5.so
SHIM_FILES := mayo_r2_1_shim.c mayo_r2_2_shim.c mayo_r2_3_shim.c mayo_r2_5_shim.c

.PHONY: all clean check_ref

all: check_ref $(SHIM_FILES) $(SOS)

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

check_ref:
	@if [ ! -f $(REF)/src/mayo.c ]; then \
	    echo "ERROR: bench/schemes/mayo_r2/ref/ not populated." >&2; \
	    echo "Run: git submodule update --init bench/schemes/mayo_r2/ref" >&2; \
	    exit 1; \
	fi

$(BUILD):
	mkdir -p $@

# Variant-independent sources (upstream's mayo_common_sys library)
COMMON_SRCS := \
    $(SRC)/common/fips202.c \
    $(SRC)/common/mem.c \
    $(SRC)/common/randombytes_system.c \
    $(SRC)/common/aes_c.c \
    $(SRC)/common/aes128ctr.c \
    $(SRC)/common/aes_neon.c

COMMON_OBJS := $(patsubst $(SRC)/common/%.c, $(BUILD)/common_%.o, $(COMMON_SRCS))

$(BUILD)/common_%.o: $(SRC)/common/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/libmayo_common.a: $(COMMON_OBJS)
	ar rcs $@ $^

# Per-variant: mayo.c + arithmetic.c with MAYO_VARIANT=X (params.c is empty without
# ENABLE_PARAMS_DYNAMIC), linked with the shim into a .so
define VARIANT_RULES
$(BUILD)/$(2)_mayo.o: $(SRC)/mayo.c | $(BUILD)
	$(CC) -DMAYO_VARIANT=$(1) $(CFLAGS) -c $$< -o $$@

$(BUILD)/$(2)_arith.o: $(SRC)/arithmetic.c | $(BUILD)
	$(CC) -DMAYO_VARIANT=$(1) $(CFLAGS) -c $$< -o $$@

$(BUILD)/$(2).so: $(2)_shim.c $(BUILD)/$(2)_mayo.o $(BUILD)/$(2)_arith.o $(BUILD)/libmayo_common.a | $(BUILD)
	$(CC) -DMAYO_VARIANT=$(1) -I../.. $(CFLAGS) -shared -o $$@ $(2)_shim.c \
	    $(BUILD)/$(2)_mayo.o $(BUILD)/$(2)_arith.o $(BUILD)/libmayo_common.a
endef

$(eval $(call VARIANT_RULES,MAYO_1,mayo_r2_1))
$(eval $(call VARIANT_RULES,MAYO_2,mayo_r2_2))
$(eval $(call VARIANT_RULES,MAYO_3,mayo_r2_3))
$(eval $(call VARIANT_RULES,MAYO_5,mayo_r2_5))

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
