CC      ?= cc
BUILD    = build
SRC      = ref/src

BASE_CFLAGS := -O3 -fPIC -DNDEBUG -Wall -Wextra -Wno-unused-parameter \
               -Wno-sign-compare -DSNOVA_OPT=opt -mavx2 -maes

PARAMS := \
    snova_24_5_16_4   snova_28_5_19_4   snova_48_17_16_2  snova_48_16_19_2  \
    snova_28_4_16_4x5 snova_28_4_19_4x5                                      \
    snova_40_7_19_4   snova_37_8_16_4   snova_72_25_16_2                     \
    snova_38_5_16_4x5 snova_38_5_19_4x5                                      \
    snova_50_9_19_4   snova_60_10_16_4  snova_97_33_16_2

SOS        := $(addprefix $(BUILD)/, $(addsuffix .so, $(PARAMS)))
SHIM_FILES := $(addsuffix _shim.c, $(PARAMS))

# Per-variant SNOVA parameter flags (v, o, q, l, and optionally r for rectangular)
pf_snova_24_5_16_4   := -DSNOVA_v=24 -DSNOVA_o=5  -DSNOVA_q=16 -DSNOVA_l=4
pf_snova_28_5_19_4   := -DSNOVA_v=28 -DSNOVA_o=5  -DSNOVA_q=19 -DSNOVA_l=4
pf_snova_48_17_16_2  := -DSNOVA_v=48 -DSNOVA_o=17 -DSNOVA_q=16 -DSNOVA_l=2
pf_snova_48_16_19_2  := -DSNOVA_v=48 -DSNOVA_o=16 -DSNOVA_q=19 -DSNOVA_l=2
pf_snova_28_4_16_4x5 := -DSNOVA_v=28 -DSNOVA_o=4  -DSNOVA_q=16 -DSNOVA_l=4 -DSNOVA_r=5
pf_snova_28_4_19_4x5 := -DSNOVA_v=28 -DSNOVA_o=4  -DSNOVA_q=19 -DSNOVA_l=4 -DSNOVA_r=5
pf_snova_40_7_19_4   := -DSNOVA_v=40 -DSNOVA_o=7  -DSNOVA_q=19 -DSNOVA_l=4
pf_snova_37_8_16_4   := -DSNOVA_v=37 -DSNOVA_o=8  -DSNOVA_q=16 -DSNOVA_l=4
pf_snova_72_25_16_2  := -DSNOVA_v=72 -DSNOVA_o=25 -DSNOVA_q=16 -DSNOVA_l=2
pf_snova_38_5_16_4x5 := -DSNOVA_v=38 -DSNOVA_o=5  -DSNOVA_q=16 -DSNOVA_l=4 -DSNOVA_r=5
pf_snova_38_5_19_4x5 := -DSNOVA_v=38 -DSNOVA_o=5  -DSNOVA_q=19 -DSNOVA_l=4 -DSNOVA_r=5
pf_snova_50_9_19_4   := -DSNOVA_v=50 -DSNOVA_o=9  -DSNOVA_q=19 -DSNOVA_l=4
pf_snova_60_10_16_4  := -DSNOVA_v=60 -DSNOVA_o=10 -DSNOVA_q=16 -DSNOVA_l=4
pf_snova_97_33_16_2  := -DSNOVA_v=97 -DSNOVA_o=33 -DSNOVA_q=16 -DSNOVA_l=2

# Sources compiled into each per-param static library.
# rng.c excluded — shim provides randombytes via getrandom().
SRCS := $(SRC)/snova_opt.c $(SRC)/symmetric.c $(SRC)/sign.c $(SRC)/aes.c

.PHONY: all clean check_ref

all: check_ref $(SHIM_FILES) $(SOS)

check_ref:
	@if [ ! -f $(SRC)/snova.h ]; then \
	    echo "ERROR: bench/schemes/snova/ref/ not populated." >&2; \
	    echo "Run: git submodule update --init bench/schemes/snova/ref" >&2; \
	    exit 1; \
	fi

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(BUILD):
	mkdir -p $@

# ---------------------------------------------------------------------------
# Rule template — instantiated once per param set via $(eval $(call ...))
# ---------------------------------------------------------------------------
# $(1) = param name

define SNOVA_RULES

cflags_$(1) := $(BASE_CFLAGS) $(pf_$(1)) -I$(SRC)
objs_$(1)   := $(patsubst $(SRC)/%.c,$(BUILD)/$(1)/%.o,$(SRCS))
lib_$(1)    := $(BUILD)/$(1)/libsnova.a

$(BUILD)/$(1):
	mkdir -p $$@

$$(objs_$(1)): $(BUILD)/$(1)/%.o : $(SRC)/%.c | $(BUILD)/$(1)
	$$(CC) $$(cflags_$(1)) -c $$< -o $$@

$$(lib_$(1)): $$(objs_$(1)) | $(BUILD)/$(1)
	ar rcs $$@ $$^

$(BUILD)/$(1).so: $(1)_shim.c $$(lib_$(1)) | $(BUILD)
	$$(CC) $$(cflags_$(1)) -shared $$< $$(lib_$(1)) -o $$@

endef

$(foreach P,$(PARAMS),$(eval $(call SNOVA_RULES,$(P))))

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
