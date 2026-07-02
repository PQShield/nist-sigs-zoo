CC     ?= cc
BUILD   = build
REF     = ref/AVX_Implementation_KEM

# Locate OpenSSL (rng.c's AES256-CTR-DRBG uses EVP): brew prefix > pkg-config > /usr
OSSL_PREFIX := $(shell brew --prefix openssl 2>/dev/null)
ifeq ($(OSSL_PREFIX),)
  OSSL_PREFIX := $(shell pkg-config --variable=prefix openssl 2>/dev/null)
endif
ifeq ($(OSSL_PREFIX),)
  OSSL_PREFIX := /usr
endif

CFLAGS  := -O3 -fomit-frame-pointer -fPIC -mavx2 -march=native -Wall -Wextra -I$(OSSL_PREFIX)/include
LDFLAGS := -L$(OSSL_PREFIX)/lib -lcrypto -Wl,-rpath,$(OSSL_PREFIX)/lib

# cpucycles.c is linked in only because kem.c/SABER_indcpa.c call cpucycles()
# (declared via SABER_indcpa.h) for their own internal (unused) profiling
# counters; upstream's own Makefile instead pulls it in via #include from a
# test-harness .c, which we don't build.
SRCS = pack_unpack.c verify.c fips202.c SABER_indcpa.c kem.c cbd.c rng.c cpucycles.c

SHIM_FILES := lightsaber_shim.c saber_shim.c firesaber_shim.c

.PHONY: all clean
all: $(SHIM_FILES) $(BUILD)/lightsaber.so $(BUILD)/saber.so $(BUILD)/firesaber.so

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

# Per-parameter-set: api.h hardcodes "#define SABER_TYPE Saber" with no
# #ifndef guard, so the set can't be chosen via -D on the command line (the
# header's own #define wins over a command-line macro of the same name). So
# copy the whole reference tree per set into build/ and patch the copy's
# api.h — quoted #include "api.h" resolves relative to the including file's
# own directory first, so the patched copy shadows the submodule original
# without ever touching the checked-out submodule.
define lvl_rules
$$(BUILD)/$(1)/.copied:
	mkdir -p $$(BUILD)/$(1)
	cp $(REF)/*.c $(REF)/*.h $$(BUILD)/$(1)/
	cp -r $(REF)/polymul $$(BUILD)/$(1)/
	sed -i.bak 's/#define SABER_TYPE Saber/#define SABER_TYPE $(2)/' $$(BUILD)/$(1)/api.h
	rm -f $$(BUILD)/$(1)/api.h.bak
	touch $$@

$$(BUILD)/$(1)/%.o: $$(BUILD)/$(1)/.copied
	$$(CC) $$(CFLAGS) -c $$(BUILD)/$(1)/$$*.c -o $$@

$$(BUILD)/$(1)_shim.o: $(1)_shim.c ../../scheme.h | $$(BUILD)
	$$(CC) $$(CFLAGS) -c $(1)_shim.c -o $$@

$$(BUILD)/$(1).so: $$(BUILD)/$(1)_shim.o $$(addprefix $$(BUILD)/$(1)/,$$(SRCS:.c=.o))
	$$(CC) -shared -o $$@ $$^ $$(LDFLAGS)
endef

$(eval $(call lvl_rules,lightsaber,LightSaber))
$(eval $(call lvl_rules,saber,Saber))
$(eval $(call lvl_rules,firesaber,FireSaber))

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
