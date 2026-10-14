# Shared build for China NGCC round-1 candidates, which all implement the ICCS
# public-key C API. Included by bench/schemes/ngcc/Makefile (NGCC_KIND := sig)
# and bench-kem/schemes/ngcc/Makefile (NGCC_KIND := kem). See bench/CLAUDE.md.
#
# Best effort by design: an instance that fails to compile prints FAIL, leaves
# its log in $(BUILD)/<file>.log and is skipped; `make` still exits 0.
# $(BUILD)/built.list names the .so files that exist afterwards, and the suite
# Makefile appends it to SO_PATHS.
NGCC_ROOT   := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
.DEFAULT_GOAL := all

# Opt-in: this suite compiles and loads untrusted candidate code. It only runs
# when NGCC_BUILD=1, which the scripts/ngcc-run container image sets (and the AWS
# VM, Task 12). On a host it refuses, exits 0 and builds nothing.
# Build and `status` must run in the same ngcc-run invocation: each ngcc-run is a
# fresh container, so a separate `make status` would see no .so files.
ifneq ($(NGCC_BUILD),1)
.PHONY: all status clean
all status:
	@echo "NGCC: skipped outside the container (set NGCC_BUILD=1; use scripts/ngcc-run)"
clean:
	rm -rf $(BUILD)
	rm -f *_shim.c
else

NGCC_SRC    ?= $(NGCC_ROOT)/src
PARAMS      ?= params.tsv
BUILD       ?= build
STATUS      ?= status.tsv
KAT_HASHES  ?= $(NGCC_ROOT)/kat_hashes.tsv
# gnu17: ICCS targets GCC 8.5-era C; GCC 15 defaults to C23 (bool keywords, () prototypes).
# -fcommon: several submissions rely on tentative definitions shared between files.
# -Wno-error=...: GCC 14+ turned these into hard errors; the old code relies on them
# (pointer casts without a cast, undeclared calls that return int).
NGCC_CFLAGS ?= -O3 -march=native -mtune=native -fPIC -std=gnu17 -fcommon \
    -Wno-error=incompatible-pointer-types -Wno-error=implicit-function-declaration \
    -Wno-error=int-conversion
# C++ sources (.cpp/.cc/.cxx, see gen_rules.py) get the same optimisation, arch and -fPIC
# flags as C, with a C++ standard: gnu++23 covers the submitters' -std=c++20/c++23 trees.
# Compiled with $(CXX) (make's default g++); the .so links them with -lstdc++.
NGCC_CXXFLAGS ?= -O3 -march=native -mtune=native -fPIC -std=gnu++23
SUITE_PREFIX := schemes/ngcc

.PHONY: all sos built-list status clean

all:
	@mkdir -p $(BUILD)
	@if [ -f "$(NGCC_SRC)/.extracted" ]; then \
	    $(MAKE) --no-print-directory -k sos || echo "NGCC: some instances failed to build (FAIL lines above)"; \
	else \
	    echo "NGCC: no extracted sources in $(NGCC_SRC); skipping (bench-common/ngcc/fetch.py + extract.py)"; \
	fi
	@$(MAKE) --no-print-directory built-list

ifeq ($(filter clean,$(MAKECMDGOALS)),)
-include $(BUILD)/rules.mk
endif

$(BUILD)/rules.mk: $(PARAMS) shim_template.c $(NGCC_ROOT)/gen_rules.py $(wildcard $(NGCC_SRC)/.extracted)
	@mkdir -p $(BUILD)
	@python3 $(NGCC_ROOT)/gen_rules.py --params $(PARAMS) --src $(NGCC_SRC) --build $(BUILD) \
	    --suite-dir . --template shim_template.c --out $@

sos: $(NGCC_SOS)

built-list:
	@: > $(BUILD)/built.list
	@for so in $(NGCC_SOS); do \
	    if [ -f $$so ]; then case $$so in /*) echo $$so ;; *) echo "$(SUITE_PREFIX)/$$so" ;; esac; fi; \
	done >> $(BUILD)/built.list
	@echo "NGCC: $$(wc -l < $(BUILD)/built.list) of $(words $(NGCC_SOS)) instances built"

status: built-list
	python3 $(NGCC_ROOT)/status.py --kind $(NGCC_KIND) --params $(PARAMS) --src $(NGCC_SRC) \
	    --build $(BUILD) --suite-dir . --hashes $(KAT_HASHES) --out $(STATUS)

clean:
	rm -rf $(BUILD)
	rm -f *_shim.c
endif
