CC      ?= cc
CFLAGS  := -O3 -fomit-frame-pointer -fPIC -Wall -Wextra -I ref/ref
REF_DIR  = ref/ref
BUILD    = build

REF_SRCS = sign.c packing.c polyvec.c poly.c ntt.c reduce.c rounding.c \
           fips202.c symmetric-shake.c randombytes.c

.PHONY: all clean
all: $(BUILD)/mldsa44.so $(BUILD)/mldsa65.so $(BUILD)/mldsa87.so

# Compile dilithium mode-N objects (all with -fPIC for .so inclusion).
define mode_rules
$$(BUILD)/mode$(1)/%.o: $$(REF_DIR)/%.c | $$(BUILD)/mode$(1)
	$$(CC) $$(CFLAGS) -DDILITHIUM_MODE=$(1) -c $$< -o $$@
$$(BUILD)/mode$(1):
	mkdir -p $$@
endef

$(eval $(call mode_rules,2))
$(eval $(call mode_rules,3))
$(eval $(call mode_rules,5))

MODE2_OBJS = $(addprefix $(BUILD)/mode2/, $(REF_SRCS:.c=.o))
MODE3_OBJS = $(addprefix $(BUILD)/mode3/, $(REF_SRCS:.c=.o))
MODE5_OBJS = $(addprefix $(BUILD)/mode5/, $(REF_SRCS:.c=.o))

# Shim objects.
$(BUILD)/mldsa44_shim.o: mldsa44_shim.c ../../scheme.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/mldsa65_shim.o: mldsa65_shim.c ../../scheme.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/mldsa87_shim.o: mldsa87_shim.c ../../scheme.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# Each .so: shim + all mode-specific dilithium objects.
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
