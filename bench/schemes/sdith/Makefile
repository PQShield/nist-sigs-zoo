CC      ?= cc
BUILD    = build
IMPL_BASE = ref/Optimized_Implementation

# Common flags for all variants
BASE_CFLAGS := -O3 -fPIC -DNDEBUG -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare

# Hypercube: uses AVX2 field arithmetic
HC_CFLAGS   := $(BASE_CFLAGS) -DAVX2 -mavx2 -mpclmul -mgfni -mavx -maes

# Threshold: uses AVX2 (polytable GF256 file) + 4-way SHA3
TH_CFLAGS   := $(BASE_CFLAGS) -mavx2 -mpclmul -mgfni -mavx -maes -DPARAM_THRESHOLD_NFPR -DHASHX4 -DXOFX4

# Per-variant extra defines
xtra_sdith_hypercube_cat1_gf256 := -DCAT_1
xtra_sdith_hypercube_cat1_p251  := -DCAT_1
xtra_sdith_hypercube_cat3_gf256 := -DCAT_3
xtra_sdith_hypercube_cat3_p251  := -DCAT_3
xtra_sdith_hypercube_cat5_gf256 := -DCAT_5
xtra_sdith_hypercube_cat5_p251  := -DCAT_5
xtra_sdith_threshold_cat1_gf256 := -DPARAM_GF256 -DPARAM_L1
xtra_sdith_threshold_cat1_p251  := -DPARAM_GF251 -DPARAM_L1
xtra_sdith_threshold_cat3_gf256 := -DPARAM_GF256 -DPARAM_L3
xtra_sdith_threshold_cat3_p251  := -DPARAM_GF251 -DPARAM_L3
xtra_sdith_threshold_cat5_gf256 := -DPARAM_GF256 -DPARAM_L5
xtra_sdith_threshold_cat5_p251  := -DPARAM_GF251 -DPARAM_L5

HYPER_PARAMS := \
    sdith_hypercube_cat1_gf256 sdith_hypercube_cat1_p251 \
    sdith_hypercube_cat3_gf256 sdith_hypercube_cat3_p251 \
    sdith_hypercube_cat5_gf256 sdith_hypercube_cat5_p251

THRESH_PARAMS := \
    sdith_threshold_cat1_gf256 sdith_threshold_cat1_p251 \
    sdith_threshold_cat3_gf256 sdith_threshold_cat3_p251 \
    sdith_threshold_cat5_gf256 sdith_threshold_cat5_p251

ALL_PARAMS  := $(HYPER_PARAMS) $(THRESH_PARAMS)
SOS         := $(addprefix $(BUILD)/, $(addsuffix .so, $(ALL_PARAMS)))
HYPER_SHIMS := $(addsuffix _shim.c, $(HYPER_PARAMS))
THRESH_SHIMS:= $(addsuffix _shim.c, $(THRESH_PARAMS))
SHIM_FILES  := $(HYPER_SHIMS) $(THRESH_SHIMS)

.PHONY: all clean check_ref

all: check_ref $(SHIM_FILES) $(SOS)

check_ref:
	@if [ ! -f $(IMPL_BASE)/Hypercube_Variant/sdith_hypercube_cat1_gf256/api.h ]; then \
	    echo "ERROR: bench/schemes/sdith/ref/ not populated." >&2; \
	    echo "Run: git submodule update --init bench/schemes/sdith/ref" >&2; \
	    exit 1; \
	fi

$(HYPER_SHIMS): shim_hypercube_template.c params_hypercube.tsv
	python3 ../../gen_shims.py shim_hypercube_template.c params_hypercube.tsv .

$(THRESH_SHIMS): shim_threshold_template.c params_threshold.tsv
	python3 ../../gen_shims.py shim_threshold_template.c params_threshold.tsv .

$(BUILD):
	mkdir -p $@

# ---------------------------------------------------------------------------
# Rule templates — instantiated once per param set via $(eval $(call ...))
# ---------------------------------------------------------------------------

# SHA3 C sources in sha3/ (flat — no subdir, avx2 SIMD handled separately below)
SHA3_C_NAMES := sha3/KeccakHash.c sha3/KeccakSponge.c \
                sha3/KeccakHashtimes4.c sha3/KeccakSpongetimes4.c

# $(1) = param name   $(2) = variant base dir (Hypercube_Variant|Threshold_Variant)
# $(3) = base CFLAGS  $(4) = sign.c excluded? ("" or "yes")
define SDITH_RULES

impl_$(1)   := $(IMPL_BASE)/$(2)/$(1)
iflags_$(1) := -I$$(impl_$(1)) -I$$(impl_$(1))/sha3 -I$$(impl_$(1))/sha3/avx2
cflags_$(1) := $(3) $$(xtra_$(1)) $$(iflags_$(1))

# Main .c sources (Threshold excludes sign.c to avoid ull/size_t ABI conflict)
ifeq ($(4),yes)
main_c_$(1) := $$(filter-out $$(impl_$(1))/sign.c,$$(wildcard $$(impl_$(1))/*.c))
else
main_c_$(1) := $$(wildcard $$(impl_$(1))/*.c)
endif

sha3_c_$(1)   := $$(addprefix $$(impl_$(1))/, $(SHA3_C_NAMES))
sha3_asm_$(1)    := $$(impl_$(1))/sha3/avx2/KeccakP-1600-AVX2.S
sha3_simd256_$(1):= $$(impl_$(1))/sha3/avx2/KeccakP-1600-times4-SIMD256.c

main_objs_$(1)      := $$(patsubst $$(impl_$(1))/%.c,$(BUILD)/$(1)/%.o,$$(main_c_$(1)))
sha3_c_objs_$(1)    := $$(patsubst $$(impl_$(1))/sha3/%.c,$(BUILD)/$(1)/sha3_%.o,$$(sha3_c_$(1)))
sha3_simd256_obj_$(1):= $(BUILD)/$(1)/sha3_times4_SIMD256.o
sha3_asm_obj_$(1)   := $(BUILD)/$(1)/sha3_avx2_asm.o
lib_$(1)            := $(BUILD)/$(1)/libsdith.a

# Build dir
$(BUILD)/$(1):
	mkdir -p $$@

# Compile main sources
$$(main_objs_$(1)): $(BUILD)/$(1)/%.o : $$(impl_$(1))/%.c | $(BUILD)/$(1)
	$$(CC) $$(cflags_$(1)) -c $$< -o $$@

# Compile SHA3 C sources (flat sha3_ prefix in output names)
$(BUILD)/$(1)/sha3_%.o : $$(impl_$(1))/sha3/%.c | $(BUILD)/$(1)
	$$(CC) $(BASE_CFLAGS) $$(iflags_$(1)) -c $$< -o $$@

# Compile SHA3 AVX2 times4 SIMD C source
$$(sha3_simd256_obj_$(1)): $$(sha3_simd256_$(1)) | $(BUILD)/$(1)
	$$(CC) $(BASE_CFLAGS) $$(iflags_$(1)) -mavx2 -c $$< -o $$@

# Compile SHA3 AVX2 assembly
$$(sha3_asm_obj_$(1)): $$(sha3_asm_$(1)) | $(BUILD)/$(1)
	$$(CC) -c $$< -o $$@

# Static library
$$(lib_$(1)): $$(main_objs_$(1)) $$(sha3_c_objs_$(1)) $$(sha3_simd256_obj_$(1)) $$(sha3_asm_obj_$(1)) | $(BUILD)/$(1)
	ar rcs $$@ $$^

# Shared library (shim + libsdith)
$(BUILD)/$(1).so: $(1)_shim.c $$(lib_$(1)) | $(BUILD)
	$$(CC) $$(cflags_$(1)) -shared $$< $$(lib_$(1)) -o $$@

endef

# Instantiate rules for all 12 param sets
$(foreach P,$(HYPER_PARAMS),$(eval $(call SDITH_RULES,$(P),Hypercube_Variant,$(HC_CFLAGS),)))
$(foreach P,$(THRESH_PARAMS),$(eval $(call SDITH_RULES,$(P),Threshold_Variant,$(TH_CFLAGS),yes)))

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
