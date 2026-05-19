CC      ?= cc
AR      ?= ar
CFLAGS  := -O3 -fomit-frame-pointer -Wall -Wextra -I ref/ref

REF_DIR  = ref/ref
BUILD    = build

REF_SRCS = sign.c packing.c polyvec.c poly.c ntt.c reduce.c rounding.c \
           fips202.c symmetric-shake.c randombytes.c

# Must be first so `make` with no args builds everything, not just mode2.
.PHONY: all clean
all: $(BUILD)/libmldsa.a

# Build objects for each dilithium mode in separate subdirs to avoid conflicts.
define mode_rules
$$(BUILD)/mode$(1)/%.o: $$(REF_DIR)/%.c | $$(BUILD)/mode$(1)
	$$(CC) $$(CFLAGS) -DDILITHIUM_MODE=$(1) -c $$< -o $$@

$$(BUILD)/libdilithium$(1)_ref.a: $$(addprefix $$(BUILD)/mode$(1)/, $$(REF_SRCS:.c=.o))
	$$(AR) rcs $$@ $$^

$$(BUILD)/mode$(1):
	mkdir -p $$@
endef

$(eval $(call mode_rules,2))
$(eval $(call mode_rules,3))
$(eval $(call mode_rules,5))

LIBS = $(BUILD)/libdilithium2_ref.a \
       $(BUILD)/libdilithium3_ref.a \
       $(BUILD)/libdilithium5_ref.a

$(BUILD)/mldsa.o: mldsa.c mldsa.h ../../scheme.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

ALL_OBJS = $(BUILD)/mldsa.o \
           $(addprefix $(BUILD)/mode2/, $(REF_SRCS:.c=.o)) \
           $(addprefix $(BUILD)/mode3/, $(REF_SRCS:.c=.o)) \
           $(addprefix $(BUILD)/mode5/, $(REF_SRCS:.c=.o))

$(BUILD)/libmldsa.a: $(ALL_OBJS)
	$(AR) rcs $@ $^

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
