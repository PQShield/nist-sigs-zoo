CC     ?= cc
CMAKE  ?= cmake

.DEFAULT_GOAL := all

REF    := ref
BUILD  := build
# SQIsign build type: broadwell = reference code + x86-64 assembly field
# arithmetic (Broadwell and later, the fastest x86 option upstream offers);
# ref = portable C. Only the gf module differs between the two.
SQISIGN_BUILD_TYPE ?= broadwell
GF_IMPL := $(if $(filter ref opt,$(SQISIGN_BUILD_TYPE)),ref,$(SQISIGN_BUILD_TYPE))
CBUILD := $(BUILD)/cmake-$(SQISIGN_BUILD_TYPE)
CSRC   := $(CBUILD)/src

PARAM_SETS :=

# $(1) = file stem (e.g. sqisign_lvl1)
# $(2) = variant   (e.g. p324_3) — nist-v3 dropped GMP and renamed variants
#        from lvl1/lvl3/lvl5 to the prime-shape names p324_3/p500_27/p664_17;
#        the mp and quaternion modules now build one lib per variant too
#        (no more shared .../ref/generic/... lib).
define SQISIGN_RULES
PARAM_SETS += $(1)
$(BUILD)/$(1).so: $(1)_shim.c $(CSRC)/libsqisign_$(2)_nistapi.a | $(BUILD)
	$$(CC) -O2 -fPIC -std=c99 -shared -DSQISIGN_NS_TYPE=$(SQISIGN_BUILD_TYPE) \
	    $(1)_shim.c \
	    $(CSRC)/libsqisign_$(2)_nistapi.a \
	    $(CSRC)/libsqisign_$(2).a \
	    $(CSRC)/signature/ref/$(2)/libsqisign_signature_$(2).a \
	    $(CSRC)/verification/ref/$(2)/libsqisign_verification_$(2).a \
	    $(CSRC)/id2iso/ref/$(2)/libsqisign_id2iso_$(2).a \
	    $(CSRC)/quaternion/ref/$(2)/libsqisign_quaternion_$(2).a \
	    $(CSRC)/hd/ref/$(2)/libsqisign_hd_$(2).a \
	    $(CSRC)/ec/ref/$(2)/libsqisign_ec_$(2).a \
	    $(CSRC)/gf/$(GF_IMPL)/$(2)/libsqisign_gf_$(2).a \
	    $(CSRC)/mp/ref/$(2)/libsqisign_mp_$(2).a \
	    $(CSRC)/precomp/ref/$(2)/libsqisign_precomp_$(2).a \
	    $(CSRC)/common/generic/libsqisign_common_sys.a \
	    -lm \
	    -o $$@
endef

$(eval $(call SQISIGN_RULES,sqisign_lvl1,p324_3))
$(eval $(call SQISIGN_RULES,sqisign_lvl3,p500_27))
$(eval $(call SQISIGN_RULES,sqisign_lvl5,p664_17))

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
$(CSRC)/libsqisign_p324_3_nistapi.a \
$(CSRC)/libsqisign_p500_27_nistapi.a \
$(CSRC)/libsqisign_p664_17_nistapi.a: $(CBUILD)/.built
	@true

$(CBUILD)/.built: $(REF)/CMakeLists.txt | $(CBUILD)
	cd $(CBUILD) && $(CMAKE) ../../$(REF) \
	    -DCMAKE_BUILD_TYPE=Release \
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
