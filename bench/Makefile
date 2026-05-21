CC     ?= cc
CFLAGS := -O2 -Wall -Wextra
BUILD   = build

MLDSA_SOS   = $(addprefix schemes/mldsa/build/,    mldsa44.so mldsa65.so mldsa87.so)
SLHDSA_SOS  = $(addprefix schemes/slhdsa/build/,   \
    slhdsa_sha2_128s.so  slhdsa_shake_128s.so       \
    slhdsa_sha2_128f.so  slhdsa_shake_128f.so       \
    slhdsa_sha2_192s.so  slhdsa_shake_192s.so       \
    slhdsa_sha2_192f.so  slhdsa_shake_192f.so       \
    slhdsa_sha2_256s.so  slhdsa_shake_256s.so       \
    slhdsa_sha2_256f.so  slhdsa_shake_256f.so)
FNDSA_SOS   = $(addprefix schemes/fndsa/build/,    fndsa512.so fndsa1024.so)
CLASSIC_SOS = $(addprefix schemes/classic/build/,  \
    rsa2048.so rsa3072.so rsa4096.so                \
    ecdsa_p256.so ecdsa_p384.so ecdsa_p521.so       \
    ed25519.so ed448.so)
HAWK_SOS    = $(addprefix schemes/hawk/build/,     hawk512.so hawk1024.so)
MAYO_SOS    = $(addprefix schemes/mayo/build/,     mayo1.so mayo2.so mayo3.so mayo5.so)
FAEST_SOS   = $(addprefix schemes/faest/build/, \
    faest_128s.so faest_128f.so faest_em_128s.so faest_em_128f.so \
    faest_192s.so faest_192f.so faest_em_192s.so faest_em_192f.so \
    faest_256s.so faest_256f.so faest_em_256s.so faest_em_256f.so)
SDITH_SOS   = $(addprefix schemes/sdith/build/, \
    sdith_hypercube_cat1_gf256.so sdith_hypercube_cat1_p251.so \
    sdith_hypercube_cat3_gf256.so sdith_hypercube_cat3_p251.so \
    sdith_hypercube_cat5_gf256.so sdith_hypercube_cat5_p251.so \
    sdith_threshold_cat1_gf256.so sdith_threshold_cat1_p251.so \
    sdith_threshold_cat3_gf256.so sdith_threshold_cat3_p251.so \
    sdith_threshold_cat5_gf256.so sdith_threshold_cat5_p251.so)

ALL_SOS     = $(MLDSA_SOS) $(SLHDSA_SOS) $(FNDSA_SOS) $(CLASSIC_SOS) $(HAWK_SOS) $(MAYO_SOS) $(FAEST_SOS) $(SDITH_SOS)
SCHEME_DIRS = mldsa slhdsa fndsa classic hawk mayo faest sdith
STAMPS      = $(addprefix schemes/, $(addsuffix /.stamp, $(SCHEME_DIRS)))

.PHONY: all clean

all: bench

# Per-scheme source prerequisites that invalidate the stamp.
schemes/mldsa/.stamp:   schemes/mldsa/Makefile   schemes/mldsa/shim_template.c   schemes/mldsa/params.tsv
schemes/slhdsa/.stamp:  schemes/slhdsa/Makefile  schemes/slhdsa/shim_template.c  schemes/slhdsa/params.tsv
schemes/fndsa/.stamp:   schemes/fndsa/Makefile   schemes/fndsa/shim_template.c   schemes/fndsa/params.tsv
schemes/classic/.stamp: schemes/classic/Makefile $(wildcard schemes/classic/*_template.c schemes/classic/*.tsv)
schemes/hawk/.stamp:    schemes/hawk/Makefile    schemes/hawk/shim_template.c    schemes/hawk/params.tsv
schemes/mayo/.stamp:    schemes/mayo/Makefile    schemes/mayo/shim_template.c    schemes/mayo/params.tsv
schemes/faest/.stamp:   schemes/faest/Makefile   schemes/faest/shim_template.c   schemes/faest/params.tsv schemes/faest/gen_faest.py
schemes/sdith/.stamp:   schemes/sdith/Makefile \
    schemes/sdith/shim_hypercube_template.c schemes/sdith/params_hypercube.tsv \
    schemes/sdith/shim_threshold_template.c schemes/sdith/params_threshold.tsv

# Shared recipe for every scheme stamp.
$(STAMPS): schemes/%/.stamp:
	$(MAKE) -C $(@D) CC=$(CC)
	touch $@

# Each scheme's .so files depend on its stamp.
$(MLDSA_SOS):   schemes/mldsa/.stamp
$(SLHDSA_SOS):  schemes/slhdsa/.stamp
$(FNDSA_SOS):   schemes/fndsa/.stamp
$(CLASSIC_SOS): schemes/classic/.stamp
$(HAWK_SOS):    schemes/hawk/.stamp
$(MAYO_SOS):    schemes/mayo/.stamp
$(FAEST_SOS):   schemes/faest/.stamp
$(SDITH_SOS):   schemes/sdith/.stamp

# Generate the SO path list included by main.c.
$(BUILD)/so_paths.h: Makefile | $(BUILD)
	@printf '"%s",\n' $(ALL_SOS) > $@

$(BUILD)/main.o: main.c harness.h loader.h scheme.h $(BUILD)/so_paths.h | $(BUILD)
	$(CC) $(CFLAGS) -I$(BUILD) -c $< -o $@

$(BUILD)/loader.o: loader.c loader.h scheme.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

bench: $(BUILD)/main.o $(BUILD)/loader.o $(ALL_SOS) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(BUILD)/main.o $(BUILD)/loader.o -ldl

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	rm -f $(STAMPS)
	for d in $(SCHEME_DIRS); do $(MAKE) -C schemes/$$d clean; done
