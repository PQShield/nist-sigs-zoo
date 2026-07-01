CC     ?= cc
BUILD   = build

# Locate OpenSSL: brew prefix > pkg-config > /usr
OSSL_PREFIX := $(shell brew --prefix openssl 2>/dev/null)
ifeq ($(OSSL_PREFIX),)
  OSSL_PREFIX := $(shell pkg-config --variable=prefix openssl 2>/dev/null)
endif
ifeq ($(OSSL_PREFIX),)
  OSSL_PREFIX := /usr
endif

OSSL_CFLAGS  = -I$(OSSL_PREFIX)/include
OSSL_LDFLAGS = -L$(OSSL_PREFIX)/lib -lcrypto -Wl,-rpath,$(OSSL_PREFIX)/lib

CFLAGS := -O3 -fomit-frame-pointer -fPIC -Wall -Wextra $(OSSL_CFLAGS)

X25519_SHIMS := ecdh_x25519_shim.c ecdh_x448_shim.c
NIST_SHIMS   := ecdh_p256_shim.c ecdh_p384_shim.c
SHIM_FILES   := $(X25519_SHIMS) $(NIST_SHIMS)

SHIMS   = ecdh_x25519 ecdh_x448 ecdh_p256 ecdh_p384
TARGETS = $(addprefix $(BUILD)/, $(addsuffix .so, $(SHIMS)))

.PHONY: all clean

all: $(SHIM_FILES) $(TARGETS)

$(X25519_SHIMS): x25519_shim_template.c x25519_params.tsv
	python3 ../../gen_shims.py x25519_shim_template.c x25519_params.tsv .

$(NIST_SHIMS): ecdh_nist_shim_template.c ecdh_nist_params.tsv
	python3 ../../gen_shims.py ecdh_nist_shim_template.c ecdh_nist_params.tsv .

$(BUILD):
	mkdir -p $@

$(BUILD)/%.o: %_shim.c ../../scheme.h classic_common.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.so: $(BUILD)/%.o
	$(CC) -shared -o $@ $^ $(OSSL_LDFLAGS)

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
