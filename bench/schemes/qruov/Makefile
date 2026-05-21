.DEFAULT_GOAL := all

CC      ?= cc
BUILD    = build
REFDIR   = ref/src/ref

# OpenSSL: needed for AES (PRG) and SHA3/EVP (MGF/hash).
OSSL_PREFIX := $(shell brew --prefix openssl 2>/dev/null)
ifeq ($(OSSL_PREFIX),)
  OSSL_PREFIX := $(shell pkg-config --variable=prefix openssl 2>/dev/null)
endif
ifeq ($(OSSL_PREFIX),)
  OSSL_PREFIX := /usr
endif
OSSL_CFLAGS      := -I$(OSSL_PREFIX)/include
OSSL_LDFLAGS     := -L$(OSSL_PREFIX)/lib -lcrypto -Wl,-rpath,$(OSSL_PREFIX)/lib
# SHA3_absorb / SHA3_squeeze are internal symbols not exported from shared libcrypto;
# link the static library to pull them in for the SHAKE MGF implementation.
OSSL_STATIC_LIB  := $(OSSL_PREFIX)/lib/libcrypto.a

BASE_CFLAGS := -O3 -fPIC -DNDEBUG -Wall -Wno-unused-parameter \
               -Wno-unused-result -Wno-deprecated-declarations \
               -Wno-unused-function -Wno-unused-variable \
               -Wno-stringop-overflow \
               $(OSSL_CFLAGS)

PARAMS := \
    qruov1q127L3v156m54  qruov1q31L3v165m60   qruov1q31L10v600m70   qruov1q7L10v740m100  \
    qruov3q127L3v228m78  qruov3q31L3v246m87   qruov3q31L10v890m100  qruov3q7L10v1100m140 \
    qruov5q127L3v306m105 qruov5q31L3v324m114  qruov5q31L10v1120m120 qruov5q7L10v1490m190

SOS        := $(addprefix $(BUILD)/, $(addsuffix .so, $(PARAMS)))
SHIM_FILES := $(addsuffix _shim.c, $(PARAMS))

# Per-variant parameter -D flags (platform, PRG variant added in template).
pf_qruov1q127L3v156m54   := -DQRUOV_security_strength_category=1 -DQRUOV_q=127 -DQRUOV_L=3  -DQRUOV_v=156  -DQRUOV_m=54  -DQRUOV_fc=1 -DQRUOV_fe=1 -DQRUOV_fc0=1
pf_qruov1q31L3v165m60    := -DQRUOV_security_strength_category=1 -DQRUOV_q=31  -DQRUOV_L=3  -DQRUOV_v=165  -DQRUOV_m=60  -DQRUOV_fc=1 -DQRUOV_fe=1 -DQRUOV_fc0=1
pf_qruov1q31L10v600m70   := -DQRUOV_security_strength_category=1 -DQRUOV_q=31  -DQRUOV_L=10 -DQRUOV_v=600  -DQRUOV_m=70  -DQRUOV_fc=5 -DQRUOV_fe=3 -DQRUOV_fc0=1
pf_qruov1q7L10v740m100   := -DQRUOV_security_strength_category=1 -DQRUOV_q=7   -DQRUOV_L=10 -DQRUOV_v=740  -DQRUOV_m=100 -DQRUOV_fc=2 -DQRUOV_fe=1 -DQRUOV_fc0=1
pf_qruov3q127L3v228m78   := -DQRUOV_security_strength_category=3 -DQRUOV_q=127 -DQRUOV_L=3  -DQRUOV_v=228  -DQRUOV_m=78  -DQRUOV_fc=1 -DQRUOV_fe=1 -DQRUOV_fc0=1
pf_qruov3q31L3v246m87    := -DQRUOV_security_strength_category=3 -DQRUOV_q=31  -DQRUOV_L=3  -DQRUOV_v=246  -DQRUOV_m=87  -DQRUOV_fc=1 -DQRUOV_fe=1 -DQRUOV_fc0=1
pf_qruov3q31L10v890m100  := -DQRUOV_security_strength_category=3 -DQRUOV_q=31  -DQRUOV_L=10 -DQRUOV_v=890  -DQRUOV_m=100 -DQRUOV_fc=5 -DQRUOV_fe=3 -DQRUOV_fc0=1
pf_qruov3q7L10v1100m140  := -DQRUOV_security_strength_category=3 -DQRUOV_q=7   -DQRUOV_L=10 -DQRUOV_v=1100 -DQRUOV_m=140 -DQRUOV_fc=2 -DQRUOV_fe=1 -DQRUOV_fc0=1
pf_qruov5q127L3v306m105  := -DQRUOV_security_strength_category=5 -DQRUOV_q=127 -DQRUOV_L=3  -DQRUOV_v=306  -DQRUOV_m=105 -DQRUOV_fc=1 -DQRUOV_fe=1 -DQRUOV_fc0=1
pf_qruov5q31L3v324m114   := -DQRUOV_security_strength_category=5 -DQRUOV_q=31  -DQRUOV_L=3  -DQRUOV_v=324  -DQRUOV_m=114 -DQRUOV_fc=1 -DQRUOV_fe=1 -DQRUOV_fc0=1
pf_qruov5q31L10v1120m120 := -DQRUOV_security_strength_category=5 -DQRUOV_q=31  -DQRUOV_L=10 -DQRUOV_v=1120 -DQRUOV_m=120 -DQRUOV_fc=5 -DQRUOV_fe=3 -DQRUOV_fc0=1
pf_qruov5q7L10v1490m190  := -DQRUOV_security_strength_category=5 -DQRUOV_q=7   -DQRUOV_L=10 -DQRUOV_v=1490 -DQRUOV_m=190 -DQRUOV_fc=2 -DQRUOV_fe=1 -DQRUOV_fc0=1

# mgf.c needs a compatibility patch for OpenSSL 3.3+ (EVP_DigestSqueeze API).
# Build a patched copy in build/ so ref/ stays untouched.
PATCHED_MGF := $(BUILD)/patched_mgf.c

$(PATCHED_MGF): $(REFDIR)/mgf.c mgf_openssl36.patch | $(BUILD)
	cp $(REFDIR)/mgf.c $@
	patch $@ mgf_openssl36.patch

# Sources compiled into each per-param static library.
# rng.c excluded — shim provides randombytes via getrandom().
# PQCgenKAT_sign.c, qruov_config_h_gen.c, api_h_gen.c excluded (have main()).
# mgf.c replaced by patched copy above.
SRCS := $(REFDIR)/Fql.c $(REFDIR)/qruov.c $(REFDIR)/sign.c \
        $(REFDIR)/matrix.c

.PHONY: all clean check_ref

all: check_ref $(SHIM_FILES) $(SOS)

check_ref:
	@if [ ! -f $(REFDIR)/sign.c ]; then \
	    echo "ERROR: bench/schemes/qruov/ref/ not populated." >&2; \
	    echo "Run: git submodule update --init bench/schemes/qruov/ref" >&2; \
	    exit 1; \
	fi

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(BUILD):
	mkdir -p $@

# ---------------------------------------------------------------------------
# Rule template — instantiated once per param set via $(eval $(call ...))
# $(1) = param name (e.g. qruov1q127L3v156m54)
#
# Each variant needs its own qruov_config.h and api.h (generated from the
# upstream generator programs, compiled with per-variant -D flags).
# ---------------------------------------------------------------------------

define QRUOV_RULES

# Param defines come from generated qruov_config.h (via -I); don't repeat on
# command line to avoid redefined-macro warnings.
cflags_$(1)    := $(BASE_CFLAGS) -I$(BUILD)/$(1) -I$(REFDIR)
objs_ref_$(1)  := $(patsubst $(REFDIR)/%.c,$(BUILD)/$(1)/%.o,$(SRCS))
objs_$(1)      := $$(objs_ref_$(1)) $(BUILD)/$(1)/mgf.o
lib_$(1)       := $(BUILD)/$(1)/libqruov.a

$(BUILD)/$(1):
	mkdir -p $$@

# Generate qruov_config.h by compiling and running the upstream generator.
$(BUILD)/$(1)/qruov_config.h: $(REFDIR)/qruov_config_h_gen.c | $(BUILD)/$(1)
	$$(CC) $(pf_$(1)) -DQRUOV_PLATFORM=ref -DQRUOV_PRG_SHAKE \
	    -DQRUOV_CONFIG_H_GEN -I$(REFDIR) \
	    -o $(BUILD)/$(1)/cfg_gen $$<
	$(BUILD)/$(1)/cfg_gen > $$@
	rm $(BUILD)/$(1)/cfg_gen

# Generate api.h (includes CRYPTO_* sizes); needs qruov_config.h first.
$(BUILD)/$(1)/api.h: $(REFDIR)/api_h_gen.c $(BUILD)/$(1)/qruov_config.h
	$$(CC) -I$(BUILD)/$(1) -I$(REFDIR) \
	    -o $(BUILD)/$(1)/api_gen $(REFDIR)/api_h_gen.c
	$(BUILD)/$(1)/api_gen > $$@
	rm $(BUILD)/$(1)/api_gen

$$(objs_ref_$(1)): $(BUILD)/$(1)/%.o: $(REFDIR)/%.c \
    | $(BUILD)/$(1)/qruov_config.h $(BUILD)/$(1)/api.h
	mkdir -p $$(dir $$@)
	$$(CC) $$(cflags_$(1)) -c $$< -o $$@

$(BUILD)/$(1)/mgf.o: $(PATCHED_MGF) \
    | $(BUILD)/$(1)/qruov_config.h $(BUILD)/$(1)/api.h
	$$(CC) $$(cflags_$(1)) -c $$< -o $$@

$$(lib_$(1)): $$(objs_$(1))
	ar rcs $$@ $$^

$(BUILD)/$(1).so: $(1)_shim.c $$(lib_$(1)) | $(BUILD)
	$$(CC) $$(cflags_$(1)) -shared $$< $$(lib_$(1)) \
	    $(OSSL_STATIC_LIB) $(OSSL_LDFLAGS) -o $$@

endef

$(foreach P,$(PARAMS),$(eval $(call QRUOV_RULES,$(P))))

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
