include ../../../bench-common/flags.mk
CC     ?= cc
BUILD   = build
REF     = ref/optimized_implementation/kem

CFLAGS := -Wall -Wextra -mavx2 -mbmi2 -mpopcnt $(BENCH_CFLAGS) -fPIC
INC    := -I$(REF)/include -I$(REF)/include/keccak -I$(REF)/include/mul

MODES = 1 3 5
SETS  = smaugt1 smaugt3 smaugt5 timer
SHIM_FILES := $(addsuffix _shim.c, $(SETS))

SRCS      := $(addprefix $(REF)/src/, dg.c pack.c poly.c key.c ciphertext.c \
             hwt.c kem.c io.c indcpa.c hash.c verify.c cbd.c)
SRCS_RNG  := $(REF)/src/randombytes.c
SRCS_FIPS202 := $(addprefix $(REF)/src/, keccak/KeccakP-1600-times4-SIMD256.c \
             keccak/KeccakP-1600-AVX2.S fips202x4.c fips202.c)
SRCS_NTT  := $(addprefix $(REF)/src/mul/, basemul.S consts7681.c consts10753.c \
             invntt.S ntt.S ntt.c)

# TiMER: same file layout in its own tree (additional_implementation, AVX2).
TREF  := ref/additional_implementation/TiMER_optimized_implementation/kem
TINC  := -I$(TREF)/include -I$(TREF)/include/keccak -I$(TREF)/include/mul
TSRCS := $(addprefix $(TREF)/src/, dg.c pack.c poly.c key.c ciphertext.c \
         hwt.c kem.c io.c indcpa.c hash.c verify.c cbd.c randombytes.c \
         keccak/KeccakP-1600-times4-SIMD256.c keccak/KeccakP-1600-AVX2.S \
         fips202x4.c fips202.c) \
         $(addprefix $(TREF)/src/mul/, basemul.S consts7681.c consts10753.c \
         invntt.S ntt.S ntt.c)

.PHONY: all clean
all: $(SHIM_FILES) $(addprefix $(BUILD)/, $(addsuffix .so, $(SETS)))

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(BUILD):
	mkdir -p $@

# combo: $(1)=scheme name (smaugt1)  $(2)=SMAUG_MODE value (1)
define combo
$$(BUILD)/$(1).so: $(1)_shim.c $(SRCS) $(SRCS_RNG) $(SRCS_FIPS202) $(SRCS_NTT) ../../scheme.h | $$(BUILD)
	$$(CC) $$(CFLAGS) -DSMAUG_MODE=$(2) $$(INC) \
	    -shared -o $$@ $(1)_shim.c $(SRCS) $(SRCS_RNG) $(SRCS_FIPS202) $(SRCS_NTT)
endef

$(eval $(call combo,smaugt1,1))
$(eval $(call combo,smaugt3,3))
$(eval $(call combo,smaugt5,5))

$(BUILD)/timer.so: timer_shim.c $(TSRCS) ../../scheme.h | $(BUILD)
	$(CC) $(CFLAGS) -DTiMER_MODE=1 $(TINC) -shared -o $@ timer_shim.c $(TSRCS)

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
