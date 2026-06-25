CC     ?= cc
CFLAGS := -O2 -Wall -Wextra
BUILD   = build

CLASSIC_SOS = $(addprefix schemes/classic/build/, ecdh_x25519.so ecdh_x448.so ecdh_p256.so ecdh_p384.so)
MLKEM_SOS   = $(addprefix schemes/mlkem/build/,   mlkem512.so mlkem768.so mlkem1024.so)
HQC_SOS     = $(addprefix schemes/hqc/build/,     hqc1.so hqc3.so hqc5.so)

ALL_SOS     = $(CLASSIC_SOS) $(MLKEM_SOS) $(HQC_SOS)
SCHEME_DIRS = classic mlkem hqc
STAMPS      = $(addprefix schemes/, $(addsuffix /.stamp, $(SCHEME_DIRS)))

.PHONY: all clean

all: bench-kem

# Per-scheme source prerequisites that invalidate the stamp.
schemes/classic/.stamp: schemes/classic/Makefile \
    $(wildcard schemes/classic/*_template.c schemes/classic/*_params.tsv schemes/classic/*.h)
schemes/mlkem/.stamp:   schemes/mlkem/Makefile schemes/mlkem/shim_template.c schemes/mlkem/params.tsv
schemes/hqc/.stamp:     schemes/hqc/Makefile   schemes/hqc/shim_template.c   schemes/hqc/params.tsv

# Shared recipe for every scheme stamp.
$(STAMPS): schemes/%/.stamp:
	$(MAKE) -C $(@D) CC=$(CC)
	touch $@

# Each scheme's .so files depend on its stamp.
$(CLASSIC_SOS): schemes/classic/.stamp
$(MLKEM_SOS):   schemes/mlkem/.stamp
$(HQC_SOS):     schemes/hqc/.stamp

# Generate the SO path list included by main.c.
$(BUILD)/so_paths.h: Makefile | $(BUILD)
	@printf '"%s",\n' $(ALL_SOS) > $@

$(BUILD)/main.o: main.c harness.h loader.h scheme.h $(BUILD)/so_paths.h | $(BUILD)
	$(CC) $(CFLAGS) -I$(BUILD) -c $< -o $@

$(BUILD)/loader.o: loader.c loader.h scheme.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

bench-kem: $(BUILD)/main.o $(BUILD)/loader.o $(ALL_SOS) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(BUILD)/main.o $(BUILD)/loader.o -ldl -lpthread

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	rm -f $(STAMPS)
	for d in $(SCHEME_DIRS); do $(MAKE) -C schemes/$$d clean; done
