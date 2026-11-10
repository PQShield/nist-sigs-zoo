include ../../../bench-common/flags.mk
.DEFAULT_GOAL := all

# SDitH v2.0 (NIST round 2). sdith/sdith has no v2 tag, so this builds the official round-2
# submission package from NIST: .source holds its URL (line 1) and sha256 (line 2); it is
# fetched into build/dl and unpacked into build/src. Optimized_Implementation/ has one
# directory per parameter set, but their src/ and lib/ trees are identical (only
# CMakeLists.txt and generator/api.h differ; the parameter set is a runtime struct), so
# one library is built from sdith_cat1_short and the shims pick CAT*_PARAMETERS.
# The AVX2 code is selected at run time (vole_parameters_init, __builtin_cpu_supports).
CC  ?= cc
CXX ?= c++
BUILD = build

URL      := $(shell sed -n 1p .source)
ZIP      := $(BUILD)/dl/sdith-submission-round2.zip
IMPL     := $(BUILD)/src/SDitH-v2/Optimized_Implementation/sdith_cat1_short
UNZIPPED := $(BUILD)/src/.unzipped

# Upstream CMake (Release): -O3 -g3 -Wall -Werror -DNDEBUG, with -mpclmul -msse2 -mavx2
# (-maes for lib/aes) on the AVX2 files; we keep the ISA flags for all files (as sdith2)
# and use the global flag set. vole_parameters{,_avx2}.cpp are C code compiled as C++ by
# upstream (extern "C" via commons.h); CMake's default for g++ is -std=gnu++17.
ISA      := -mavx2 -mpclmul -msse2 -maes
CFLAGS   := -Wno-unused-parameter $(ISA) $(BENCH_CFLAGS) -fPIC
CXXFLAGS := -std=gnu++17 -Wno-unused-parameter $(ISA) $(BENCH_CXXFLAGS) -fPIC

SHA3_DIR := $(IMPL)/lib/sha3
AES_DIR  := $(IMPL)/lib/aes
SRC_DIR  := $(IMPL)/src
IFLAGS   := -I$(SRC_DIR) -I$(SHA3_DIR) -I$(SHA3_DIR)/avx2 -I$(AES_DIR)

SHA3_CSRCS := KeccakHash.c KeccakSponge.c KeccakSpongetimes4.c KeccakHashtimes4.c \
              avx2/KeccakP-1600-times4-SIMD256.c
SHA3_SSRCS := avx2/KeccakP-1600-AVX2.s
AES_CSRCS  := rijndael256.c aes_ansi_ref.c aes128_ctrle_ref.c aes128_ctrle_special_ref.c \
              rijndael256_ctrle_ref.c rijndael256_ctrle_special_ref.c \
              rijndael256_avx.c aes128_ctrle_avx2.c rijndael256_ctrle_avx2.c
AES_SSRCS  := aes128_ctrle_special_avx2_unix.s
VOLE_CSRCS := commons.c sdith_prng.c gf256.c gf192.c matrix_vector_products_f2.c \
              matrix_transpose_f2.c ggm.c vole_expansion.c piop_circuit.c gf128.c rsd.c \
              sdith_signature.c sdith_signature_parameters.c vole_to_piop.c \
              gf128_avx2.c gf192_avx2.c gf256_avx2.c matrix_vector_products_f2_avx2.c \
              sdith_prng_avx2.c
VOLE_CXXSRCS := vole_parameters.cpp vole_parameters_avx2.cpp

OBJS := $(patsubst %.c,$(BUILD)/obj/sha3/%.o,$(SHA3_CSRCS)) \
        $(patsubst %.s,$(BUILD)/obj/sha3/%.o,$(SHA3_SSRCS)) \
        $(patsubst %.c,$(BUILD)/obj/aes/%.o,$(AES_CSRCS)) \
        $(patsubst %.s,$(BUILD)/obj/aes/%.o,$(AES_SSRCS)) \
        $(patsubst %.c,$(BUILD)/obj/src/%.o,$(VOLE_CSRCS)) \
        $(patsubst %.cpp,$(BUILD)/obj/src/%.o,$(VOLE_CXXSRCS))
LIB := $(BUILD)/libsdith_r2.a

PARAMS     := sdith_r2_cat1_short sdith_r2_cat1_fast \
              sdith_r2_cat3_short sdith_r2_cat3_fast \
              sdith_r2_cat5_short sdith_r2_cat5_fast
SHIM_FILES := $(addsuffix _shim.c, $(PARAMS))
SOS        := $(addprefix $(BUILD)/, $(addsuffix .so, $(PARAMS)))

.PHONY: all clean

all: $(SHIM_FILES) $(SOS)

$(SHIM_FILES) &: shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(ZIP): .source
	mkdir -p $(@D)
	curl -fsSL --retry 3 -o $@.tmp '$(URL)'
	cd $(@D) && sed -n 2p ../../.source | sed 's/\.zip$$/.zip.tmp/' | sha256sum -c - \
	    || { echo "SHA-256 mismatch for $@" >&2; rm -f $(@F).tmp; exit 1; }
	mv $@.tmp $@

$(UNZIPPED): $(ZIP)
	rm -rf $(BUILD)/src
	mkdir -p $(BUILD)/src
	python3 -m zipfile -e $(ZIP) $(BUILD)/src
	touch $@

# All sources come out of the zip: order them after the unpack.
$(BUILD)/obj/sha3/%.o: $(UNZIPPED)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(IFLAGS) -c $(SHA3_DIR)/$*.c -o $@
$(BUILD)/obj/aes/%.o: $(UNZIPPED)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(IFLAGS) -c $(AES_DIR)/$*.c -o $@
$(BUILD)/obj/src/%.o: $(UNZIPPED)
	@mkdir -p $(@D)
	$(if $(filter $*.cpp,$(VOLE_CXXSRCS)),$(CXX) $(CXXFLAGS),$(CC) $(CFLAGS)) $(IFLAGS) \
	    -c $(SRC_DIR)/$(if $(filter $*.cpp,$(VOLE_CXXSRCS)),$*.cpp,$*.c) -o $@
# Assembly (not preprocessed; upstream's CMake passes no extra flags).
$(BUILD)/obj/sha3/avx2/KeccakP-1600-AVX2.o: $(UNZIPPED)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $(SHA3_DIR)/avx2/KeccakP-1600-AVX2.s -o $@
$(BUILD)/obj/aes/aes128_ctrle_special_avx2_unix.o: $(UNZIPPED)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $(AES_DIR)/aes128_ctrle_special_avx2_unix.s -o $@

$(LIB): $(OBJS)
	ar rcs $@ $^

$(BUILD)/%.so: %_shim.c $(LIB)
	$(CC) $(CFLAGS) $(IFLAGS) -I../.. -shared $< $(LIB) -lm -o $@

clean:
	rm -rf $(BUILD)/src $(BUILD)/obj $(BUILD)/*.so $(BUILD)/*.a
	rm -f $(SHIM_FILES)
