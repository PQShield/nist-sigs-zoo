include ../../../bench-common/flags.mk
CC      ?= cc
# Upstream's CMake Release flags for OPT_IMPL=avx (CMakeLists.txt, src/CMakeLists.txt:
# -O3 -flto=auto -ftree-vectorize -funroll-loops, ISA flags, -D_SHAKE_TIMES4_; Rijndael
# commitments, i.e. no -D_SHA3_). We keep the ISA, language (CMAKE_C_STANDARD 11 =
# -std=gnu11) and define flags and use the global flag set (flags.mk); its
# -fno-semantic-interposition matters here: RYDE-1-Short signing was 4x slower without it.
CFLAGS  := -Wall -Wextra -Wno-unused-function \
           -mpclmul -msse2 -mavx -mavx2 -maes -std=gnu11 \
           -D_SHAKE_TIMES4_ $(BENCH_CFLAGS) -fPIC
BUILD   := build
# RYDE v2.1.0 is released only as a zip on pqc-ryde.org: .source holds its URL
# (line 1) and sha256 (line 2); it is fetched into build/dl and unpacked into build/src.
URL      := $(shell sed -n 1p .source)
ZIP      := $(BUILD)/dl/ryde_v2.1.0.zip
OPT      := $(BUILD)/src/ryde_v2.1.0/Optimized_Implementation
UNZIPPED := $(BUILD)/src/.unzipped

.DEFAULT_GOAL := all
.PHONY: all clean

PARAM_SETS :=

# Not compiled: nist/ (combined NIST API, KAT generator), src/library/rng/rng.c
# (NIST DRBG, needs OpenSSL), bench/, example/.
SRCS = ggm_tree.c field_vector.c field_matrix.c binary_matrix.c parsing.c keygen.c \
       tcith.c tcith_sign.c tcith_verify.c \
       library/xkcp/KeccakHash.c library/xkcp/KeccakHashtimes4.c library/xkcp/KeccakSponge.c \
       library/xkcp/KeccakSpongetimes4.c library/xkcp/SimpleFIPS202.c \
       library/xkcp/avx2/KeccakP-1600-AVX2.s library/xkcp/avx2/KeccakP-1600-times4-SIMD256.c
INCS = . optimized/avx library library/rng rijndael library/xkcp library/xkcp/avx2 field

# $(1) = file stem = upstream directory, $(2) = hash bits, $(3) = field degree
define RYDE_RULES
PARAM_SETS += $(1)
$(BUILD)/$(1).so: $(1)_shim.c $(UNZIPPED)
	$(CC) $(CFLAGS) $(addprefix -I$(OPT)/$(1)/src/,$(INCS)) -I../.. -shared -o $$@ \
	    $(1)_shim.c $(addprefix $(OPT)/$(1)/src/,$(SRCS) \
	        library/hash_sha3_$(2).c library/hash_sha3_x4_$(2).c field/field_$(3).c)
endef

$(eval $(call RYDE_RULES,ryde1s,256,53))
$(eval $(call RYDE_RULES,ryde1f,256,53))
$(eval $(call RYDE_RULES,ryde1ff,256,53))
$(eval $(call RYDE_RULES,ryde3s,384,61))
$(eval $(call RYDE_RULES,ryde3f,384,61))
$(eval $(call RYDE_RULES,ryde3ff,384,61))
$(eval $(call RYDE_RULES,ryde5s,512,61))
$(eval $(call RYDE_RULES,ryde5f,512,61))
$(eval $(call RYDE_RULES,ryde5ff,512,61))

SOS        := $(addprefix $(BUILD)/,$(addsuffix .so,$(PARAM_SETS)))
SHIM_FILES := $(addsuffix _shim.c,$(PARAM_SETS))

all: $(SHIM_FILES) $(SOS)

$(SHIM_FILES) &: shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(ZIP): .source
	mkdir -p $(@D)
	curl -fsSL --retry 3 -o $@.tmp '$(URL)'
	cd $(@D) && sed -n 2p ../../.source | sed 's/ryde_v2.1.0.zip$$/ryde_v2.1.0.zip.tmp/' | sha256sum -c - \
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
