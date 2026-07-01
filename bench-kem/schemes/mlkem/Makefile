CC      ?= cc
CFLAGS  := -O3 -fomit-frame-pointer -fPIC -mavx2 -mbmi2 -mpopcnt -maes \
            -march=native -mtune=native -Wall -Wextra
REF      = ref/mlkem
BUILD    = build

# mlkem-native monolithic build: one C unit + one asm unit per security level.
# - Native (AVX2) backends are off in the default config; enable them explicitly.
# - The default namespace prefix is PQCP_MLKEM_NATIVE_*; set it to "mlkem" so the
#   per-level symbols are mlkem<lvl>_* (matching the shim).
NATIVE = -DMLK_CONFIG_USE_NATIVE_BACKEND_ARITH -DMLK_CONFIG_USE_NATIVE_BACKEND_FIPS202 \
         -DMLK_CONFIG_NAMESPACE_PREFIX=mlkem
INC    = -I$(REF)

SHIM_FILES := mlkem512_shim.c mlkem768_shim.c mlkem1024_shim.c

.PHONY: all clean
all: $(SHIM_FILES) $(BUILD)/mlkem512.so $(BUILD)/mlkem768.so $(BUILD)/mlkem1024.so

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

# Per-level (= parameter set = namespace number) compilation.
define lvl_rules
$$(BUILD)/mlkem$(1):
	mkdir -p $$@

$$(BUILD)/mlkem$(1)/mlkem_native.o: $(REF)/mlkem_native.c | $$(BUILD)/mlkem$(1)
	$$(CC) $$(CFLAGS) $$(NATIVE) -DMLK_CONFIG_PARAMETER_SET=$(1) $$(INC) -c $$< -o $$@

$$(BUILD)/mlkem$(1)/mlkem_native_asm.o: $(REF)/mlkem_native_asm.S | $$(BUILD)/mlkem$(1)
	$$(CC) $$(CFLAGS) $$(NATIVE) -DMLK_CONFIG_PARAMETER_SET=$(1) $$(INC) -c $$< -o $$@

$$(BUILD)/mlkem$(1)/randombytes.o: randombytes.c | $$(BUILD)/mlkem$(1)
	$$(CC) $$(CFLAGS) -c $$< -o $$@

$$(BUILD)/mlkem$(1)_shim.o: mlkem$(1)_shim.c ../../scheme.h | $$(BUILD)
	$$(CC) $$(CFLAGS) -c $$< -o $$@

$$(BUILD)/mlkem$(1).so: $$(BUILD)/mlkem$(1)_shim.o $$(BUILD)/mlkem$(1)/mlkem_native.o \
                        $$(BUILD)/mlkem$(1)/mlkem_native_asm.o $$(BUILD)/mlkem$(1)/randombytes.o
	$$(CC) -shared -o $$@ $$^
endef

$(eval $(call lvl_rules,512))
$(eval $(call lvl_rules,768))
$(eval $(call lvl_rules,1024))

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
