include ../../../bench-common/flags.mk
CC      ?= cc
BUILD    = build
SRC      = ref/src

# Upstream's default build: OPT=OPT (SNOVA_OPT=10, snova_opt_16.c) with
# -march=native (in the global flag set, flags.mk), which enables the GFNI and AVX-512
# Keccak paths where the CPU has them (snova_opt_16.c / symmetric.c test __GFNI__ / __AVX512F__).
BASE_CFLAGS := -Wall -Wextra -Wno-unused-parameter \
               -Wno-sign-compare -DSNOVA_OPT=10 $(BENCH_CFLAGS) -fPIC

# Alternative parameter sets (spec Table 6), named snova_<v>_<o>_<q>_<l>_<r>_<m1>_<m2>.
# The l = 2 sets have 48-byte secret keys in the upstream code (HASH_PK defaults to l > 2),
# although spec Table 6 gives 96 bytes for every set; params.tsv uses the code size.
# q = 16 sets use the same optimised build as the recommended sets; the
# odd-characteristic sets need upstream's reference implementation (OPT=REF:
# snova_ref.c + symmetric_ref.c, SNOVA_OPT=0), since snova_opt_16.c and the
# other optimised files only support q = 16.
ALT_Q16_PARAMS := \
    snova_27_3_16_4_7_5_84 \
    snova_27_4_16_4_5_5_80 \
    snova_50_17_16_2_2_17_68 \
    snova_27_4_16_5_5_4_100 \
    snova_33_5_16_4_5_6_100 \
    snova_76_25_16_2_2_25_100 \
    snova_104_33_16_2_2_33_132
ALT_REF_PARAMS := \
    snova_27_4_19_4_6_5_96 \
    snova_27_5_13_4_4_5_80 \
    snova_39_5_11_4_6_7_120 \
    snova_38_5_19_4_6_7_120 \
    snova_40_5_19_5_6_6_150 \
    snova_47_6_19_4_6_9_144
ALT_PARAMS := $(ALT_Q16_PARAMS) $(ALT_REF_PARAMS)

PARAMS := \
    snova_i_k   snova_i_b   snova_i_s   \
    snova_iii_k snova_iii_b snova_iii_s \
    snova_v_k   snova_v_b   snova_v_s   \
    $(ALT_PARAMS)

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
pf_snova_27_3_16_4_7_5_84 := -DSNOVA_v=27 -DSNOVA_o=3 -DSNOVA_q=16 -DSNOVA_l=4 -DSNOVA_r=7 -DSNOVA_m1=5 -DSNOVA_NAME=SNOVA_27_3_16_4_7_5_84
pf_snova_27_4_16_4_5_5_80 := -DSNOVA_v=27 -DSNOVA_o=4 -DSNOVA_q=16 -DSNOVA_l=4 -DSNOVA_r=5 -DSNOVA_m1=5 -DSNOVA_NAME=SNOVA_27_4_16_4_5_5_80
pf_snova_27_4_19_4_6_5_96 := -DSNOVA_v=27 -DSNOVA_o=4 -DSNOVA_q=19 -DSNOVA_l=4 -DSNOVA_r=6 -DSNOVA_m1=5 -DSNOVA_NAME=SNOVA_27_4_19_4_6_5_96
pf_snova_27_5_13_4_4_5_80 := -DSNOVA_v=27 -DSNOVA_o=5 -DSNOVA_q=13 -DSNOVA_l=4 -DSNOVA_r=4 -DSNOVA_m1=5 -DSNOVA_NAME=SNOVA_27_5_13_4_4_5_80
pf_snova_50_17_16_2_2_17_68 := -DSNOVA_v=50 -DSNOVA_o=17 -DSNOVA_q=16 -DSNOVA_l=2 -DSNOVA_r=2 -DSNOVA_m1=17 -DSNOVA_NAME=SNOVA_50_17_16_2_2_17_68
pf_snova_27_4_16_5_5_4_100 := -DSNOVA_v=27 -DSNOVA_o=4 -DSNOVA_q=16 -DSNOVA_l=5 -DSNOVA_r=5 -DSNOVA_m1=4 -DSNOVA_NAME=SNOVA_27_4_16_5_5_4_100
pf_snova_33_5_16_4_5_6_100 := -DSNOVA_v=33 -DSNOVA_o=5 -DSNOVA_q=16 -DSNOVA_l=4 -DSNOVA_r=5 -DSNOVA_m1=6 -DSNOVA_NAME=SNOVA_33_5_16_4_5_6_100
pf_snova_39_5_11_4_6_7_120 := -DSNOVA_v=39 -DSNOVA_o=5 -DSNOVA_q=11 -DSNOVA_l=4 -DSNOVA_r=6 -DSNOVA_m1=7 -DSNOVA_NAME=SNOVA_39_5_11_4_6_7_120
pf_snova_38_5_19_4_6_7_120 := -DSNOVA_v=38 -DSNOVA_o=5 -DSNOVA_q=19 -DSNOVA_l=4 -DSNOVA_r=6 -DSNOVA_m1=7 -DSNOVA_NAME=SNOVA_38_5_19_4_6_7_120
pf_snova_76_25_16_2_2_25_100 := -DSNOVA_v=76 -DSNOVA_o=25 -DSNOVA_q=16 -DSNOVA_l=2 -DSNOVA_r=2 -DSNOVA_m1=25 -DSNOVA_NAME=SNOVA_76_25_16_2_2_25_100
pf_snova_40_5_19_5_6_6_150 := -DSNOVA_v=40 -DSNOVA_o=5 -DSNOVA_q=19 -DSNOVA_l=5 -DSNOVA_r=6 -DSNOVA_m1=6 -DSNOVA_NAME=SNOVA_40_5_19_5_6_6_150
pf_snova_47_6_19_4_6_9_144 := -DSNOVA_v=47 -DSNOVA_o=6 -DSNOVA_q=19 -DSNOVA_l=4 -DSNOVA_r=6 -DSNOVA_m1=9 -DSNOVA_NAME=SNOVA_47_6_19_4_6_9_144
pf_snova_104_33_16_2_2_33_132 := -DSNOVA_v=104 -DSNOVA_o=33 -DSNOVA_q=16 -DSNOVA_l=2 -DSNOVA_r=2 -DSNOVA_m1=33 -DSNOVA_NAME=SNOVA_104_33_16_2_2_33_132

# Sources compiled into each per-param static library. Round 3 replaced the
# generic snova_opt.c with a q=16-specific snova_opt_16.c (all recommended
# sets use q=16); it auto-detects AVX2/GFNI via __AVX2__/__GFNI__ (from -march=native).
# rng.c excluded — shim provides randombytes via getrandom().
SRCS := $(SRC)/snova_opt_16.c $(SRC)/symmetric.c $(SRC)/sign.c $(SRC)/aes.c
REF_SRCS := $(SRC)/snova_ref.c $(SRC)/symmetric_ref.c $(SRC)/sign.c $(SRC)/aes.c
REF_CFLAGS := $(subst -DSNOVA_OPT=10,-DSNOVA_OPT=0,$(BASE_CFLAGS))

# Per-set sources and base flags: reference build for the odd-q sets.
$(foreach P,$(ALT_REF_PARAMS),$(eval srcs_$(P) := $(REF_SRCS))$(eval base_$(P) := $(REF_CFLAGS)))

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

srcs_$(1)   ?= $(SRCS)
base_$(1)   ?= $(BASE_CFLAGS)
cflags_$(1) := $$(base_$(1)) $(pf_$(1)) -I$(SRC)
objs_$(1)   := $$(patsubst $(SRC)/%.c,$(BUILD)/$(1)/%.o,$$(srcs_$(1)))
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
