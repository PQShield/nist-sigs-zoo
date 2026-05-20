CC      ?= cc
CFLAGS  := -O3 -fomit-frame-pointer -fPIC -Wall -Wextra -DFNDSA_SHAKE256X4=1
REF_DIR  = ref
BUILD    = build

.PHONY: all clean check-submodule

all: check-submodule $(BUILD)/fndsa512.so $(BUILD)/fndsa1024.so

check-submodule:
	@test -f $(REF_DIR)/fndsa.h || { \
	    echo "ERROR: bench/schemes/fndsa/ref/ is empty."; \
	    echo "Run: git submodule update --init bench/schemes/fndsa/ref"; \
	    exit 1; }

# Library: all implementation files, excluding tests and speed binary.
REF_CSRC = codec.c mq.c sha3.c sysrng.c util.c \
           kgen.c kgen_fxp.c kgen_gauss.c kgen_mp31.c kgen_ntru.c \
           kgen_poly.c kgen_zint31.c \
           sign.c sign_core.c sign_fpoly.c sign_fpr.c sign_sampler.c \
           vrfy.c
REF_OBJS = $(addprefix $(BUILD)/ref/, $(REF_CSRC:.c=.o))

$(BUILD)/ref:
	mkdir -p $@

$(BUILD)/ref/%.o: $(REF_DIR)/%.c | $(BUILD)/ref
	$(CC) $(CFLAGS) -I$(REF_DIR) -c $< -o $@

$(BUILD)/libfndsa.a: $(REF_OBJS) | $(BUILD)
	ar rcs $@ $^

$(BUILD):
	mkdir -p $@

$(BUILD)/fndsa512.o: fndsa512_shim.c ../../scheme.h | $(BUILD)
	$(CC) $(CFLAGS) -I$(REF_DIR) -c $< -o $@

$(BUILD)/fndsa1024.o: fndsa1024_shim.c ../../scheme.h | $(BUILD)
	$(CC) $(CFLAGS) -I$(REF_DIR) -c $< -o $@

$(BUILD)/fndsa512.so: $(BUILD)/fndsa512.o $(BUILD)/libfndsa.a
	$(CC) -shared -o $@ $^

$(BUILD)/fndsa1024.so: $(BUILD)/fndsa1024.o $(BUILD)/libfndsa.a
	$(CC) -shared -o $@ $^

clean:
	rm -rf $(BUILD)
