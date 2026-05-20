CC     ?= cc
CFLAGS := -O2 -Wall -Wextra
BUILD   = build

MLDSA_DIR  = schemes/mldsa
MLDSA_SOS  = $(MLDSA_DIR)/build/mldsa44.so \
             $(MLDSA_DIR)/build/mldsa65.so \
             $(MLDSA_DIR)/build/mldsa87.so

SLHDSA_DIR = schemes/slhdsa
SLHDSA_SOS = $(addprefix $(SLHDSA_DIR)/build/, \
    slhdsa_sha2_128s.so  slhdsa_shake_128s.so \
    slhdsa_sha2_128f.so  slhdsa_shake_128f.so \
    slhdsa_sha2_192s.so  slhdsa_shake_192s.so \
    slhdsa_sha2_192f.so  slhdsa_shake_192f.so \
    slhdsa_sha2_256s.so  slhdsa_shake_256s.so \
    slhdsa_sha2_256f.so  slhdsa_shake_256f.so)

FNDSA_DIR = schemes/fndsa
FNDSA_SOS = $(FNDSA_DIR)/build/fndsa512.so \
            $(FNDSA_DIR)/build/fndsa1024.so

CLASSIC_DIR = schemes/classic
CLASSIC_SOS = $(addprefix $(CLASSIC_DIR)/build/, \
    rsa2048.so rsa3072.so rsa4096.so \
    ecdsa_p256.so ecdsa_p384.so ecdsa_p521.so \
    ed25519.so ed448.so)

HAWK_DIR = schemes/hawk
HAWK_SOS = $(HAWK_DIR)/build/hawk512.so \
           $(HAWK_DIR)/build/hawk1024.so

MAYO_DIR = schemes/mayo
MAYO_SOS = $(addprefix $(MAYO_DIR)/build/, mayo1.so mayo2.so mayo3.so mayo5.so)

.PHONY: all clean schemes

all: bench

$(MLDSA_SOS):
	$(MAKE) -C $(MLDSA_DIR) CC=$(CC)

$(SLHDSA_SOS):
	$(MAKE) -C $(SLHDSA_DIR) CC=$(CC)

$(FNDSA_SOS):
	$(MAKE) -C $(FNDSA_DIR) CC=$(CC)

$(CLASSIC_SOS):
	$(MAKE) -C $(CLASSIC_DIR) CC=$(CC)

$(HAWK_SOS):
	$(MAKE) -C $(HAWK_DIR) CC=$(CC)

$(MAYO_SOS):
	$(MAKE) -C $(MAYO_DIR) CC=$(CC)

$(BUILD)/main.o: main.c harness.h loader.h scheme.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/loader.o: loader.c loader.h scheme.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

bench: $(BUILD)/main.o $(BUILD)/loader.o $(MLDSA_SOS) $(SLHDSA_SOS) $(FNDSA_SOS) $(CLASSIC_SOS) $(HAWK_SOS) $(MAYO_SOS) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(BUILD)/main.o $(BUILD)/loader.o -ldl

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	$(MAKE) -C $(MLDSA_DIR) clean
	$(MAKE) -C $(SLHDSA_DIR) clean
	$(MAKE) -C $(FNDSA_DIR) clean
	$(MAKE) -C $(CLASSIC_DIR) clean
	$(MAKE) -C $(HAWK_DIR) clean
	$(MAKE) -C $(MAYO_DIR) clean
