# MAYO, NIST on-ramp round 1: PQCMayo/MAYO-C 3c9be9e "Add implementation" (2023-06-01,
# the first commit; untagged). Its code is identical to Optimized_Implementation and
# Additional_Implementations/AVX2 of the NIST round-1 package mayo-submission.zip (only
# README and the AVX2 directory's default build type in .cmake/target.cmake differ).
include ../../../bench-common/flags.mk
.DEFAULT_GOAL := all
CC      ?= cc
# Upstream's CMake adds -O3 -funroll-loops -march=native; we use the global flag set
# (flags.mk). Upstream's default build type at this commit is "opt" (portable C); the
# "avx2" build type (the default of the NIST package's AVX2 directory) is its fastest x86
# backend at round 1, so we build that one.
# gnu99, not c99: what upstream's CMAKE_C_STANDARD 99 gives (CMake keeps extensions on).
# mayo.c includes debug_bench_tools.h, whose cpucycles() uses GNU `asm`.
STD     := -Wall -Wextra -std=gnu99
BUILD   := build
REF     := ref
SRC     := $(REF)/src

# avx2 build type: AVX2 headers replace generic/ (round 1 has no fallthrough to generic).
INCFLAGS := -I$(REF)/include -I$(SRC) -I$(SRC)/common -I$(SRC)/AVX2
# TARGET_AMD64/TARGET_OS_UNIX: set by upstream's .cmake/target.cmake (only used by the
# unused cpucycles() in debug_bench_tools.h).
DEFINES    := -DMAYO_BUILD_TYPE_AVX2 -DMAYO_AVX -DENABLE_AESNI -DTARGET_AMD64 -DTARGET_OS_UNIX
SIMD_FLAGS := -mavx2 -maes -mssse3
CFLAGS  := $(STD) $(SIMD_FLAGS) $(DEFINES) $(INCFLAGS) $(BENCH_CFLAGS) -fPIC

SOS := $(BUILD)/mayo_r1_1.so $(BUILD)/mayo_r1_2.so $(BUILD)/mayo_r1_3.so $(BUILD)/mayo_r1_5.so
SHIM_FILES := mayo_r1_1_shim.c mayo_r1_2_shim.c mayo_r1_3_shim.c mayo_r1_5_shim.c

.PHONY: all clean check_ref

all: check_ref $(SHIM_FILES) $(SOS)

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

check_ref:
	@if [ ! -f $(REF)/src/mayo.c ]; then \
	    echo "ERROR: bench/schemes/mayo_r1/ref/ not populated." >&2; \
	    echo "Run: git submodule update --init bench/schemes/mayo_r1/ref" >&2; \
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
    $(SRC)/common/aes128ctr.c

COMMON_OBJS := $(patsubst $(SRC)/common/%.c, $(BUILD)/common_%.o, $(COMMON_SRCS))

$(BUILD)/common_%.o: $(SRC)/common/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/libmayo_common.a: $(COMMON_OBJS)
	ar rcs $@ $^

# Per-variant: mayo.c + params.c + bitsliced_arithmetic.c with MAYO_VARIANT=X (upstream's
# per-variant library), linked with the shim into a .so
define VARIANT_RULES
$(BUILD)/$(2)_mayo.o: $(SRC)/mayo.c | $(BUILD)
	$(CC) -DMAYO_VARIANT=$(1) $(CFLAGS) -c $$< -o $$@

$(BUILD)/$(2)_params.o: $(SRC)/params.c | $(BUILD)
	$(CC) -DMAYO_VARIANT=$(1) $(CFLAGS) -c $$< -o $$@

$(BUILD)/$(2)_arith.o: $(SRC)/bitsliced_arithmetic.c | $(BUILD)
	$(CC) -DMAYO_VARIANT=$(1) $(CFLAGS) -c $$< -o $$@

$(BUILD)/$(2).so: $(2)_shim.c $(BUILD)/$(2)_mayo.o $(BUILD)/$(2)_params.o $(BUILD)/$(2)_arith.o $(BUILD)/libmayo_common.a | $(BUILD)
	$(CC) -DMAYO_VARIANT=$(1) -I../.. $(CFLAGS) -shared -o $$@ $(2)_shim.c \
	    $(BUILD)/$(2)_mayo.o $(BUILD)/$(2)_params.o $(BUILD)/$(2)_arith.o \
	    $(BUILD)/libmayo_common.a
endef

$(eval $(call VARIANT_RULES,MAYO_1,mayo_r1_1))
$(eval $(call VARIANT_RULES,MAYO_2,mayo_r1_2))
$(eval $(call VARIANT_RULES,MAYO_3,mayo_r1_3))
$(eval $(call VARIANT_RULES,MAYO_5,mayo_r1_5))

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
