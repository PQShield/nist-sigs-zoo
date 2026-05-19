CC      ?= cc
CFLAGS  := -O2 -Wall -Wextra
BUILD    = build

MLDSA_DIR = schemes/mldsa
MLDSA_LIB = $(MLDSA_DIR)/build/libmldsa.a

.PHONY: all clean bench schemes

all: bench

$(MLDSA_LIB):
	$(MAKE) -C $(MLDSA_DIR) CC=$(CC)

$(BUILD)/main.o: main.c harness.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

bench: $(BUILD)/main.o $(MLDSA_LIB) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	$(MAKE) -C $(MLDSA_DIR) clean
