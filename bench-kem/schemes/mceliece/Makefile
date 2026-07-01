CC     ?= cc
CFLAGS := -O2 -fPIC -Wall -Wextra
BUILD   = build
LOCAL   = $(BUILD)/local

SETS = mceliece348864 mceliece460896 mceliece6688128 mceliece6960119 mceliece8192128
SHIM_FILES := $(addsuffix _shim.c, $(SETS))
SOS        := $(addprefix $(BUILD)/, $(addsuffix .so, $(SETS)))

MC_A = $(LOCAL)/lib/libmceliece.a
RB_A = $(LOCAL)/lib/librandombytes-kernel.a

.PHONY: all clean
all: $(SOS)

# Fetch + build libmceliece (+ randombytes/cpucycles) into build/local once.
$(LOCAL)/.done:
	./build_libs.sh
$(MC_A) $(RB_A): $(LOCAL)/.done

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(BUILD)/%_shim.o: %_shim.c ../../scheme.h | $(LOCAL)/.done
	$(CC) $(CFLAGS) -I$(LOCAL)/include -c $< -o $@

# Static-link libmceliece + its RNG into each self-contained parameter-set .so.
$(BUILD)/%.so: $(BUILD)/%_shim.o $(MC_A) $(RB_A)
	$(CC) -shared -o $@ $< $(MC_A) $(RB_A)

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
