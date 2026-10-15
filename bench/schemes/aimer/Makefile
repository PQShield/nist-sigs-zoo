CC      ?= cc
# Upstream's Makefile builds with -O3 -fomit-frame-pointer (portable C, no SIMD
# code); like the other schemes we add -march=native. RANDOMIZED_SIGNING stays
# undefined, as in upstream's default build.
CFLAGS  := -O3 -fomit-frame-pointer -march=native -mtune=native -Wall -Wextra -fPIC
BUILD   := build
# The upstream repo ships its code only as AIMer.zip; it is unpacked into build/src.
ZIP     := ref/AIMer.zip
SRC     := $(BUILD)/src/Reference_Implementation
UNZIPPED := $(BUILD)/src/.unzipped

PARAMS_LIST := 128f 128s 192f 192s 256f 256s
SOS        := $(foreach p,$(PARAMS_LIST),$(BUILD)/aimer$(p).so)
SHIM_FILES := $(foreach p,$(PARAMS_LIST),aimer$(p)_shim.c)

.DEFAULT_GOAL := all
.PHONY: all clean check_ref

all: check_ref $(SHIM_FILES) $(SOS)

check_ref:
	@if [ ! -f $(ZIP) ]; then \
	    echo "ERROR: bench/schemes/aimer/ref/ not populated." >&2; \
	    echo "Run: git submodule update --init bench/schemes/aimer/ref" >&2; \
	    exit 1; \
	fi

$(SHIM_FILES) &: shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(UNZIPPED): $(ZIP)
	rm -rf $(BUILD)/src
	mkdir -p $(BUILD)/src
	python3 -m zipfile -e $(ZIP) $(BUILD)/src
	touch $@

COMMON_SRCS := aim3.c field_common.c hash.c sign.c tree.c common/fips202.c

# $(1) = PARAMS value (128f ...), $(2) = field source (field128.c ...)
define VARIANT_RULES
$(BUILD)/aimer$(1).so: aimer$(1)_shim.c $(UNZIPPED)
	$(CC) $(CFLAGS) -I$(SRC) -I../.. -DPARAMS=$(1) -shared -o $$@ \
	    aimer$(1)_shim.c $(addprefix $(SRC)/,$(COMMON_SRCS) $(2))
endef

$(foreach p,$(PARAMS_LIST),$(eval $(call VARIANT_RULES,$(p),field$(firstword $(subst f, ,$(subst s, ,$(p)))).c)))

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
