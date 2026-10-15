CC      ?= cc
# Upstream's Makefile flags (Optimized_Implementation/aes_aes/*/Makefile) minus its
# -Werror/-pedantic warning set, plus -fPIC. djbsort is compiled separately with
# -fwrapv, as upstream does.
CFLAGS  := -std=c99 -O3 -funroll-loops -march=native -mavx2 -mpclmul -msse4.2 -maes -fPIC -fno-semantic-interposition \
           -Wall -Wextra -Wno-missing-prototypes -Wno-sign-compare \
           -Wno-unused-but-set-variable -Wno-unused-parameter
# -fno-semantic-interposition: with -fPIC -shared GCC otherwise may not inline or
# specialise calls between the upstream's global functions (they could be interposed);
# (RYDE-1-Short signing was 4x slower without it; harmless here.)
BUILD   := build
# PERK v2.2.0 is released only as a zip on pqc-perk.org: .source holds its URL
# (line 1) and sha256 (line 2); it is fetched into build/dl and unpacked into build/src.
URL      := $(shell sed -n 1p .source)
ZIP      := $(BUILD)/dl/perk-v2.2.0.zip
OPT      := $(BUILD)/src/perk-v2.2.0/Optimized_Implementation/aes_aes
UNZIPPED := $(BUILD)/src/.unzipped

.DEFAULT_GOAL := all
.PHONY: all clean

PARAM_SETS :=

# Not compiled: src/main.c, src/PQCgenKAT_sign.c, lib/randombytes/rng.c (NIST DRBG,
# needs OpenSSL). Upstream links -lgmp but no source uses GMP.
SRCS = crypto_memset.c ev_expand.c gf_arithmetic.c ggm_tree.c keygen.c parsing.c \
       permutation.c prove_pkp.c signature.c sign.c symmetric.c symmetric_times4.c \
       verbose.c verify.c verify_ev_expand.c verify_pkp.c voles.c
XKCP = KeccakHash.c KeccakHashtimes4.c KeccakSponge.c KeccakSpongetimes4.c SimpleFIPS202.c \
       KeccakP-1600-times4-SIMD256.c KeccakP-1600-AVX2.s
INCS = src lib/XKCP lib/randombytes lib/djbsort

# $(1) = file stem, $(2) = upstream directory
define PERK_RULES
PARAM_SETS += $(1)
$(BUILD)/obj/$(1)/%.o: $(UNZIPPED)
	mkdir -p $$(@D)
	$(CC) $(CFLAGS) -fwrapv -I$(OPT)/$(2)/lib/djbsort -c -o $$@ $(OPT)/$(2)/lib/djbsort/$$*.c
$(BUILD)/$(1).so: $(1)_shim.c $(UNZIPPED) $(BUILD)/obj/$(1)/djbsort.o $(BUILD)/obj/$(1)/sort.o
	$(CC) $(CFLAGS) $(addprefix -I$(OPT)/$(2)/,$(INCS)) -I../.. -shared -o $$@ \
	    $(1)_shim.c $(addprefix $(OPT)/$(2)/src/,$(SRCS)) $(addprefix $(OPT)/$(2)/lib/XKCP/,$(XKCP)) \
	    $(BUILD)/obj/$(1)/djbsort.o $(BUILD)/obj/$(1)/sort.o
endef

$(eval $(call PERK_RULES,perk1s,perk-1-short))
$(eval $(call PERK_RULES,perk1f,perk-1-fast))
$(eval $(call PERK_RULES,perk3s,perk-3-short))
$(eval $(call PERK_RULES,perk3f,perk-3-fast))
$(eval $(call PERK_RULES,perk5s,perk-5-short))
$(eval $(call PERK_RULES,perk5f,perk-5-fast))

SOS        := $(addprefix $(BUILD)/,$(addsuffix .so,$(PARAM_SETS)))
SHIM_FILES := $(addsuffix _shim.c,$(PARAM_SETS))

all: $(SHIM_FILES) $(SOS)

$(SHIM_FILES) &: shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(ZIP): .source
	mkdir -p $(@D)
	curl -fsSL --retry 3 -o $@.tmp '$(URL)'
	cd $(@D) && sed -n 2p ../../.source | sed 's/perk-v2.2.0.zip$$/perk-v2.2.0.zip.tmp/' | sha256sum -c - \
	    || { echo "SHA-256 mismatch for $@" >&2; rm -f $(@F).tmp; exit 1; }
	mv $@.tmp $@

$(UNZIPPED): $(ZIP)
	rm -rf $(BUILD)/src
	mkdir -p $(BUILD)/src
	python3 -m zipfile -e $(ZIP) $(BUILD)/src
	touch $@

clean:
	rm -rf $(BUILD)/src $(BUILD)/obj $(BUILD)/*.so
	rm -f $(SHIM_FILES)
