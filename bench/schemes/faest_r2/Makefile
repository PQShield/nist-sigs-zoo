include ../../../bench-common/flags.mk
CXX   ?= g++
CC    ?= cc
BUILD  = build
GEN    = $(BUILD)/gen
REF    = ref

MESON_BUILD := $(BUILD)/meson

CXXFLAGS := -std=c++23 -maes -mavx2 -mpopcnt -Wall -Wextra -Wno-unused-parameter \
            $(BENCH_CXXFLAGS) -fPIC
CFLAGS   := -Wall $(BENCH_CFLAGS) -fPIC

# FAEST v2.0.0 (NIST round 2): faest-arch-opt tag v2.0.0, whose sources are identical to
# Additional_Implementations/avx2 of the NIST round-2 package (faest-submission-round2.zip).
# XKCP (rev 64404bee, pinned in ref/subprojects/xkcp.wrap) is downloaded by meson into
# ref/subprojects/xkcp/; include dirs as in ref/subprojects/packagefiles/xkcp/meson.build.
XKCP_SRC := $(REF)/subprojects/xkcp
XKCP_INCS := $(addprefix -I$(XKCP_SRC)/, . lib/common lib/high/Keccak lib/high/Keccak/FIPS202 \
             lib/low/KeccakP-1600/AVX2 lib/low/KeccakP-1600/common \
             lib/low/KeccakP-1600-times2/SIMD128 lib/low/KeccakP-1600-times2/SIMD128/SSSE3-u2 \
             lib/low/KeccakP-1600-times4/AVX2 lib/low/KeccakP-1600-times4/AVX2/u12 \
             lib/low/KeccakP-1600-times8/fallback-on4)

IFLAGS := -I$(REF) -I$(GEN) -I$(MESON_BUILD) $(XKCP_INCS)

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
	    echo "ERROR: bench/schemes/faest_r2/ref/ is empty."; \
	    echo "Run: git submodule update --init bench/schemes/faest_r2/ref"; \
	    exit 1; }

# Shim generation (one *_shim.c per row in params.tsv)
$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

# Generate per-variant api.cpp + api.h from arch-opt templates (both at once), as
# upstream's tools/prepare_nist.py does for the NIST package
$(GEN_API_SRCS) $(GEN_API_HDRS): gen_faest.py $(REF)/api.cpp.in $(REF)/api.h.in | $(GEN)
	python3 gen_faest.py $(REF) $(GEN)

# Build libfaest.a and libxkcp.a via Meson
$(LIBFAEST) $(LIBXKCP): $(MESON_BUILD)/.built
	@true

# Meson puts c_args/cpp_args after its buildtype flags, so the global flag set (flags.mk)
# comes last; they also reach the XKCP subproject. They replace upstream's default
# c_args/cpp_args (-O3 -march=native -mtune=native -g), of which -g is kept; LTO
# (upstream default b_lto=true) is off, as for every scheme.
$(MESON_BUILD)/.built: $(REF)/meson.build Makefile ../../../bench-common/flags.mk | $(MESON_BUILD)
	meson setup $(CURDIR)/$(MESON_BUILD) $(CURDIR)/$(REF) \
	    --buildtype=release \
	    -Doptimization=3 \
	    -Db_lto=false \
	    -Db_ndebug=true \
	    -Dc_args='-g $(BENCH_CFLAGS)' \
	    -Dcpp_args='-g $(BENCH_CXXFLAGS)' \
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
