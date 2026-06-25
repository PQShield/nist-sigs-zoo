CC      ?= cc
BUILD    = build
REF      = ref

# HQC's reference uses the bundled lib/fips202 for SHA3/SHAKE — no OpenSSL.
# We build the AVX2 implementation (HQC_ARCH=x86_64, HQC_X86_IMPL=avx256).
# Flags + defines mirror the upstream top-level CMakeLists.txt for x86_64.
HQC_DEFS := -DHQC_ARCH_X86_64=1 -DHQC_ARCH_X86_64_AVX256=1
CFLAGS   := -O3 -fomit-frame-pointer -fPIC -std=c99 -Wall -Wextra \
            -mavx -mavx2 -mbmi -mpclmul $(HQC_DEFS)

SHIM_FILES := hqc128_shim.c hqc192_shim.c hqc256_shim.c

.PHONY: all clean
all: $(SHIM_FILES) $(BUILD)/hqc128.so $(BUILD)/hqc192.so $(BUILD)/hqc256.so

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

# Per-variant build. $(1)=output stem (hqc128…), $(2)=upstream variant (hqc-1…).
# Source sets mirror src/CMakeLists.txt with HQC_ARCH=x86_64, HQC_X86_IMPL=avx256:
#   common       : src/common/*.c                 (incl. kem.c)
#   arch-common  : src/x86_64/common/*.c          (hqc.c, parsing.c)
#   common-var   : src/x86_64/common/<var>/*.c    (gf2x.c + parameters.h)
#   platform     : src/x86_64/avx256/*.c          (gf.c, reed_muller.c, vector.c)
#   variant      : src/x86_64/avx256/<var>/*.c    (reed_solomon.c)
#   api-var      : src/common/<var>/*.c           (api.h only; no .c, glob is safe)
#   fips202      : lib/fips202/*.c
# Include dirs give every .o the correct per-variant api.h / parameters.h.
define hqc_rules
$(1)_SRC := $$(wildcard \
    $$(REF)/src/common/*.c \
    $$(REF)/src/x86_64/common/*.c \
    $$(REF)/src/x86_64/common/$(2)/*.c \
    $$(REF)/src/x86_64/avx256/*.c \
    $$(REF)/src/x86_64/avx256/$(2)/*.c \
    $$(REF)/src/common/$(2)/*.c \
    $$(REF)/lib/fips202/*.c)
$(1)_INC := \
    -I$$(REF)/src/common \
    -I$$(REF)/src/common/$(2) \
    -I$$(REF)/src/x86_64/common \
    -I$$(REF)/src/x86_64/common/$(2) \
    -I$$(REF)/src/x86_64/avx256 \
    -I$$(REF)/src/x86_64/avx256/$(2) \
    -I$$(REF)/lib/fips202
$(1)_OBJ := $$(patsubst $$(REF)/%.c,$$(BUILD)/$(1)/%.o,$$($(1)_SRC))

$$(BUILD)/$(1)/%.o: $$(REF)/%.c
	@mkdir -p $$(@D)
	$$(CC) $$(CFLAGS) $$($(1)_INC) -c $$< -o $$@

$$(BUILD)/$(1).so: $(1)_shim.c $$($(1)_OBJ) ../../scheme.h | $$(BUILD)
	$$(CC) $$(CFLAGS) $$($(1)_INC) -shared -o $$@ $(1)_shim.c $$($(1)_OBJ)
endef

$(eval $(call hqc_rules,hqc128,hqc-1))
$(eval $(call hqc_rules,hqc192,hqc-3))
$(eval $(call hqc_rules,hqc256,hqc-5))

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
