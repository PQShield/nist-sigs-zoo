.DEFAULT_GOAL := all
include ../../../bench-common/flags.mk
CC      ?= cc
BUILD    = build

# SNOVA Round 1 is only available as the NIST round-1 submission package (the
# PQCLAB-SNOVA/SNOVA git history starts after round 1). .source holds its URL (line 1) and
# sha256 (line 2); it is fetched into build/dl and unpacked into build/src.
URL      := $(shell sed -n 1p .source)
ZIP      := $(BUILD)/dl/SNOVAnoKATs-submission.zip
OPT_BASE := $(BUILD)/src/SNOVA/Optimized_Implementation
UNZIPPED := $(BUILD)/src/.unzipped

# Optimized_Implementation, one directory per parameter set and variant (the directories
# differ only in their makefile). Variant: ssk (seed-type secret key, SK_IS_SEED=1), the
# variant whose spec Table 5 numbers data/schemes/SNOVA.yaml "Round 1" quotes.
# TURBO=1 (table-based GF(16) arithmetic) is the Optimized_Implementation makefile's
# setting. SHAKE256 and AES-128-CTR come from OpenSSL's libcrypto, as upstream links them.
OSSL_PREFIX := $(shell pkg-config --variable=prefix openssl 2>/dev/null)
ifeq ($(OSSL_PREFIX),)
  OSSL_PREFIX := /usr
endif
OSSL_LDFLAGS := -lcrypto

BASE_CFLAGS := -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare \
               -Dsk_is_seed=1 -DTURBO=1 $(BENCH_CFLAGS) -fPIC

# v o l (q = 16).
PARAMS := \
    snova_r1_28_17_2 snova_r1_25_8_3 snova_r1_24_5_4 \
    snova_r1_43_25_2 snova_r1_49_11_3 snova_r1_37_8_4 \
    snova_r1_61_33_2 snova_r1_66_15_3 snova_r1_60_10_4

SOS        := $(addprefix $(BUILD)/, $(addsuffix .so, $(PARAMS)))
SHIM_FILES := $(addsuffix _shim.c, $(PARAMS))

# Per-set values from the name: snova_r1_<v>_<o>_<l>.
snova_w = $(subst _, ,$(1))
snova_v = $(word 3,$(call snova_w,$(1)))
snova_o = $(word 4,$(call snova_w,$(1)))
snova_l = $(word 5,$(call snova_w,$(1)))
snova_dir = $(OPT_BASE)/SNOVA-$(call snova_v,$(1))-$(call snova_o,$(1))-16-$(call snova_l,$(1))-ssk

.PHONY: all clean fetch

all: $(SHIM_FILES) $(SOS)

fetch: $(ZIP)

$(SHIM_FILES) &: shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(ZIP): .source
	mkdir -p $(@D)
	curl -fsSL --retry 3 -o $@.tmp '$(URL)'
	cd $(@D) && sed -n 2p ../../.source | sed 's/submission.zip$$/submission.zip.tmp/' | sha256sum -c - \
	    || { echo "SHA-256 mismatch for $@" >&2; rm -f $(@F).tmp; exit 1; }
	mv $@.tmp $@

$(UNZIPPED): $(ZIP)
	rm -rf $(BUILD)/src
	mkdir -p $(BUILD)/src
	python3 -m zipfile -e $(ZIP) $(BUILD)/src
	touch $@

define SNOVA_RULES
dir_$(1)    := $(call snova_dir,$(1))
cflags_$(1) := $(BASE_CFLAGS) -Dv_SNOVA=$(call snova_v,$(1)) -Do_SNOVA=$(call snova_o,$(1)) \
               -Dl_SNOVA=$(call snova_l,$(1)) \
               -DCRYPTO_ALGNAME='"SNOVA_$(call snova_v,$(1))_$(call snova_o,$(1))_$(call snova_l,$(1))"' \
               -I$(OSSL_PREFIX)/include -I$$(dir_$(1))
objs_$(1)   := $(BUILD)/$(1)/snova.o $(BUILD)/$(1)/sign.o

$(BUILD)/$(1)/%.o: $(UNZIPPED)
	@mkdir -p $$(@D)
	$$(CC) $$(cflags_$(1)) -c $$(dir_$(1))/$$*.c -o $$@

$(BUILD)/$(1).so: $(1)_shim.c $$(objs_$(1))
	$$(CC) $$(cflags_$(1)) -shared $$< $$(objs_$(1)) -L$(OSSL_PREFIX)/lib $(OSSL_LDFLAGS) -o $$@
endef

$(foreach P,$(PARAMS),$(eval $(call SNOVA_RULES,$(P))))

# Keeps build/dl (the downloaded package).
clean:
	rm -rf $(BUILD)/src $(BUILD)/snova_r1_* $(BUILD)/*.so
	rm -f $(SHIM_FILES)
