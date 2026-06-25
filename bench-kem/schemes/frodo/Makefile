CC     ?= cc
BUILD   = build
FRODO   = ref
COMMON  = $(FRODO)/common
SRC     = $(FRODO)/FrodoKEM/src

# Standard FrodoKEM (the ISO/salted variant), "fast" AVX2 implementation, no OpenSSL
# (upstream's own aes_ni.c for AES-NI). NIX + _AMD64_ + _FAST_ are fixed; the
# matrix-A generation method (_AES128_FOR_A_ / _SHAKE128_FOR_A_) is set per combo.
CFLAGS := -O3 -fomit-frame-pointer -fPIC -Wall -Wextra -std=gnu11 \
          -mavx2 -maes -msse2 -march=native -mtune=native \
          -DNIX -D_AMD64_ -D_FAST_ \
          -I$(SRC) -I$(COMMON)/random -I$(COMMON)/sha3 -I$(COMMON)/sha3/keccak4x \
          -I$(COMMON)/aes

# Generic (level/method-independent) objects, built once and shared by every .so.
# Unused objects in a given .so (e.g. aes_ni in a SHAKE build) are harmless. config.h
# demands an A-generation define even where the file never uses it (util.c), so the
# generic compiles carry a placeholder one; the real choice lives in each frodo*.o.
GEN_CFLAGS = $(CFLAGS) -D_SHAKE128_FOR_A_
GEN_OBJS = $(addprefix $(BUILD)/common/, \
             random.o fips202.o fips202x4.o KeccakP-1600-times4-SIMD256.o aes_ni.o util.o)

SOS = frodo640aes frodo640shake frodo976aes frodo976shake frodo1344aes frodo1344shake
SHIM_FILES := $(addsuffix _shim.c, $(SOS))

.PHONY: all clean
all: $(SHIM_FILES) $(addprefix $(BUILD)/, $(addsuffix .so, $(SOS)))

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(BUILD) $(BUILD)/common:
	mkdir -p $@

# Generic object pattern rules (one per source root; make selects by existing prereq).
$(BUILD)/common/%.o: $(COMMON)/random/%.c | $(BUILD)/common
	$(CC) $(GEN_CFLAGS) -c $< -o $@
$(BUILD)/common/%.o: $(COMMON)/sha3/%.c | $(BUILD)/common
	$(CC) $(GEN_CFLAGS) -c $< -o $@
$(BUILD)/common/%.o: $(COMMON)/sha3/keccak4x/%.c | $(BUILD)/common
	$(CC) $(GEN_CFLAGS) -c $< -o $@
$(BUILD)/common/%.o: $(COMMON)/aes/%.c | $(BUILD)/common
	$(CC) $(GEN_CFLAGS) -c $< -o $@
$(BUILD)/common/%.o: $(SRC)/%.c | $(BUILD)/common
	$(CC) $(GEN_CFLAGS) -c $< -o $@

# combo: $(1)=so basename  $(2)=level source (frodo640/976/1344)  $(3)=A-generation define
define combo
$$(BUILD)/$(1).o: $(SRC)/$(2).c | $$(BUILD)
	$$(CC) $$(CFLAGS) -D$(3) -c $$< -o $$@
$$(BUILD)/$(1)_shim.o: $(1)_shim.c ../../scheme.h | $$(BUILD)
	$$(CC) $$(CFLAGS) -c $$< -o $$@
$$(BUILD)/$(1).so: $$(BUILD)/$(1).o $$(BUILD)/$(1)_shim.o $$(GEN_OBJS)
	$$(CC) -shared -o $$@ $$^ -lm
endef

$(eval $(call combo,frodo640aes,frodo640,_AES128_FOR_A_))
$(eval $(call combo,frodo640shake,frodo640,_SHAKE128_FOR_A_))
$(eval $(call combo,frodo976aes,frodo976,_AES128_FOR_A_))
$(eval $(call combo,frodo976shake,frodo976,_SHAKE128_FOR_A_))
$(eval $(call combo,frodo1344aes,frodo1344,_AES128_FOR_A_))
$(eval $(call combo,frodo1344shake,frodo1344,_SHAKE128_FOR_A_))

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
