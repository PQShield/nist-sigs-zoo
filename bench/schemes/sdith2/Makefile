.DEFAULT_GOAL := all

CC  ?= cc
CXX ?= c++
BUILD = build
IMPL  = impl

CFLAGS   := -O3 -fPIC -DNDEBUG -Wno-unused-parameter -mavx2 -mpclmul -msse2 -maes
CXXFLAGS := $(CFLAGS) -std=c++11

SHA3_DIR  := $(IMPL)/lib/sha3
AES_DIR   := $(IMPL)/lib/aes
SRC_DIR   := $(IMPL)/src

IFLAGS := -I$(SRC_DIR) -I$(SHA3_DIR) -I$(SHA3_DIR)/avx2 -I$(AES_DIR)

# ---------------------------------------------------------------------------
# SHA3
# ---------------------------------------------------------------------------
SHA3_CSRCS := $(SHA3_DIR)/KeccakHash.c $(SHA3_DIR)/KeccakSponge.c \
              $(SHA3_DIR)/KeccakSpongetimes4.c $(SHA3_DIR)/KeccakHashtimes4.c
SHA3_AVX2C := $(SHA3_DIR)/avx2/KeccakP-1600-times4-SIMD256.c
SHA3_SSRCS := $(SHA3_DIR)/avx2/KeccakP-1600-AVX2.s

SHA3_COBJS := $(patsubst $(SHA3_DIR)/%.c,      $(BUILD)/sha3_%.o, $(SHA3_CSRCS))
SHA3_AVX2OBJ := $(BUILD)/sha3_keccak_times4.o
SHA3_SOBJ  := $(BUILD)/sha3_keccak_avx2.o

# ---------------------------------------------------------------------------
# AES
# ---------------------------------------------------------------------------
AES_CSRCS := $(wildcard $(AES_DIR)/*.c)
AES_SSRCS := $(AES_DIR)/aes128_ctrle_special_avx2_unix.s

AES_COBJS := $(patsubst $(AES_DIR)/%.c, $(BUILD)/aes_%.o, $(AES_CSRCS))
AES_SOBJ  := $(BUILD)/aes_avx2.o

# ---------------------------------------------------------------------------
# vole / main source
# ---------------------------------------------------------------------------
VOLE_CSRCS   := $(filter-out %.cpp, $(wildcard $(SRC_DIR)/*.c))
VOLE_CXXSRCS := $(wildcard $(SRC_DIR)/*.cpp)

VOLE_COBJS   := $(patsubst $(SRC_DIR)/%.c,   $(BUILD)/vole_%.o, $(VOLE_CSRCS))
VOLE_CXXOBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD)/vole_%.o, $(VOLE_CXXSRCS))

# ---------------------------------------------------------------------------
# Static library
# ---------------------------------------------------------------------------
LIB := $(BUILD)/libsdith2.a
ALL_OBJS := $(SHA3_COBJS) $(SHA3_AVX2OBJ) $(SHA3_SOBJ) $(AES_COBJS) $(AES_SOBJ) \
            $(VOLE_COBJS) $(VOLE_CXXOBJS)

# ---------------------------------------------------------------------------
# Shim targets
# ---------------------------------------------------------------------------
PARAMS     := sdith2_cat1_short sdith2_cat1_fast \
              sdith2_cat3_short sdith2_cat3_fast \
              sdith2_cat5_short sdith2_cat5_fast
SHIM_FILES := $(addsuffix _shim.c, $(PARAMS))
SOS        := $(addprefix $(BUILD)/, $(addsuffix .so, $(PARAMS)))

.PHONY: all clean

all: $(SHIM_FILES) $(SOS)

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(BUILD):
	mkdir -p $@

# ---------------------------------------------------------------------------
# Compile rules
# ---------------------------------------------------------------------------
$(BUILD)/sha3_%.o: $(SHA3_DIR)/%.c | $(BUILD)
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(SHA3_AVX2OBJ): $(SHA3_AVX2C) | $(BUILD)
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(SHA3_SOBJ): $(SHA3_SSRCS) | $(BUILD)
	$(CC) -c -Dno_type -Dno_size -Dno_plt $< -o $@

$(BUILD)/aes_%.o: $(AES_DIR)/%.c | $(BUILD)
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(AES_SOBJ): $(AES_SSRCS) | $(BUILD)
	$(CC) -c $< -o $@

$(BUILD)/vole_%.o: $(SRC_DIR)/%.c | $(BUILD)
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(BUILD)/vole_%.o: $(SRC_DIR)/%.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

# ---------------------------------------------------------------------------
# Static library + shared libs
# ---------------------------------------------------------------------------
$(LIB): $(ALL_OBJS) | $(BUILD)
	ar rcs $@ $^

$(BUILD)/%.so: %_shim.c $(LIB) | $(BUILD)
	$(CC) $(CFLAGS) $(IFLAGS) -shared $< $(LIB) -lm -o $@

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
