CC      ?= cc
CFLAGS  := -O2 -Wall -Wextra -std=c99
BUILD   := build
REF     := ref
SRC     := $(REF)/src

INCFLAGS := -I$(REF)/include -I$(SRC) -I$(SRC)/common -I$(SRC)/AVX2 -I$(SRC)/generic
DEFINES  := -DMAYO_BUILD_TYPE_AVX2 -DMAYO_AVX -DENABLE_AESNI

# AVX2 arithmetic and AES-NI intrinsics require these flags.
SIMD_FLAGS := -mavx2 -maes -mssse3

SOS := $(BUILD)/mayo1.so $(BUILD)/mayo2.so $(BUILD)/mayo3.so $(BUILD)/mayo5.so
SHIM_FILES := mayo1_shim.c mayo2_shim.c mayo3_shim.c mayo5_shim.c

.PHONY: all clean check_ref

all: check_ref $(SHIM_FILES) $(SOS)

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

check_ref:
	@if [ ! -f $(REF)/src/mayo.c ]; then \
	    echo "ERROR: bench/schemes/mayo/ref/ not populated." >&2; \
	    echo "Run: git submodule update --init bench/schemes/mayo/ref" >&2; \
	    exit 1; \
	fi

$(BUILD):
	mkdir -p $@

# Common (variant-independent) object files, compiled once
COMMON_SRCS := \
    $(SRC)/common/fips202.c \
    $(SRC)/common/mem.c \
    $(SRC)/common/randombytes_system.c \
    $(SRC)/common/aes_c.c \
    $(SRC)/common/aes128ctr.c \
    $(SRC)/common/aes_neon.c

COMMON_OBJS := $(patsubst $(SRC)/common/%.c, $(BUILD)/common_%.o, $(COMMON_SRCS))

$(BUILD)/common_%.o: $(SRC)/common/%.c | $(BUILD)
	$(CC) $(CFLAGS) $(SIMD_FLAGS) $(DEFINES) $(INCFLAGS) -fPIC -c $< -o $@

$(BUILD)/libmayo_common.a: $(COMMON_OBJS)
	ar rcs $@ $^

# Per-variant rules: compile mayo.c + arithmetic.c with MAYO_VARIANT=X, link shim .so
define VARIANT_RULES
$(BUILD)/$(2)_mayo.o: $(SRC)/mayo.c | $(BUILD)
	$(CC) $(CFLAGS) $(SIMD_FLAGS) $(DEFINES) -DMAYO_VARIANT=$(1) $(INCFLAGS) -fPIC -c $$< -o $$@

$(BUILD)/$(2)_arith.o: $(SRC)/arithmetic.c | $(BUILD)
	$(CC) $(CFLAGS) $(SIMD_FLAGS) $(DEFINES) -DMAYO_VARIANT=$(1) $(INCFLAGS) -fPIC -c $$< -o $$@

$(BUILD)/$(2).so: $(2)_shim.c $(BUILD)/$(2)_mayo.o $(BUILD)/$(2)_arith.o $(BUILD)/libmayo_common.a | $(BUILD)
	$(CC) $(CFLAGS) $(SIMD_FLAGS) $(DEFINES) -DMAYO_VARIANT=$(1) $(INCFLAGS) -I../.. \
	    -fPIC -shared -o $$@ $(2)_shim.c $(BUILD)/$(2)_mayo.o $(BUILD)/$(2)_arith.o \
	    $(BUILD)/libmayo_common.a
endef

$(eval $(call VARIANT_RULES,MAYO_1,mayo1))
$(eval $(call VARIANT_RULES,MAYO_2,mayo2))
$(eval $(call VARIANT_RULES,MAYO_3,mayo3))
$(eval $(call VARIANT_RULES,MAYO_5,mayo5))

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
