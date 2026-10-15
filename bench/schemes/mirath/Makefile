include ../../../bench-common/flags.mk
CC      ?= cc
# Upstream's CMake Release flags (Optimized_Implementation/*/CMakeLists.txt) are
# -O3 -DNDEBUG -ftree-vectorize -funroll-loops -fomit-frame-pointer -fno-stack-protector
# with ISA flags; we keep the ISA and language flags (CMAKE_C_STANDARD 11 = -std=gnu11)
# and use the global flag set (flags.mk).
CFLAGS  := -Wall -Wextra -mavx -mavx2 -mbmi -mbmi2 -maes -std=gnu11 $(BENCH_CFLAGS) -fPIC
BUILD   := build
# Mirath v2.1.0 is released only as a zip on pqc-mirath.org: .source holds its URL
# (line 1) and sha256 (line 2); it is fetched into build/dl and unpacked into build/src.
URL      := $(shell sed -n 1p .source)
ZIP      := $(BUILD)/dl/mirath_v2.1.0.zip
OPT      := $(BUILD)/src/mirath_v2.1.0/Optimized_Implementation/mirath_tcith
UNZIPPED := $(BUILD)/src/.unzipped

.DEFAULT_GOAL := all
.PHONY: all clean

PARAM_SETS :=

# Not compiled: sign.c (combined NIST API; the shim calls mirath_sign/verify),
# rng.c (NIST DRBG, needs OpenSSL), main.c, PQCgenKAT_sign.c.
SRCS = mirath_matrix_ff.c mirath_ggm_tree.c mirath_keygen.c \
       tcith/mirath_tcith.c tcith/mirath_sign.c tcith/mirath_verify.c tcith/mirath_parsing.c \
       common/xkcp/KeccakHash.c common/xkcp/KeccakP-1600-AVX2.s \
       common/xkcp/avx2/KeccakHashtimes4.c common/xkcp/avx2/KeccakP-1600-times4-SIMD256.c \
       common/xkcp/avx2/KeccakSpongetimes4.c
INCS = . common common/xkcp common/xkcp/avx2 optimized/avx2 optimized/avx2/arith tcith

# $(1) = file stem, $(2) = upstream directory, $(3) = lambda (hash_sha3_<lambda>.c)
define MIRATH_RULES
PARAM_SETS += $(1)
$(BUILD)/$(1).so: $(1)_shim.c $(UNZIPPED)
	$(CC) $(CFLAGS) $(addprefix -I$(OPT)/$(2)/,$(INCS)) -I../.. -shared -o $$@ \
	    $(1)_shim.c $(addprefix $(OPT)/$(2)/,$(SRCS) common/hash_sha3_$(3).c)
endef

$(eval $(call MIRATH_RULES,mirath_1a_short,mirath_tcith_1a_short,128))
$(eval $(call MIRATH_RULES,mirath_1a_fast,mirath_tcith_1a_fast,128))
$(eval $(call MIRATH_RULES,mirath_1b_short,mirath_tcith_1b_short,128))
$(eval $(call MIRATH_RULES,mirath_1b_fast,mirath_tcith_1b_fast,128))
$(eval $(call MIRATH_RULES,mirath_3a_short,mirath_tcith_3a_short,192))
$(eval $(call MIRATH_RULES,mirath_3a_fast,mirath_tcith_3a_fast,192))
$(eval $(call MIRATH_RULES,mirath_3b_short,mirath_tcith_3b_short,192))
$(eval $(call MIRATH_RULES,mirath_3b_fast,mirath_tcith_3b_fast,192))
$(eval $(call MIRATH_RULES,mirath_5a_short,mirath_tcith_5a_short,256))
$(eval $(call MIRATH_RULES,mirath_5a_fast,mirath_tcith_5a_fast,256))
$(eval $(call MIRATH_RULES,mirath_5b_short,mirath_tcith_5b_short,256))
$(eval $(call MIRATH_RULES,mirath_5b_fast,mirath_tcith_5b_fast,256))

SOS        := $(addprefix $(BUILD)/,$(addsuffix .so,$(PARAM_SETS)))
SHIM_FILES := $(addsuffix _shim.c,$(PARAM_SETS))

all: $(SHIM_FILES) $(SOS)

$(SHIM_FILES) &: shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(ZIP): .source
	mkdir -p $(@D)
	curl -fsSL --retry 3 -o $@.tmp '$(URL)'
	cd $(@D) && sed -n 2p ../../.source | sed 's/mirath_v2.1.0.zip$$/mirath_v2.1.0.zip.tmp/' | sha256sum -c - \
	    || { echo "SHA-256 mismatch for $@" >&2; rm -f $(@F).tmp; exit 1; }
	mv $@.tmp $@

$(UNZIPPED): $(ZIP)
	rm -rf $(BUILD)/src
	mkdir -p $(BUILD)/src
	python3 -m zipfile -e $(ZIP) $(BUILD)/src
	touch $@

clean:
	rm -rf $(BUILD)/src $(BUILD)/*.so
	rm -f $(SHIM_FILES)
