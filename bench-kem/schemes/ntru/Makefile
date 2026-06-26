CC     ?= cc
BUILD   = build
REF     = ref
NS      = ntru_

# CRYPTO_NAMESPACE must match the NTRU_NAMESPACE the .s assembly is generated with.
CFLAGS := -O3 -fomit-frame-pointer -march=native -mtune=native -fPIC -Wall -Wextra \
          -DCRYPTO_NAMESPACE\(s\)=$(NS)\#\#s

# The portable C source set differs per parameter set (e.g. HRSS has no
# crypto_sort_int32.c / poly_lift.c), so each .so globs its dir's *.c at recipe time
# — after `make asm` has generated poly_s3_inv.c — excluding only cpucycles.c.
SETS = ntruhps2048509 ntruhps2048677 ntruhps4096821 ntruhrss701
SHIM_FILES := $(addsuffix _shim.c, $(SETS))

.PHONY: all clean
all: $(SHIM_FILES) $(addprefix $(BUILD)/, $(addsuffix .so, $(SETS)))

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(BUILD):
	mkdir -p $@

# combo: $(1)=scheme name (ntruhps2048509)  $(2)=upstream impl dir (avx2-hps2048509)
# The upstream codegen writes *.s + poly_s3_inv.c into the impl dir; *.s is globbed
# at recipe time (after codegen). The whole set compiles in one cc invocation.
define combo
$$(BUILD)/$(1).so: $(1)_shim.c ../../scheme.h | $$(BUILD)
	$$(MAKE) -C $(REF)/$(2) asm NTRU_NAMESPACE=$(NS)
	$$(CC) $$(CFLAGS) -I$(REF)/$(2) -shared -o $$@ $(1)_shim.c \
	    $$$$(ls $(REF)/$(2)/*.c | grep -v cpucycles) $(REF)/$(2)/*.s
endef

$(eval $(call combo,ntruhps2048509,avx2-hps2048509))
$(eval $(call combo,ntruhps2048677,avx2-hps2048677))
$(eval $(call combo,ntruhps4096821,avx2-hps4096821))
$(eval $(call combo,ntruhrss701,avx2-hrss701))

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
	for d in avx2-hps2048509 avx2-hps2048677 avx2-hps4096821 avx2-hrss701; do \
	    $(MAKE) -C $(REF)/$$d clean >/dev/null 2>&1 || true; done
