CC     ?= cc
CFLAGS := -O2 -Wall -Wextra -std=c11
BUILD  := build
AVX2_DIR := ref/Optimized_Implementation/avx2

LIB_SRCS := hawk_kgen.c hawk_sign.c hawk_vrfy.c \
             ng_fxp.c ng_hawk.c ng_mp31.c ng_ntru.c ng_poly.c ng_zint31.c \
             sha3.c
LIB_OBJS := $(patsubst %.c, $(BUILD)/%.o, $(LIB_SRCS))
LIB      := $(BUILD)/libhawk.a

SOS := $(BUILD)/hawk512.so $(BUILD)/hawk1024.so

.PHONY: all clean

all: check_ref $(SOS)

check_ref:
	@if [ ! -f ref/build.py ]; then \
	    echo "ERROR: bench/schemes/hawk/ref/ not populated." >&2; \
	    echo "Run: git submodule update --init bench/schemes/hawk/ref" >&2; \
	    exit 1; \
	fi
	@if [ ! -f $(AVX2_DIR)/hawk.h ]; then \
	    echo "Generating HAWK AVX2 sources via build.py..." >&2; \
	    cd ref && python3 build.py; \
	fi

$(BUILD)/%.o: $(AVX2_DIR)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -fPIC -I$(AVX2_DIR) -c $< -o $@

$(LIB): $(LIB_OBJS)
	ar rcs $@ $^

$(BUILD)/hawk512.so: hawk512_shim.c $(LIB) | $(BUILD)
	$(CC) $(CFLAGS) -fPIC -I$(AVX2_DIR) -I../.. -shared -o $@ $< $(LIB)

$(BUILD)/hawk1024.so: hawk1024_shim.c $(LIB) | $(BUILD)
	$(CC) $(CFLAGS) -fPIC -I$(AVX2_DIR) -I../.. -shared -o $@ $< $(LIB)

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
