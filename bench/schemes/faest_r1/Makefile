include ../../../bench-common/flags.mk
CC     ?= cc
BUILD  := build
# FAEST v1.0 (NIST round 1) is benchmarked from the NIST round-1 submission package:
# .source holds its URL (line 1) and sha256 (line 2); it is fetched into build/dl and
# unpacked into build/src. Implementation: Additional_Implementations/avx2 (the
# submitters' AVX2 + AES-NI code, the predecessor of faest-arch-opt), one directory per
# parameter set that differs only in config.h/api.h.
# (faest-arch-opt's `release` branch, commit 4676dfe "FAEST v1.0", has the same code, but
# a different layout with an XKCP submodule and a generated Makefile.)
URL      := $(shell sed -n 1p .source)
ZIP      := $(BUILD)/dl/FAEST-submission.zip
AVX2     := $(BUILD)/src/nist_submission/Additional_Implementations/avx2
UNZIPPED := $(BUILD)/src/.unzipped

# Upstream's Makefile: -g -O2 -march=native -mtune=native -std=c11 -DHAVE_OPENSSL -DNDEBUG
# (HAVE_OPENSSL is unused by the library). We keep -std=c11 and use the global flag set.
CFLAGS := -std=c11 -Wall $(BENCH_CFLAGS) -fPIC

# Upstream's library sources: all *.c and *.s except the randomness backends and the NIST
# DRBG (rng.c); also not the two programs (api_test.c, PQCgenKAT_sign.c). The bench links
# randomness_os.c (getrandom).
LIB_SRCS := aes.c aes_impl.c api.c block.c block_impl.c faest.c hash.c KeccakHash.c \
            KeccakHashtimes4.c KeccakP-1600-AVX2.s KeccakP-1600-times2-SIMD128.c \
            KeccakP-1600-times4-SIMD256.c KeccakP-1600-times8-on4.c KeccakSponge.c \
            KeccakSpongetimes4.c owf_proof.c polynomials.c polynomials_impl.c prgs.c \
            quicksilver.c small_vole.c transpose.c universal_hash.c util.c vector_com.c \
            vole_check.c vole_commit.c randomness_os.c

PARAM_NAMES := faest_128s faest_128f faest_em_128s faest_em_128f \
               faest_192s faest_192f faest_em_192s faest_em_192f \
               faest_256s faest_256f faest_em_256s faest_em_256f

SHIM_FILES := $(addsuffix _shim.c, $(PARAM_NAMES))
SOS        := $(addprefix $(BUILD)/, $(addsuffix .so, $(PARAM_NAMES)))

.DEFAULT_GOAL := all
.PHONY: all clean

all: $(SHIM_FILES) $(SOS)

$(SHIM_FILES) &: shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(ZIP): .source
	mkdir -p $(@D)
	curl -fsSL --retry 3 -o $@.tmp '$(URL)'
	cd $(@D) && sed -n 2p ../../.source | sed 's/FAEST-submission.zip$$/FAEST-submission.zip.tmp/' | sha256sum -c - \
	    || { echo "SHA-256 mismatch for $@" >&2; rm -f $(@F).tmp; exit 1; }
	mv $@.tmp $@

$(UNZIPPED): $(ZIP)
	rm -rf $(BUILD)/src
	mkdir -p $(BUILD)/src
	python3 -m zipfile -e $(ZIP) $(BUILD)/src
	touch $@

# Each variant is compiled from its own directory (its config.h selects the parameters).
# Upstream's crypto_sign_keypair is renamed so the shim can wrap it (see shim_template.c:
# key self-test for FAEST-256s/256f, which work around a key-validity bug in this code).
$(BUILD)/%.so: %_shim.c $(UNZIPPED)
	$(CC) $(CFLAGS) -Dcrypto_sign_keypair=faest_upstream_keypair -I$(AVX2)/$* -I../.. -shared -o $@ \
	    $*_shim.c $(addprefix $(AVX2)/$*/, $(LIB_SRCS))

clean:
	rm -rf $(BUILD)/src $(BUILD)/*.so
	rm -f $(SHIM_FILES)
