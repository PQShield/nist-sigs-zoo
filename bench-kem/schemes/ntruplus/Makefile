CC     ?= cc
BUILD   = build
REF     = ref/Additional_Implementation/avx2

CFLAGS := -O3 -fomit-frame-pointer -fPIC -Wall -Wextra \
          -mavx2 -mbmi2 -mpopcnt -maes -march=native -mtune=native

SETS = ntruplus768 ntruplus864 ntruplus1152
SHIM_FILES := $(addsuffix _shim.c, $(SETS))

.PHONY: all clean
all: $(SHIM_FILES) $(addprefix $(BUILD)/, $(addsuffix .so, $(SETS)))

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(BUILD):
	mkdir -p $@

# combo: $(1)=scheme name (ntruplus768)  $(2)=upstream per-set dir name (NTRU+768)
# consts.c defines lookup tables (_16xv, zetas, ...) that the hand-written AVX2
# asm addresses %rip-relative; those relocations only resolve in a shared object
# if the symbols are DSO-local, so consts.c alone is compiled with
# -fvisibility=hidden. kem.c keeps default visibility so crypto_kem_* stay
# dlsym-able.
define combo
$$(BUILD)/$(1)_consts.o: $(REF)/$(2)/consts.c | $$(BUILD)
	$$(CC) $$(CFLAGS) -fvisibility=hidden -I$(REF)/$(2) -I$(REF)/$(2)/fips202 -c $$< -o $$@

$$(BUILD)/$(1).so: $(1)_shim.c $$(BUILD)/$(1)_consts.o ../../scheme.h | $$(BUILD)
	$$(CC) $$(CFLAGS) -I$(REF)/$(2) -I$(REF)/$(2)/fips202 \
	    -shared -o $$@ $(1)_shim.c $$(BUILD)/$(1)_consts.o \
	    $(REF)/$(2)/kem.c $(REF)/$(2)/poly.c \
	    $(REF)/$(2)/symmetric.c $(REF)/$(2)/randombytes.c \
	    $(REF)/$(2)/fips202/fips202.c \
	    $(REF)/$(2)/asm/*.s $(REF)/$(2)/fips202/KeccakP-1600-AVX2.s
endef

$(eval $(call combo,ntruplus768,NTRU+768))
$(eval $(call combo,ntruplus864,NTRU+864))
$(eval $(call combo,ntruplus1152,NTRU+1152))

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
