CC      ?= cc
CFLAGS  := -O3 -fomit-frame-pointer -fPIC -mavx2 -mpopcnt -march=native \
            -mtune=native -Wall -Wextra -I ref/avx2
AVX2_DIR = ref/avx2
BUILD    = build

# C sources and assembly sources for the AVX2 implementation (SHAKE variant).
C_SRCS = sign.c packing.c polyvec.c poly.c consts.c rejsample.c rounding.c \
         fips202.c fips202x4.c symmetric-shake.c randombytes.c
S_SRCS = ntt.S invntt.S pointwise.S shuffle.S f1600x4.S

SHIM_FILES := mldsa44_shim.c mldsa65_shim.c mldsa87_shim.c

.PHONY: all clean
all: $(SHIM_FILES) $(BUILD)/mldsa44.so $(BUILD)/mldsa65.so $(BUILD)/mldsa87.so

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

define mode_rules
$$(BUILD)/mode$(1)/%.o: $$(AVX2_DIR)/%.c | $$(BUILD)/mode$(1)
	$$(CC) $$(CFLAGS) -DDILITHIUM_MODE=$(1) -c $$< -o $$@

$$(BUILD)/mode$(1)/%.o: $$(AVX2_DIR)/%.S | $$(BUILD)/mode$(1)
	$$(CC) $$(CFLAGS) -DDILITHIUM_MODE=$(1) -c $$< -o $$@

$$(BUILD)/mode$(1):
	mkdir -p $$@
endef

$(eval $(call mode_rules,2))
$(eval $(call mode_rules,3))
$(eval $(call mode_rules,5))

ALL_SRCS = $(C_SRCS) $(S_SRCS)

MODE2_OBJS = $(addprefix $(BUILD)/mode2/, $(ALL_SRCS:.S=.o))
MODE3_OBJS = $(addprefix $(BUILD)/mode3/, $(ALL_SRCS:.S=.o))
MODE5_OBJS = $(addprefix $(BUILD)/mode5/, $(ALL_SRCS:.S=.o))

# .c → .o substitution (pattern above handles both .c and .S)
MODE2_OBJS := $(MODE2_OBJS:.c=.o)
MODE3_OBJS := $(MODE3_OBJS:.c=.o)
MODE5_OBJS := $(MODE5_OBJS:.c=.o)

$(BUILD)/mldsa44_shim.o: mldsa44_shim.c ../../scheme.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/mldsa65_shim.o: mldsa65_shim.c ../../scheme.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/mldsa87_shim.o: mldsa87_shim.c ../../scheme.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/mldsa44.so: $(BUILD)/mldsa44_shim.o $(MODE2_OBJS)
	$(CC) -shared -o $@ $^
$(BUILD)/mldsa65.so: $(BUILD)/mldsa65_shim.o $(MODE3_OBJS)
	$(CC) -shared -o $@ $^
$(BUILD)/mldsa87.so: $(BUILD)/mldsa87_shim.o $(MODE5_OBJS)
	$(CC) -shared -o $@ $^

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
