include ../../../bench-common/flags.mk
CC     ?= cc
CMAKE  ?= cmake

.DEFAULT_GOAL := all

# SQIsign round-2 submission (NIST on-ramp round 2, YAML version v2.0):
# SQISign/the-sqisign tag nist-v2. Its src/ is identical to both
# Reference_Implementation/ and Additional_Implementations/broadwell/ of NIST's
# sqisign-submission-round2.zip (only README.md and the default
# SQISIGN_BUILD_TYPE in CMakeLists.txt differ).
REF    := ref
BUILD  := build
# SQIsign build type: broadwell = reference code + x86-64 assembly field
# arithmetic (Broadwell and later, the fastest x86 option upstream offers and the
# submission's Additional_Implementations/broadwell); ref = portable C. Only the
# gf module differs between the two.
SQISIGN_BUILD_TYPE ?= broadwell
GF_IMPL := $(if $(filter ref opt,$(SQISIGN_BUILD_TYPE)),ref,$(SQISIGN_BUILD_TYPE))
CBUILD := $(BUILD)/cmake-$(SQISIGN_BUILD_TYPE)
CSRC   := $(CBUILD)/src

PARAM_SETS :=

# $(1) = file stem (e.g. sqisign_r2_lvl1)
# $(2) = variant   (lvl1/lvl3/lvl5); the mp and quaternion modules are one shared
#        "generic" lib in round 2. Signing needs GMP (system libgmp, upstream's default
#        GMP_LIBRARY=SYSTEM).
define SQISIGN_RULES
PARAM_SETS += $(1)
$(BUILD)/$(1).so: $(1)_shim.c $(CSRC)/libsqisign_$(2)_nistapi.a | $(BUILD)
	$$(CC) $(BENCH_CFLAGS) -fPIC -std=c99 -shared -DSQISIGN_NS_TYPE=$(SQISIGN_BUILD_TYPE) \
	    $(1)_shim.c \
	    $(CSRC)/libsqisign_$(2)_nistapi.a \
	    $(CSRC)/libsqisign_$(2).a \
	    $(CSRC)/signature/ref/$(2)/libsqisign_signature_$(2).a \
	    $(CSRC)/verification/ref/$(2)/libsqisign_verification_$(2).a \
	    $(CSRC)/id2iso/ref/$(2)/libsqisign_id2iso_$(2).a \
	    $(CSRC)/hd/ref/$(2)/libsqisign_hd_$(2).a \
	    $(CSRC)/ec/ref/$(2)/libsqisign_ec_$(2).a \
	    $(CSRC)/quaternion/ref/generic/libsqisign_quaternion_generic.a \
	    $(CSRC)/gf/$(GF_IMPL)/$(2)/libsqisign_gf_$(2).a \
	    $(CSRC)/mp/ref/generic/libsqisign_mp_generic.a \
	    $(CSRC)/precomp/ref/$(2)/libsqisign_precomp_$(2).a \
	    $(CSRC)/common/generic/libsqisign_common_sys.a \
	    -lgmp -lm \
	    -o $$@
endef

$(eval $(call SQISIGN_RULES,sqisign_r2_lvl1,lvl1))
$(eval $(call SQISIGN_RULES,sqisign_r2_lvl3,lvl3))
$(eval $(call SQISIGN_RULES,sqisign_r2_lvl5,lvl5))

TARGETS    := $(addprefix $(BUILD)/, $(addsuffix .so, $(PARAM_SETS)))
SHIM_FILES := $(addsuffix _shim.c, $(PARAM_SETS))

.PHONY: all clean check-submodule

all: check-submodule $(SHIM_FILES) $(TARGETS)

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

check-submodule:
	@test -f $(REF)/CMakeLists.txt || { \
	    echo "ERROR: bench/schemes/sqisign_r2/ref/ is empty."; \
	    echo "Run: git submodule update --init bench/schemes/sqisign_r2/ref"; \
	    exit 1; }

# Build all cmake static libs in one go (cmake configure + make)
$(CSRC)/libsqisign_lvl1_nistapi.a \
$(CSRC)/libsqisign_lvl3_nistapi.a \
$(CSRC)/libsqisign_lvl5_nistapi.a: $(CBUILD)/.built
	@true

# Compiler flags: as bench/schemes/sqisign (see there). Upstream's .cmake/flags.cmake
# appends -O3 -funroll-loops -fvisibility=hidden and, in its default ENABLE_STRICT mode,
# -fno-strict-aliasing -Werror to CMAKE_C_FLAGS and enables LTO for every build type but
# Debug. Our Bench build type's flags (the global set, then -fno-unroll-loops) come after
# them; no_lto.cmake turns LTO off. Exceptions, kept: -fvisibility=hidden (needed: the
# broadwell fp_asm.S addresses the C constant p PC-relatively, which only links into our
# shared objects if the symbol is DSO-local) and -fno-strict-aliasing (upstream's default
# ENABLE_STRICT build). -Wno-error: GCC 15 at our -O3 flags a false-positive
# -Wmaybe-uninitialized in mp/ref/generic/mp.c, which upstream's -Werror turns into an error.
SQISIGN_CMAKE_CFLAGS := $(BENCH_CFLAGS) -fPIC -fno-unroll-loops -Wno-error

$(CBUILD)/.built: $(REF)/CMakeLists.txt Makefile no_lto.cmake ../../../bench-common/flags.mk | $(CBUILD)
	cd $(CBUILD) && $(CMAKE) ../../$(REF) \
	    -DCMAKE_BUILD_TYPE=Bench \
	    -DCMAKE_C_FLAGS_BENCH='$(SQISIGN_CMAKE_CFLAGS)' \
	    -DCMAKE_PROJECT_INCLUDE=$(CURDIR)/no_lto.cmake \
	    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
	    -DSQISIGN_BUILD_TYPE=$(SQISIGN_BUILD_TYPE) \
	    -DENABLE_TESTS=OFF \
	    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
	$(MAKE) -C $(CBUILD)
	touch $@

$(CBUILD):
	mkdir -p $@

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
