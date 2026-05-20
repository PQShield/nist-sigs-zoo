CC      ?= cc
CFLAGS  := -O3 -fomit-frame-pointer -fPIC -Wall -Wextra -std=c99
REF_DIR  = ref
BUILD    = build

PARAM_SETS = \
    slhdsa_sha2_128s  slhdsa_shake_128s \
    slhdsa_sha2_128f  slhdsa_shake_128f \
    slhdsa_sha2_192s  slhdsa_shake_192s \
    slhdsa_sha2_192f  slhdsa_shake_192f \
    slhdsa_sha2_256s  slhdsa_shake_256s \
    slhdsa_sha2_256f  slhdsa_shake_256f

TARGETS = $(addprefix $(BUILD)/, $(addsuffix .so, $(PARAM_SETS)))

.PHONY: all clean check-submodule

all: check-submodule $(TARGETS)

check-submodule:
	@test -f $(REF_DIR)/slh_dsa.c || { \
	    echo "ERROR: bench/schemes/slhdsa/ref/ is empty."; \
	    echo "Run: git submodule update --init bench/schemes/slhdsa/ref"; \
	    exit 1; }

# Build slhdsa-c sources once as a static library.
REF_CSRC = $(wildcard $(REF_DIR)/*.c)
REF_OBJS = $(patsubst $(REF_DIR)/%.c, $(BUILD)/ref/%.o, $(REF_CSRC))

$(BUILD)/ref:
	mkdir -p $@

$(BUILD)/ref/%.o: $(REF_DIR)/%.c | $(BUILD)/ref
	$(CC) $(CFLAGS) -I$(REF_DIR) -c $< -o $@

$(BUILD)/libslhdsa.a: $(REF_OBJS) | $(BUILD)
	ar rcs $@ $^

$(BUILD):
	mkdir -p $@

# Compile each shim.
$(BUILD)/%.o: %_shim.c ../../scheme.h | $(BUILD)
	$(CC) $(CFLAGS) -I$(REF_DIR) -c $< -o $@

# Link each shim .so against the static library.
$(BUILD)/%.so: $(BUILD)/%.o $(BUILD)/libslhdsa.a
	$(CC) -shared -o $@ $^

clean:
	rm -rf $(BUILD)
