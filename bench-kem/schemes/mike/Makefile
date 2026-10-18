CC     ?= cc
BUILD   = build
REF     = $(CURDIR)/ref

# name:variant:choice, must match params.tsv
SETS = mikefast1:p308_633:fast mikefast3:p474_593:fast mikefast5:p628_317:fast \
       mikerigorous1:p374_117:conservative mikerigorous3:p566_77:conservative mikerigorous5:p758_41:conservative
SOS  = $(foreach s,$(SETS),$(word 1,$(subst :, ,$(s))))
SHIM_FILES := $(addsuffix _shim.c, $(SOS))
CFLAGS := -O3 -fPIC -Wall -Wextra -march=native

.PHONY: all clean
all: $(SHIM_FILES) $(addprefix $(BUILD)/, $(addsuffix .so, $(SOS)))

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

# Upstream is CMake-based: one static-library build per prime choice. The
# broadwell build type uses assembly-optimised GF arithmetic (x86_64).
$(BUILD)/%/.built:
	cmake -S $(REF) -B $(BUILD)/$* -DMIKE_BUILD_TYPE=broadwell -DMIKE_PRIME_CHOICE=$* \
	    -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=OFF -DBUILD_SHARED_LIBS=OFF >/dev/null
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
