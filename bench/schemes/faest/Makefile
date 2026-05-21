CC    ?= cc
BUILD  = build
GEN    = $(BUILD)/gen
REF    = ref

CFLAGS    := -O3 -fomit-frame-pointer -fPIC -maes -mavx2 -mpopcnt -march=native -mtune=native \
             -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare
IFLAGS    := -DHAVE_CONFIG_H -I$(GEN) -I$(REF) -I$(REF)/sha3 -I$(REF)/sha3/avx2

SHARED_SRCS := aes.c bavc.c compat.c cpu.c faest_impl.c fields.c \
               instances.c owf.c random_oracle.c randomness.c \
               universal_hashing.c utils.c vole.c
SHARED_OBJS := $(addprefix $(BUILD)/, $(SHARED_SRCS:.c=.o))

GEN_PARAM_NAMES := faest_128s faest_128f faest_em_128s faest_em_128f \
                   faest_192s faest_192f faest_em_192s faest_em_192f \
                   faest_256s faest_256f faest_em_256s faest_em_256f
GEN_PARAM_SRCS := $(addprefix $(GEN)/, $(addsuffix .c, $(GEN_PARAM_NAMES)))
GEN_PARAM_OBJS := $(GEN_PARAM_SRCS:$(GEN)/%.c=$(BUILD)/gen_%.o)

GEN_AES_SRCS := $(GEN)/faest_aes_128.c $(GEN)/faest_aes_192.c $(GEN)/faest_aes_256.c
GEN_AES_OBJS := $(GEN_AES_SRCS:$(GEN)/%.c=$(BUILD)/gen_%.o)

SHIM_FILES := $(addsuffix _shim.c, $(GEN_PARAM_NAMES))
SOS        := $(addprefix $(BUILD)/, $(addsuffix .so, $(GEN_PARAM_NAMES)))
LIBFAEST   := $(BUILD)/libfaest.a

.PHONY: all clean

all: $(SHIM_FILES) $(SOS)

# Generate all headers/sources from upstream templates
$(GEN_PARAM_SRCS) $(GEN_AES_SRCS) $(GEN)/config.h $(GEN)/parameters.h: gen_faest.py | $(GEN)
	python3 gen_faest.py $(REF) $(GEN)

# Shim generation
$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

# Compile shared FAEST ref sources
$(BUILD)/%.o: $(REF)/%.c $(GEN)/config.h $(GEN)/parameters.h | $(BUILD)
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

# Compile generated param and AES sources
$(BUILD)/gen_%.o: $(GEN)/%.c $(GEN)/config.h $(GEN)/parameters.h | $(BUILD)
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

# Compile SHA3 (AVX2 assembly path)
$(BUILD)/sha3_KeccakHash.o: $(REF)/sha3/KeccakHash.c | $(BUILD)
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(BUILD)/sha3_KeccakSponge.o: $(REF)/sha3/KeccakSponge.c | $(BUILD)
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(BUILD)/sha3_avx2.o: $(REF)/sha3/avx2/KeccakP-1600-AVX2.s | $(BUILD)
	$(CC) -c $< -o $@

SHA3_OBJS := $(BUILD)/sha3_KeccakHash.o $(BUILD)/sha3_KeccakSponge.o $(BUILD)/sha3_avx2.o

# Static library from all object files
$(LIBFAEST): $(SHARED_OBJS) $(GEN_PARAM_OBJS) $(GEN_AES_OBJS) $(SHA3_OBJS)
	ar rcs $@ $^

# Shim .so: compile and link against libfaest in one step
$(BUILD)/%.so: %_shim.c $(LIBFAEST) | $(BUILD)
	$(CC) $(CFLAGS) $(IFLAGS) -shared $< $(LIBFAEST) -o $@

$(BUILD) $(GEN):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
