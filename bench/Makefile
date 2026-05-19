CC     ?= cc
CFLAGS := -O2 -Wall -Wextra
BUILD   = build

MLDSA_DIR  = schemes/mldsa
MLDSA_SOS  = $(MLDSA_DIR)/build/mldsa44.so \
             $(MLDSA_DIR)/build/mldsa65.so \
             $(MLDSA_DIR)/build/mldsa87.so

SLHDSA_DIR = schemes/slhdsa

.PHONY: all clean schemes

all: bench

$(MLDSA_SOS):
	$(MAKE) -C $(MLDSA_DIR) CC=$(CC)

$(BUILD)/main.o: main.c harness.h loader.h scheme.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/loader.o: loader.c loader.h scheme.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

bench: $(BUILD)/main.o $(BUILD)/loader.o $(MLDSA_SOS) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(BUILD)/main.o $(BUILD)/loader.o -ldl

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	$(MAKE) -C $(MLDSA_DIR) clean
	$(MAKE) -C $(SLHDSA_DIR) clean
