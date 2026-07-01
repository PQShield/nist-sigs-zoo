CC     ?= cc
CFLAGS := -O3 -fomit-frame-pointer -fPIC -Wall -Wextra \
          -mavx2 -mpclmul -mbmi -march=native -mtune=native
BUILD   = build
HQC     = ref

# AVX2/AVX-256 optimised implementation. Sources span four roots; gf2x.c and
# reed_solomon.c are per-variant (variant-specific tables / carryless-mul code),
# the rest are shared. Basenames are unique, so each variant compiles into its
# own dir with variant-specific include paths (parameters.h, api.h).
SHARED_SRCS = src/common/code.c src/common/crypto_memset.c src/common/fft.c \
              src/common/kem.c src/common/symmetric.c \
              src/x86_64/common/hqc.c src/x86_64/common/parsing.c \
              src/x86_64/avx256/gf.c src/x86_64/avx256/reed_muller.c \
              src/x86_64/avx256/vector.c \
              lib/fips202/fips202.c

SHIM_FILES := hqc1_shim.c hqc3_shim.c hqc5_shim.c

.PHONY: all clean
all: $(SHIM_FILES) $(BUILD)/hqc1.so $(BUILD)/hqc3.so $(BUILD)/hqc5.so

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

# Per-variant (HQC-1/3/5) compilation — parameters.h is selected by include path.
define variant
INC$(1) = -I$(HQC)/src/common -I$(HQC)/src/common/hqc-$(1) \
          -I$(HQC)/src/x86_64/common -I$(HQC)/src/x86_64/common/hqc-$(1) \
          -I$(HQC)/src/x86_64/avx256 -I$(HQC)/src/x86_64/avx256/hqc-$(1) \
          -I$(HQC)/lib/fips202
SRCS$(1) = $$(SHARED_SRCS) \
           src/x86_64/common/hqc-$(1)/gf2x.c \
           src/x86_64/avx256/hqc-$(1)/reed_solomon.c
OBJ$(1) = $$(addprefix $$(BUILD)/hqc$(1)/, $$(notdir $$(SRCS$(1):.c=.o)))

$$(BUILD)/hqc$(1):
	mkdir -p $$@

$$(BUILD)/hqc$(1)/%.o: $(HQC)/src/common/%.c | $$(BUILD)/hqc$(1)
	$$(CC) $$(CFLAGS) $$(INC$(1)) -c $$< -o $$@
$$(BUILD)/hqc$(1)/%.o: $(HQC)/src/x86_64/common/%.c | $$(BUILD)/hqc$(1)
	$$(CC) $$(CFLAGS) $$(INC$(1)) -c $$< -o $$@
$$(BUILD)/hqc$(1)/%.o: $(HQC)/src/x86_64/common/hqc-$(1)/%.c | $$(BUILD)/hqc$(1)
	$$(CC) $$(CFLAGS) $$(INC$(1)) -c $$< -o $$@
$$(BUILD)/hqc$(1)/%.o: $(HQC)/src/x86_64/avx256/%.c | $$(BUILD)/hqc$(1)
	$$(CC) $$(CFLAGS) $$(INC$(1)) -c $$< -o $$@
$$(BUILD)/hqc$(1)/%.o: $(HQC)/src/x86_64/avx256/hqc-$(1)/%.c | $$(BUILD)/hqc$(1)
	$$(CC) $$(CFLAGS) $$(INC$(1)) -c $$< -o $$@
$$(BUILD)/hqc$(1)/%.o: $(HQC)/lib/fips202/%.c | $$(BUILD)/hqc$(1)
	$$(CC) $$(CFLAGS) $$(INC$(1)) -c $$< -o $$@

$$(BUILD)/hqc$(1)_shim.o: hqc$(1)_shim.c ../../scheme.h | $$(BUILD)
	$$(CC) $$(CFLAGS) -c $$< -o $$@

$$(BUILD)/hqc$(1).so: $$(BUILD)/hqc$(1)_shim.o $$(OBJ$(1))
	$$(CC) -shared -o $$@ $$^
endef

$(eval $(call variant,1))
$(eval $(call variant,3))
$(eval $(call variant,5))

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
