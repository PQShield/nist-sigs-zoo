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

SHIMS = rsa2048 rsa3072 rsa4096 ecdsa_p256 ecdsa_p384 ecdsa_p521 ed25519 ed448
TARGETS = $(addprefix $(BUILD)/, $(addsuffix .so, $(SHIMS)))

.PHONY: all clean

all: $(TARGETS)

$(BUILD):
	mkdir -p $@

$(BUILD)/%.o: %_shim.c ../../scheme.h classic_common.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.so: $(BUILD)/%.o
	$(CC) -shared -o $@ $^ $(OSSL_LDFLAGS)

clean:
	rm -rf $(BUILD)
