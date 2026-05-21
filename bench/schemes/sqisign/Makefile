CC     ?= cc
CMAKE  ?= cmake

.DEFAULT_GOAL := all

REF    := ref
BUILD  := build
CBUILD := $(BUILD)/cmake
CSRC   := $(CBUILD)/src

PARAM_SETS :=

# $(1) = file stem (e.g. sqisign_lvl1)
# $(2) = level     (e.g. lvl1)
define SQISIGN_RULES
PARAM_SETS += $(1)
$(BUILD)/$(1).so: $(1)_shim.c $(CSRC)/libsqisign_$(2)_nistapi.a | $(BUILD)
	$$(CC) -O2 -fPIC -std=c99 -shared \
	    $(1)_shim.c \
	    $(CSRC)/libsqisign_$(2)_nistapi.a \
	    $(CSRC)/libsqisign_$(2).a \
	    $(CSRC)/signature/ref/$(2)/libsqisign_signature_$(2).a \
	    $(CSRC)/verification/ref/$(2)/libsqisign_verification_$(2).a \
	    $(CSRC)/id2iso/ref/$(2)/libsqisign_id2iso_$(2).a \
	    $(CSRC)/quaternion/ref/generic/libsqisign_quaternion_generic.a \
	    $(CSRC)/hd/ref/$(2)/libsqisign_hd_$(2).a \
	    $(CSRC)/ec/ref/$(2)/libsqisign_ec_$(2).a \
	    $(CSRC)/gf/ref/$(2)/libsqisign_gf_$(2).a \
	    $(CSRC)/mp/ref/generic/libsqisign_mp_generic.a \
	    $(CSRC)/precomp/ref/$(2)/libsqisign_precomp_$(2).a \
	    $(CSRC)/common/generic/libsqisign_common_sys.a \
	    -lgmp -lm \
	    -o $$@
endef

$(eval $(call SQISIGN_RULES,sqisign_lvl1,lvl1))
$(eval $(call SQISIGN_RULES,sqisign_lvl3,lvl3))
$(eval $(call SQISIGN_RULES,sqisign_lvl5,lvl5))

TARGETS    := $(addprefix $(BUILD)/, $(addsuffix .so, $(PARAM_SETS)))
SHIM_FILES := $(addsuffix _shim.c, $(PARAM_SETS))

.PHONY: all clean check-submodule

all: check-submodule $(SHIM_FILES) $(TARGETS)

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

check-submodule:
	@test -f $(REF)/CMakeLists.txt || { \
	    echo "ERROR: bench/schemes/sqisign/ref/ is empty."; \
	    echo "Run: git submodule update --init bench/schemes/sqisign/ref"; \
	    exit 1; }

# Build all cmake static libs in one go (cmake configure + make)
$(CSRC)/libsqisign_lvl1_nistapi.a \
$(CSRC)/libsqisign_lvl3_nistapi.a \
$(CSRC)/libsqisign_lvl5_nistapi.a: $(CBUILD)/.built
	@true

$(CBUILD)/.built: $(REF)/CMakeLists.txt | $(CBUILD)
	cd $(CBUILD) && $(CMAKE) ../../$(REF) \
	    -DCMAKE_BUILD_TYPE=Release \
	    -DSQISIGN_BUILD_TYPE=ref \
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
