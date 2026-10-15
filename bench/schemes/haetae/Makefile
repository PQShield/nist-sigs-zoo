CC      ?= cc
# Upstream's CMake release build: -O3 -std=c11 (portable C, no SIMD code); like the
# other schemes we add -march=native.
CFLAGS  := -O3 -std=c11 -march=native -mtune=native -Wall -Wextra -fPIC
BUILD   := build
# HAETAE 1.2.0 is released only as a zip on Google Drive: .source holds its URL
# (line 1) and sha256 (line 2); it is fetched into build/dl and unpacked into build/src.
URL     := $(shell sed -n 1p .source)
ZIP     := $(BUILD)/dl/HAETAE-1.2.0.zip
SRC     := $(BUILD)/src/HAETAE-1.2.0/reference_implementation
UNZIPPED := $(BUILD)/src/.unzipped

MODES      := 2 3 5
SOS        := $(foreach m,$(MODES),$(BUILD)/haetae$(m).so)
SHIM_FILES := $(foreach m,$(MODES),haetae$(m)_shim.c)

SRCS := decompose.c encoding.c fft.c fixpoint.c ntt.c packing.c poly.c polyvec.c \
        polymat.c polyfix.c reduce.c sampler.c sign.c fips202.c symmetric-shake.c \
        randombytes.c

.DEFAULT_GOAL := all
.PHONY: all clean

all: $(SHIM_FILES) $(SOS)

$(SHIM_FILES) &: shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(ZIP): .source
	mkdir -p $(@D)
	curl -fsSL --retry 3 -o $@.tmp '$(URL)'
	cd $(@D) && sed -n 2p ../../.source | sed 's/HAETAE-1.2.0.zip$$/HAETAE-1.2.0.zip.tmp/' | sha256sum -c - \
	    || { echo "SHA-256 mismatch for $@" >&2; rm -f $(@F).tmp; exit 1; }
	mv $@.tmp $@

$(UNZIPPED): $(ZIP)
	rm -rf $(BUILD)/src
	mkdir -p $(BUILD)/src
	python3 -m zipfile -e $(ZIP) $(BUILD)/src
	touch $@

define MODE_RULES
$(BUILD)/haetae$(1).so: haetae$(1)_shim.c $(UNZIPPED)
	$(CC) $(CFLAGS) -I$(SRC)/include -I../.. -DHAETAE_CONFIG_MODE=HAETAE_MODE$(1) -shared -o $$@ \
	    haetae$(1)_shim.c $(addprefix $(SRC)/src/,$(SRCS))
endef

$(foreach m,$(MODES),$(eval $(call MODE_RULES,$(m))))

clean:
	rm -rf $(BUILD)/src $(BUILD)/*.so
	rm -f $(SHIM_FILES)
