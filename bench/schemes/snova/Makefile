CC      ?= cc
BUILD    = build
SRC      = ref/src

BASE_CFLAGS := -O3 -fPIC -DNDEBUG -Wall -Wextra -Wno-unused-parameter \
               -Wno-sign-compare -DSNOVA_OPT=opt -mavx2 -maes

PARAMS := \
    snova_i_k   snova_i_b   snova_i_s   \
    snova_iii_k snova_iii_b snova_iii_s \
    snova_v_k   snova_v_b   snova_v_s

SOS        := $(addprefix $(BUILD)/, $(addsuffix .so, $(PARAMS)))
SHIM_FILES := $(addsuffix _shim.c, $(PARAMS))

# Per-variant SNOVA round-3 parameter flags (v, o, q, l, r, m1); m2 is derived
# internally by the reference code as o*l*r. All recommended sets use q=16.
pf_snova_i_k   := -DSNOVA_v=29 -DSNOVA_o=3 -DSNOVA_q=16 -DSNOVA_l=4 -DSNOVA_r=8 -DSNOVA_m1=5 -DSNOVA_NAME=SNOVA_I_K
pf_snova_i_b   := -DSNOVA_v=27 -DSNOVA_o=4 -DSNOVA_q=16 -DSNOVA_l=4 -DSNOVA_r=6 -DSNOVA_m1=5 -DSNOVA_NAME=SNOVA_I_B
pf_snova_i_s   := -DSNOVA_v=27 -DSNOVA_o=5 -DSNOVA_q=16 -DSNOVA_l=4 -DSNOVA_r=4 -DSNOVA_m1=5 -DSNOVA_NAME=SNOVA_I_S
pf_snova_iii_k := -DSNOVA_v=38 -DSNOVA_o=4 -DSNOVA_q=16 -DSNOVA_l=4 -DSNOVA_r=8 -DSNOVA_m1=7 -DSNOVA_NAME=SNOVA_III_K
pf_snova_iii_b := -DSNOVA_v=38 -DSNOVA_o=5 -DSNOVA_q=16 -DSNOVA_l=4 -DSNOVA_r=6 -DSNOVA_m1=7 -DSNOVA_NAME=SNOVA_III_B
pf_snova_iii_s := -DSNOVA_v=38 -DSNOVA_o=6 -DSNOVA_q=16 -DSNOVA_l=4 -DSNOVA_r=5 -DSNOVA_m1=7 -DSNOVA_NAME=SNOVA_III_S
pf_snova_v_k   := -DSNOVA_v=40 -DSNOVA_o=4 -DSNOVA_q=16 -DSNOVA_l=5 -DSNOVA_r=8 -DSNOVA_m1=6 -DSNOVA_NAME=SNOVA_V_K
pf_snova_v_b   := -DSNOVA_v=40 -DSNOVA_o=5 -DSNOVA_q=16 -DSNOVA_l=5 -DSNOVA_r=6 -DSNOVA_m1=6 -DSNOVA_NAME=SNOVA_V_B
pf_snova_v_s   := -DSNOVA_v=40 -DSNOVA_o=6 -DSNOVA_q=16 -DSNOVA_l=5 -DSNOVA_r=5 -DSNOVA_m1=6 -DSNOVA_NAME=SNOVA_V_S

# Sources compiled into each per-param static library. Round 3 replaced the
# generic snova_opt.c with a q=16-specific snova_opt_16.c (all recommended
# sets use q=16); it auto-detects AVX2/GFNI via __AVX2__/__GFNI__ from BASE_CFLAGS.
# rng.c excluded — shim provides randombytes via getrandom().
SRCS := $(SRC)/snova_opt_16.c $(SRC)/symmetric.c $(SRC)/sign.c $(SRC)/aes.c

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
