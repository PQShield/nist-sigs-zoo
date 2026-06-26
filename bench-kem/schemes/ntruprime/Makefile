CC     ?= cc
CFLAGS := -O2 -fPIC -Wall -Wextra
BUILD   = build
LOCAL   = $(BUILD)/local

SETS = sntrup653 sntrup761 sntrup857 sntrup953 sntrup1013 sntrup1277
SHIM_FILES := $(addsuffix _shim.c, $(SETS))
SOS        := $(addprefix $(BUILD)/, $(addsuffix .so, $(SETS)))

NP_A = $(LOCAL)/lib/libntruprime.a
RB_A = $(LOCAL)/lib/librandombytes-kernel.a

.PHONY: all clean
all: $(SOS)

# Fetch + build libntruprime (+ randombytes) into build/local once.
$(LOCAL)/.done:
	./build_libs.sh
$(NP_A) $(RB_A): $(LOCAL)/.done

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

$(BUILD)/%_shim.o: %_shim.c ../../scheme.h | $(LOCAL)/.done
	$(CC) $(CFLAGS) -I$(LOCAL)/include -c $< -o $@

# Static-link libntruprime + its RNG into each self-contained parameter-set .so.
$(BUILD)/%.so: $(BUILD)/%_shim.o $(NP_A) $(RB_A)
	$(CC) -shared -o $@ $< $(NP_A) $(RB_A)

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
