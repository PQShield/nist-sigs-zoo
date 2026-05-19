CC     ?= cc
AR     ?= ar
CFLAGS := -O3 -fomit-frame-pointer -Wall -Wextra
BUILD  := build

# Path to a PQClean checkout. Override: make PQCLEAN=/path/to/PQClean
PQCLEAN ?= ../../../../PQClean

COMMON     = $(PQCLEAN)/common
CFLAGS_ALL = $(CFLAGS) -I$(COMMON)

SHA2_SRCS  = address.c context_sha2.c fors.c hash_sha2.c merkle.c sign.c \
             thash_sha2_simple.c utils.c utilsx1.c wots.c wotsx1.c
SHAKE_SRCS = address.c context_shake.c fors.c hash_shake.c merkle.c sign.c \
             thash_shake_simple.c utils.c utilsx1.c wots.c wotsx1.c

SHA2_VARIANTS  = sphincs-sha2-128s-simple sphincs-sha2-128f-simple \
                 sphincs-sha2-192s-simple sphincs-sha2-192f-simple \
                 sphincs-sha2-256s-simple sphincs-sha2-256f-simple
SHAKE_VARIANTS = sphincs-shake-128s-simple sphincs-shake-128f-simple \
                 sphincs-shake-192s-simple sphincs-shake-192f-simple \
                 sphincs-shake-256s-simple sphincs-shake-256f-simple

# Must be first target.
.PHONY: all clean
all: $(BUILD)/libslhdsa.a

# Pattern rule for each variant.
define variant_rules
$$(BUILD)/$(1)/%.o: $$(PQCLEAN)/crypto_sign/$(1)/clean/%.c | $$(BUILD)/$(1)
	$$(CC) $$(CFLAGS_ALL) -I$$(PQCLEAN)/crypto_sign/$(1)/clean -c $$< -o $$@

$$(BUILD)/$(1):
	mkdir -p $$@
endef

$(foreach v,$(SHA2_VARIANTS) $(SHAKE_VARIANTS),$(eval $(call variant_rules,$(v))))

SHA2_OBJS  = $(foreach v,$(SHA2_VARIANTS), \
               $(addprefix $(BUILD)/$(v)/, $(SHA2_SRCS:.c=.o)))
SHAKE_OBJS = $(foreach v,$(SHAKE_VARIANTS), \
               $(addprefix $(BUILD)/$(v)/, $(SHAKE_SRCS:.c=.o)))

# PQClean common sources (compiled once, no variant-specific flags needed).
$(BUILD)/common/sha2.o: $(COMMON)/sha2.c | $(BUILD)/common
	$(CC) $(CFLAGS_ALL) -c $< -o $@

$(BUILD)/common/fips202.o: $(COMMON)/fips202.c | $(BUILD)/common
	$(CC) $(CFLAGS_ALL) -c $< -o $@

$(BUILD)/common/randombytes.o: $(COMMON)/randombytes.c | $(BUILD)/common
	$(CC) $(CFLAGS_ALL) -c $< -o $@

$(BUILD)/common:
	mkdir -p $@

COMMON_OBJS = $(BUILD)/common/sha2.o $(BUILD)/common/fips202.o \
              $(BUILD)/common/randombytes.o

$(BUILD)/slhdsa.o: slhdsa.c slhdsa.h ../../scheme.h | $(BUILD)
	$(CC) $(CFLAGS_ALL) -c $< -o $@

$(BUILD)/libslhdsa.a: $(BUILD)/slhdsa.o $(SHA2_OBJS) $(SHAKE_OBJS) $(COMMON_OBJS)
	$(AR) rcs $@ $^

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
