CC      ?= cc
BUILD    = build
REF      = ref

# pq-code-package/mlkem-native, AVX2 (x86_64) backend.
# Flags mirror the upstream test/mk/auto.mk x86_64 path: -DMLK_FORCE_X86_64
# plus -mavx2 -mbmi2. We explicitly enable the native arithmetic and FIPS202
# backends so the AVX2 assembly is used (the C backend is the default otherwise).
# x86-64 only — the AVX2 .S sources require it. For another host, drop the
# native-backend defines and -mavx2/-mbmi2 to fall back to the portable C code.
MLK_DEFS := -DMLK_FORCE_X86_64 \
            -DMLK_CONFIG_USE_NATIVE_BACKEND_ARITH \
            -DMLK_CONFIG_USE_NATIVE_BACKEND_FIPS202 \
            -DMLK_CONFIG_NAMESPACE_PREFIX=mlkem
CFLAGS   := -O3 -fomit-frame-pointer -fPIC -std=c99 -Wall -Wextra \
            -mavx2 -mbmi2 $(MLK_DEFS) -I $(REF)/mlkem

# All mlkem-native C sources (C backend + x86_64 native backend + fips202 +
# native fips202). Backend .c/.S files self-guard on MLK_FORCE_X86_64, so
# globbing everything is safe — unselected backends compile to empty objects.
SRC_C := $(wildcard \
    $(REF)/mlkem/src/*.c \
    $(REF)/mlkem/src/fips202/*.c \
    $(REF)/mlkem/src/fips202/native/x86_64/src/*.c \
    $(REF)/mlkem/src/native/x86_64/src/*.c)
SRC_S := $(wildcard \
    $(REF)/mlkem/src/fips202/native/x86_64/src/*.S \
    $(REF)/mlkem/src/native/x86_64/src/*.S)

SHIM_FILES := mlkem512_shim.c mlkem768_shim.c mlkem1024_shim.c

.PHONY: all clean
all: $(SHIM_FILES) $(BUILD)/mlkem512.so $(BUILD)/mlkem768.so $(BUILD)/mlkem1024.so

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

# Per-level build. $(1)=stem (mlkem512…), $(2)=parameter set (512/768/1024).
# -DMLK_CONFIG_PARAMETER_SET selects the level; sources compiled per level into
# a private object dir so each .so is a self-contained single-level build.
define mlk_rules
$(1)_OBJ := \
    $$(patsubst $$(REF)/%.c,$$(BUILD)/$(1)/%.o,$$(SRC_C)) \
    $$(patsubst $$(REF)/%.S,$$(BUILD)/$(1)/%.o,$$(SRC_S))

$$(BUILD)/$(1)/%.o: $$(REF)/%.c
	@mkdir -p $$(@D)
	$$(CC) $$(CFLAGS) -DMLK_CONFIG_PARAMETER_SET=$(2) -c $$< -o $$@

$$(BUILD)/$(1)/%.o: $$(REF)/%.S
	@mkdir -p $$(@D)
	$$(CC) $$(CFLAGS) -DMLK_CONFIG_PARAMETER_SET=$(2) -c $$< -o $$@

$$(BUILD)/$(1).so: $(1)_shim.c $$($(1)_OBJ) ../../scheme.h | $$(BUILD)
	$$(CC) $$(CFLAGS) -DMLK_CONFIG_PARAMETER_SET=$(2) -shared -o $$@ $(1)_shim.c $$($(1)_OBJ)
endef

$(eval $(call mlk_rules,mlkem512,512))
$(eval $(call mlk_rules,mlkem768,768))
$(eval $(call mlk_rules,mlkem1024,1024))

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
