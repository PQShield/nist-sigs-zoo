include ../../../bench-common/flags.mk
CC      ?= cc
# Upstream's CMake release build: -O3 -std=c11 (portable C, no SIMD code); we keep
# -std=c11 and use the global flag set (flags.mk).
CFLAGS  := -std=c11 -Wall -Wextra $(BENCH_CFLAGS) -fPIC
BUILD   := build
# HAETAE 1.2.0 is released only as a zip on Google Drive. The zip is vendored here
# (MIT licence, HAETAE-1.2.0/LICENSE in the zip), since the download link may go away;
# .source holds the original URL (line 1) and its sha256 (line 2), checked before unpacking.
ZIP     := HAETAE-1.2.0.zip
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

$(UNZIPPED): $(ZIP) .source
	sed -n 2p .source | sha256sum -c - || { echo "SHA-256 mismatch for $(ZIP)" >&2; exit 1; }
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
