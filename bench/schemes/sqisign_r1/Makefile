include ../../../bench-common/flags.mk
CC     ?= cc
CMAKE  ?= cmake

.DEFAULT_GOAL := all

# SQIsign round-1 submission (NIST on-ramp round 1, YAML version v1.0):
# SQISign/the-sqisign tag nist-v1. Its src/ is identical to both
# Reference_Implementation/ and Additional_Implementations/broadwell/ of NIST's
# sqisign-submission.zip (only README.md and the default SQISIGN_BUILD_TYPE in
# CMakeLists.txt differ). Signing needs GMP (system libgmp).
REF    := ref
BUILD  := build

# Round 1 offers the broadwell build type (reference code + x86-64 assembly field
# arithmetic, the submission's Additional_Implementations/broadwell, the fastest x86
# option) for lvl1 only: with SQISIGN_BUILD_TYPE=broadwell its CMakeLists.txt builds just
# lvl1. lvl3 and lvl5 exist only as portable C (build type ref). Two CMake trees.
BW_BUILD  := $(BUILD)/cmake-broadwell
REF_BUILD := $(BUILD)/cmake-ref

PARAM_SETS :=

# $(1) = file stem (e.g. sqisign_r1_lvl1)
# $(2) = variant   (lvl1/lvl3/lvl5)
# $(3) = CMake build dir
# $(4) = gf implementation dir (broadwell/ref)
# The shim calls sqisign_keypair/sign/open (libsqisign_<v>.a) directly; the nistapi lib is
# not linked (see shim_template.c).
# -Wl,--exclude-libs,ALL (link-time only, no codegen change): the round-1 broadwell
# fp_asm.S defines its constant p_plus_1 as a plain .global (default visibility) and
# addresses it PC-relatively, which ld refuses in a shared object for a preemptible symbol.
# Hiding the archive symbols makes it DSO-local; the bench entry points live in the shim
# and stay exported. Upstream builds static libraries only.
define SQISIGN_RULES
PARAM_SETS += $(1)
$(BUILD)/$(1).so: $(1)_shim.c $(3)/.built | $(BUILD)
	$$(CC) $(BENCH_CFLAGS) -fPIC -std=c99 -shared \
	    $(1)_shim.c \
	    $(3)/src/libsqisign_$(2).a \
	    $(3)/src/protocols/ref/$(2)/libsqisign_protocols_$(2).a \
	    $(3)/src/id2iso/ref/$(2)/libsqisign_id2iso_$(2).a \
	    $(3)/src/klpt/ref/$(2)/libsqisign_klpt_$(2).a \
	    $(3)/src/quaternion/ref/generic/libsqisign_quaternion_generic.a \
	    $(3)/src/precomp/ref/$(2)/libsqisign_precomp_$(2).a \
	    $(3)/src/intbig/ref/generic/libsqisign_intbig_generic.a \
	    $(3)/src/ec/ref/$(2)/libsqisign_ec_$(2).a \
	    $(3)/src/gf/$(4)/$(2)/libsqisign_gf_$(2).a \
	    $(3)/src/common/generic/libsqisign_common_sys.a \
	    -Wl,--exclude-libs,ALL -lgmp -lm \
	    -o $$@
endef

$(eval $(call SQISIGN_RULES,sqisign_r1_lvl1,lvl1,$(BW_BUILD),broadwell))
$(eval $(call SQISIGN_RULES,sqisign_r1_lvl3,lvl3,$(REF_BUILD),ref))
$(eval $(call SQISIGN_RULES,sqisign_r1_lvl5,lvl5,$(REF_BUILD),ref))

TARGETS    := $(addprefix $(BUILD)/, $(addsuffix .so, $(PARAM_SETS)))
SHIM_FILES := $(addsuffix _shim.c, $(PARAM_SETS))

.PHONY: all clean check-submodule

all: check-submodule $(SHIM_FILES) $(TARGETS)

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

check-submodule:
	@test -f $(REF)/CMakeLists.txt || { \
	    echo "ERROR: bench/schemes/sqisign_r1/ref/ is empty."; \
	    echo "Run: git submodule update --init bench/schemes/sqisign_r1/ref"; \
	    exit 1; }

# Compiler flags: as bench/schemes/sqisign, a build type upstream does not know (Bench)
# whose flags (the global set, then -fno-unroll-loops) come after upstream's
# CMAKE_C_FLAGS (-O3 -std=c99 -fvisibility=hidden -funroll-loops and -W flags; round 1
# has ENABLE_STRICT off by default and does not enable LTO, so no no_lto.cmake).
# Exception, kept: -fvisibility=hidden (needed: the broadwell fp_asm.S addresses the C
# constant p PC-relatively, which only links into our shared objects if the symbol is
# DSO-local).
SQISIGN_CMAKE_CFLAGS := $(BENCH_CFLAGS) -fPIC -fno-unroll-loops

# $(1) = build dir, $(2) = SQISIGN_BUILD_TYPE
define SQISIGN_CMAKE
$(1)/.built: $(REF)/CMakeLists.txt Makefile ../../../bench-common/flags.mk
	mkdir -p $(1)
	cd $(1) && $$(CMAKE) ../../$(REF) \
	    -DCMAKE_BUILD_TYPE=Bench \
	    -DCMAKE_C_FLAGS_BENCH='$(SQISIGN_CMAKE_CFLAGS)' \
	    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
	    -DSQISIGN_BUILD_TYPE=$(2) \
	    -DENABLE_TESTS=OFF \
	    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
	$$(MAKE) -C $(1)
	touch $$@
endef

$(eval $(call SQISIGN_CMAKE,$(BW_BUILD),broadwell))
$(eval $(call SQISIGN_CMAKE,$(REF_BUILD),ref))

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
