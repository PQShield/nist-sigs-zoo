include ../../../bench-common/flags.mk
# FN-DSA, experimental fixed-point implementation ("type 1 fixed-point 32.64", ePrint 2026/1531):
# ref = pornin/c-fn-dsa-fx32.64 1e6ef5d (2026-10-11; code as first imported 2026-10-09).
# Same API and key/signature encodings as bench/schemes/fndsa (c-fn-dsa); seeded signatures
# differ on purpose, so there are no shared KATs. AVX2 (keygen, verify) is gated at runtime.
# Upstream has no FNDSA_SHAKE256X4 option, unlike c-fn-dsa. Standard mode only: the
# expanded-key, KRM and MRM entry points are not benchmarked.
CC      ?= cc
CFLAGS  := -Wall -Wextra $(BENCH_CFLAGS) -fPIC
REF_DIR  = ref
BUILD    = build

SHIM_FILES := fndsa_fx512_shim.c fndsa_fx1024_shim.c

.PHONY: all clean check-submodule

all: check-submodule $(SHIM_FILES) $(BUILD)/fndsa_fx512.so $(BUILD)/fndsa_fx1024.so

$(SHIM_FILES): shim_template.c params.tsv
	python3 ../../gen_shims.py shim_template.c params.tsv .

check-submodule:
	@test -f $(REF_DIR)/fndsa.h || { \
	    echo "ERROR: bench/schemes/fndsa_fx/ref/ is empty."; \
	    echo "Run: git submodule update --init bench/schemes/fndsa_fx/ref"; \
	    exit 1; }

# Library: upstream's OBJ list (Makefile), without test_fndsa and speed_fndsa.
REF_CSRC = codec.c codec_mrm.c mq.c sha3.c sysrng.c util.c util_mrm.c \
           kgen.c kgen_fxp.c kgen_gauss.c kgen_mp31.c kgen_ntru.c kgen_poly.c kgen_zint31.c \
           sign.c sign_core.c sign_ext.c sign_mq2.c sign_mrm.c sign_rx.c sign_sampler.c sign_xpoly.c \
           vrfy.c vrfy_mrm.c
REF_OBJS = $(addprefix $(BUILD)/ref/, $(REF_CSRC:.c=.o))

$(BUILD)/ref:
	mkdir -p $@

$(BUILD)/ref/%.o: $(REF_DIR)/%.c | $(BUILD)/ref
	$(CC) $(CFLAGS) -I$(REF_DIR) -c $< -o $@

$(BUILD)/libfndsa_fx.a: $(REF_OBJS) | $(BUILD)
	ar rcs $@ $^

$(BUILD):
	mkdir -p $@

$(BUILD)/%.o: %_shim.c ../../scheme.h | $(BUILD)
	$(CC) $(CFLAGS) -I$(REF_DIR) -c $< -o $@

$(BUILD)/%.so: $(BUILD)/%.o $(BUILD)/libfndsa_fx.a
	$(CC) -shared -o $@ $^

clean:
	rm -rf $(BUILD)
	rm -f $(SHIM_FILES)
