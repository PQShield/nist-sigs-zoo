include ../../../bench-common/flags.mk
CC     ?= cc
BUILD   = build
REF     = $(CURDIR)/ref

# name:variant:choice, must match params.tsv
SETS = mikefast1:p308_633:fast mikefast3:p474_593:fast mikefast5:p628_317:fast \
       mikerigorous1:p374_117:conservative mikerigorous3:p566_77:conservative mikerigorous5:p758_41:conservative
SOS  = $(foreach s,$(SETS),$(word 1,$(subst :, ,$(s))))
SHIM_FILES := $(addsuffix _shim.c, $(SOS))
CFLAGS := -Wall -Wextra $(BENCH_CFLAGS) -fPIC

.PHONY: all clean
all: $(SHIM_FILES) $(addprefix $(BUILD)/, $(addsuffix .so, $(SOS)))

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

# Upstream is CMake-based: one static-library build per prime choice. The
# broadwell build type uses assembly-optimised GF arithmetic (x86_64).
# Compiler flags: upstream's .cmake/flags.cmake appends -O2 -funroll-loops
# -fvisibility=hidden (and, in its default ENABLE_STRICT mode, -fno-strict-aliasing) to
# CMAKE_C_FLAGS and enables LTO for every build type but Debug. CMAKE_C_FLAGS_<TYPE>
# comes after CMAKE_C_FLAGS, so we use a build type upstream does not know (Bench) whose
# flags are the global set (flags.mk) followed by -fno-unroll-loops; no_lto.cmake turns
# LTO off. Exceptions, kept: -fvisibility=hidden and -fno-strict-aliasing (upstream's
# default build, as for SQIsign, whose asm needs the former), and the gf/sat64 target's
# own -fno-tree-vectorize -fno-tree-slp-vectorize, which CMake places after all
# configurable flags (overriding them would mean editing upstream's CMakeLists).
MIKE_CMAKE_CFLAGS := $(BENCH_CFLAGS) -fPIC -fno-unroll-loops

$(BUILD)/%/.built: Makefile no_lto.cmake ../../../bench-common/flags.mk
	cmake -S $(REF) -B $(BUILD)/$* -DMIKE_BUILD_TYPE=broadwell -DMIKE_PRIME_CHOICE=$* \
	    -DCMAKE_BUILD_TYPE=Bench -DCMAKE_C_FLAGS_BENCH='$(MIKE_CMAKE_CFLAGS)' \
	    -DCMAKE_PROJECT_INCLUDE=$(CURDIR)/no_lto.cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
	    -DENABLE_TESTS=OFF -DBUILD_SHARED_LIBS=OFF >/dev/null
	cmake --build $(BUILD)/$* -j >/dev/null
	touch $@

# combo: $(1)=so basename, $(2)=upstream prime, $(3)=prime choice
define combo
$$(BUILD)/$(1)_shim.o: $(1)_shim.c ../../scheme.h | $$(BUILD)
	$$(CC) $$(CFLAGS) -c $(1)_shim.c -o $$@
$$(BUILD)/$(1).so: $$(BUILD)/$(1)_shim.o $$(BUILD)/$(3)/.built
	$$(CC) -shared -o $$@ $$(BUILD)/$(1)_shim.o -Wl,--start-group \
	    $$$$(find $$(BUILD)/$(3) -name 'libmike_*$(2).a' ! -name '*_test.a') \
	    $$(BUILD)/$(3)/src/common/generic/libmike_common_sys.a -Wl,--end-group
endef
$(foreach s,$(SETS),$(eval $(call combo,$(word 1,$(subst :, ,$(s))),$(word 2,$(subst :, ,$(s))),$(word 3,$(subst :, ,$(s))))))

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
