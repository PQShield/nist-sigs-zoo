CC     ?= cc
CFLAGS := -O3 -fomit-frame-pointer -fPIC -Wall -Wextra
BUILD  := build

REF = ref

.PHONY: all clean check-submodule

all: check-submodule
	@echo "TODO: add slhdsa-c shims and build rules once submodule is populated"
	@echo "      See CLAUDE.md for instructions."

check-submodule:
	@test -f $(REF)/README.md || { \
	    echo "ERROR: bench/schemes/slhdsa/ref/ is empty."; \
	    echo "Run: git submodule update --init bench/schemes/slhdsa/ref"; \
	    exit 1; }

clean:
	rm -rf $(BUILD)
