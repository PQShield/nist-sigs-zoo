CXX   ?= g++
CC    ?= cc
BUILD  = build
GEN    = $(BUILD)/gen
REF    = ref

MESON_BUILD := $(BUILD)/meson

CXXFLAGS := -O3 -std=c++23 -fPIC -maes -mavx2 -mpopcnt -march=native -mtune=native \
            -Wall -Wextra -Wno-unused-parameter
CFLAGS   := -O3 -fPIC -march=native -mtune=native -Wall

# XKCP source is downloaded by meson into ref/subprojects/xkcp/
# config.h lives at the XKCP root; libxkcp.a is in the meson build dir.
XKCP_SRC := $(REF)/subprojects/xkcp
XKCP_INCS := -I$(XKCP_SRC) \
             -I$(XKCP_SRC)/lib/high/Keccak \
             -I$(XKCP_SRC)/lib/high/Keccak/FIPS202 \
             -I$(XKCP_SRC)/lib/common \
             -I$(XKCP_SRC)/lib/low/common \
             -I$(XKCP_SRC)/lib/low/KeccakP-1600/common \
             -I$(XKCP_SRC)/lib/low/KeccakP-1600/AVX2 \
             -I$(XKCP_SRC)/lib/low/KeccakP-1600/AVX2/SnP \
             -I$(XKCP_SRC)/lib/low/KeccakP-1600-times4/AVX2 \
             -I$(XKCP_SRC)/lib/low/KeccakP-1600-times4/AVX2/SnP

IFLAGS := -I$(REF) -I$(GEN) $(XKCP_INCS)

LIBFAEST   := $(MESON_BUILD)/libfaest.a
LIBXKCP    := $(MESON_BUILD)/subprojects/xkcp/libxkcp.a
RANDOMNESS := $(BUILD)/randomness_os.o

PARAM_NAMES := faest_128s faest_128f faest_em_128s faest_em_128f \
               faest_192s faest_192f faest_em_192s faest_em_192f \
               faest_256s faest_256f faest_em_256s faest_em_256f

GEN_API_SRCS := $(addprefix $(GEN)/, $(addsuffix _api.cpp, $(PARAM_NAMES)))
GEN_API_HDRS := $(addprefix $(GEN)/, $(addsuffix _api.h,   $(PARAM_NAMES)))
SHIM_FILES   := $(addsuffix _shim.c, $(PARAM_NAMES))
SOS          := $(addprefix $(BUILD)/, $(addsuffix .so, $(PARAM_NAMES)))

.DEFAULT_GOAL := all
.PHONY: all clean check-submodule

all: check-submodule $(SHIM_FILES) $(SOS)

check-submodule:
	@test -f $(REF)/meson.build || { \
	    echo "ERROR: bench/schemes/faest/ref/ is empty."; \
	    echo "Run: git submodule update --init bench/schemes/faest/ref"; \
	    exit 1; }

# Shim generation (one *_shim.c per row in params.tsv)
$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

# Generate per-variant api.cpp + api.h from arch-opt templates (both at once)
$(GEN_API_SRCS) $(GEN_API_HDRS): gen_faest.py $(REF)/api.cpp.in $(REF)/api.h.in | $(GEN)
	python3 gen_faest.py $(REF) $(GEN)

# Build libfaest.a and libxkcp.a via Meson
$(LIBFAEST) $(LIBXKCP): $(MESON_BUILD)/.built
	@true

$(MESON_BUILD)/.built: $(REF)/meson.build | $(MESON_BUILD)
	meson setup $(CURDIR)/$(MESON_BUILD) $(CURDIR)/$(REF) \
	    --buildtype=release \
	    -Doptimization=3 \
	    -Db_lto=false \
	    --wipe
	meson compile -C $(CURDIR)/$(MESON_BUILD) faest xkcp
	touch $@

# rand_bytes is in randomness_os.c (not in libfaest.a); compile once, link into each .so
$(RANDOMNESS): $(REF)/randomness_os.c | $(BUILD)
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

# Compile the C shim separately (void* → typed ptr is valid C but not C++)
$(BUILD)/%_shim.o: %_shim.c $(GEN)/%_api.h | $(BUILD)
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

# Compile the C++ api file and link with the shim into a .so
# Link with CXX to pull in the C++ runtime
$(BUILD)/%.so: $(BUILD)/%_shim.o $(GEN)/%_api.cpp $(LIBFAEST) $(LIBXKCP) $(RANDOMNESS) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(IFLAGS) -shared \
	    $(BUILD)/$*_shim.o \
	    $(GEN)/$*_api.cpp \
	    $(RANDOMNESS) \
	    $(LIBFAEST) $(LIBXKCP) \
	    -o $@

$(BUILD) $(GEN) $(MESON_BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
