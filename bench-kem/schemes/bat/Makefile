CC     ?= cc
BUILD   = build
BAT     = ref/src
CFLAGS := -O3 -fPIC -Wall -Wextra -march=native -mtune=native -DBAT_AVX2 -I$(BAT)

# Generic BAT objects, built once and shared by both .so. The generic keygen/codec
# dispatch into every q-variant, so all three kem<q>.o must be present in each .so
# (only the api_<q>_<n>.o that defines the bat_<q>_<n>_* entry points is per-set).
GEN_OBJS = $(addprefix $(BUILD)/common/, codec.o fft.o fnr.o keygen.o modqp.o prng.o blake2b.o blake2s.o kem128.o kem257.o kem769.o)

SOS = bat257512 bat7691024
SHIM_FILES := $(addsuffix _shim.c, $(SOS))

.PHONY: all clean
all: $(SHIM_FILES) $(addprefix $(BUILD)/, $(addsuffix .so, $(SOS)))

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(BUILD) $(BUILD)/common:
	mkdir -p $@

$(BUILD)/common/%.o: $(BAT)/%.c | $(BUILD)/common
	$(CC) $(CFLAGS) -c $< -o $@

# combo: $(1)=so basename  $(2)=api source (api_257_512)
define combo
$$(BUILD)/$(1)_api.o: $(BAT)/$(2).c | $$(BUILD)
	$$(CC) $$(CFLAGS) -c $$< -o $$@
$$(BUILD)/$(1)_shim.o: $(1)_shim.c ../../scheme.h | $$(BUILD)
	$$(CC) $$(CFLAGS) -c $$< -o $$@
$$(BUILD)/$(1).so: $$(BUILD)/$(1)_shim.o $$(BUILD)/$(1)_api.o $$(GEN_OBJS)
	$$(CC) -shared -o $$@ $$^
endef

$(eval $(call combo,bat257512,api_257_512))
$(eval $(call combo,bat7691024,api_769_1024))

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
