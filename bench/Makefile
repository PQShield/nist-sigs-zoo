CC      ?= cc
CFLAGS  := -O2 -Wall -Wextra
BUILD    = build

MLDSA_DIR  = schemes/mldsa
MLDSA_LIB  = $(MLDSA_DIR)/build/libmldsa.a

SLHDSA_DIR = schemes/slhdsa
SLHDSA_LIB = $(SLHDSA_DIR)/build/libslhdsa.a

.PHONY: all clean bench

all: bench

$(MLDSA_LIB):
	$(MAKE) -C $(MLDSA_DIR) CC=$(CC)

$(SLHDSA_LIB):
	$(MAKE) -C $(SLHDSA_DIR) CC=$(CC)

$(BUILD)/main.o: main.c harness.h scheme.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

bench: $(BUILD)/main.o $(MLDSA_LIB) $(SLHDSA_LIB) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	$(MAKE) -C $(MLDSA_DIR) clean
	$(MAKE) -C $(SLHDSA_DIR) clean
