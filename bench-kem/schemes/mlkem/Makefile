CC      ?= cc
CFLAGS  := -O3 -fomit-frame-pointer -fPIC -mavx2 -mbmi2 -mpopcnt -march=native \
            -mtune=native -Wall -Wextra -I ref/avx2
AVX2_DIR = ref/avx2
BUILD    = build

# C and assembly sources for the AVX2 implementation.
C_SRCS = kem.c indcpa.c polyvec.c poly.c consts.c rejsample.c cbd.c verify.c \
         fips202.c fips202x4.c symmetric-shake.c randombytes.c
S_SRCS = fq.S shuffle.S ntt.S invntt.S basemul.S

SHIM_FILES := mlkem512_shim.c mlkem768_shim.c mlkem1024_shim.c

.PHONY: all clean
all: $(SHIM_FILES) $(BUILD)/mlkem512.so $(BUILD)/mlkem768.so $(BUILD)/mlkem1024.so

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

# Per-KYBER_K compilation — the namespace (kyber512/768/1024) is selected by KYBER_K.
define mode_rules
$$(BUILD)/k$(1)/%.o: $$(AVX2_DIR)/%.c | $$(BUILD)/k$(1)
	$$(CC) $$(CFLAGS) -DKYBER_K=$(1) -c $$< -o $$@

$$(BUILD)/k$(1)/%.o: $$(AVX2_DIR)/%.S | $$(BUILD)/k$(1)
	$$(CC) $$(CFLAGS) -DKYBER_K=$(1) -c $$< -o $$@

# Keccak4x lives in a subdir; build it explicitly into the same per-K dir.
$$(BUILD)/k$(1)/keccak4x.o: $$(AVX2_DIR)/keccak4x/KeccakP-1600-times4-SIMD256.c | $$(BUILD)/k$(1)
	$$(CC) $$(CFLAGS) -DKYBER_K=$(1) -c $$< -o $$@

$$(BUILD)/k$(1):
	mkdir -p $$@
endef

$(eval $(call mode_rules,2))
$(eval $(call mode_rules,3))
$(eval $(call mode_rules,4))

ALL_SRCS = $(C_SRCS) $(S_SRCS)

K2_OBJS = $(addprefix $(BUILD)/k2/, $(ALL_SRCS:.S=.o)) $(BUILD)/k2/keccak4x.o
K3_OBJS = $(addprefix $(BUILD)/k3/, $(ALL_SRCS:.S=.o)) $(BUILD)/k3/keccak4x.o
K4_OBJS = $(addprefix $(BUILD)/k4/, $(ALL_SRCS:.S=.o)) $(BUILD)/k4/keccak4x.o

K2_OBJS := $(K2_OBJS:.c=.o)
K3_OBJS := $(K3_OBJS:.c=.o)
K4_OBJS := $(K4_OBJS:.c=.o)

$(BUILD)/mlkem512_shim.o:  mlkem512_shim.c  ../../scheme.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/mlkem768_shim.o:  mlkem768_shim.c  ../../scheme.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/mlkem1024_shim.o: mlkem1024_shim.c ../../scheme.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/mlkem512.so:  $(BUILD)/mlkem512_shim.o  $(K2_OBJS)
	$(CC) -shared -o $@ $^
$(BUILD)/mlkem768.so:  $(BUILD)/mlkem768_shim.o  $(K3_OBJS)
	$(CC) -shared -o $@ $^
$(BUILD)/mlkem1024.so: $(BUILD)/mlkem1024_shim.o $(K4_OBJS)
	$(CC) -shared -o $@ $^

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
