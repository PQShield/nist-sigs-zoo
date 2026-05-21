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
SNOVA_SOS   = $(addprefix schemes/snova/build/, \
    snova_24_5_16_4.so   snova_28_5_19_4.so   snova_48_17_16_2.so  snova_48_16_19_2.so  \
    snova_28_4_16_4x5.so snova_28_4_19_4x5.so                                            \
    snova_40_7_19_4.so   snova_37_8_16_4.so   snova_72_25_16_2.so                        \
    snova_38_5_16_4x5.so snova_38_5_19_4x5.so                                            \
    snova_50_9_19_4.so   snova_60_10_16_4.so  snova_97_33_16_2.so)
UOV_SOS     = $(addprefix schemes/uov/build/, \
    uov_Ip_pkc.so uov_Ip_classic.so \
    uov_Is_pkc.so uov_Is_classic.so \
    uov_III_pkc.so uov_III_classic.so \
    uov_V_pkc.so uov_V_classic.so)
QRUOV_SOS   = $(addprefix schemes/qruov/build/, \
    qruov1q127L3v156m54.so  qruov1q31L3v165m60.so   \
    qruov1q31L10v600m70.so  qruov1q7L10v740m100.so  \
    qruov3q127L3v228m78.so  qruov3q31L3v246m87.so   \
    qruov3q31L10v890m100.so qruov3q7L10v1100m140.so \
    qruov5q127L3v306m105.so qruov5q31L3v324m114.so  \
    qruov5q31L10v1120m120.so qruov5q7L10v1490m190.so)
MQOM_SOS    = $(addprefix schemes/mqom/build/, \
    mqom_cat1_gf2_fast_r3.so   mqom_cat1_gf2_fast_r5.so   \
    mqom_cat1_gf2_short_r3.so  mqom_cat1_gf2_short_r5.so  \
    mqom_cat1_gf16_fast_r3.so  mqom_cat1_gf16_fast_r5.so  \
    mqom_cat1_gf16_short_r3.so mqom_cat1_gf16_short_r5.so \
    mqom_cat1_gf256_fast_r3.so mqom_cat1_gf256_fast_r5.so \
    mqom_cat1_gf256_short_r3.so mqom_cat1_gf256_short_r5.so \
    mqom_cat3_gf2_fast_r3.so   mqom_cat3_gf2_fast_r5.so   \
    mqom_cat3_gf2_short_r3.so  mqom_cat3_gf2_short_r5.so  \
    mqom_cat3_gf16_fast_r3.so  mqom_cat3_gf16_fast_r5.so  \
    mqom_cat3_gf16_short_r3.so mqom_cat3_gf16_short_r5.so \
    mqom_cat3_gf256_fast_r3.so mqom_cat3_gf256_fast_r5.so \
    mqom_cat3_gf256_short_r3.so mqom_cat3_gf256_short_r5.so \
    mqom_cat5_gf2_fast_r3.so   mqom_cat5_gf2_fast_r5.so   \
    mqom_cat5_gf2_short_r3.so  mqom_cat5_gf2_short_r5.so  \
    mqom_cat5_gf16_fast_r3.so  mqom_cat5_gf16_fast_r5.so  \
    mqom_cat5_gf16_short_r3.so mqom_cat5_gf16_short_r5.so \
    mqom_cat5_gf256_fast_r3.so mqom_cat5_gf256_fast_r5.so \
    mqom_cat5_gf256_short_r3.so mqom_cat5_gf256_short_r5.so)

ALL_SOS     = $(MLDSA_SOS) $(SLHDSA_SOS) $(FNDSA_SOS) $(CLASSIC_SOS) $(HAWK_SOS) $(MAYO_SOS) $(FAEST_SOS) $(SDITH_SOS) $(SNOVA_SOS) $(UOV_SOS) $(QRUOV_SOS) $(MQOM_SOS)
SCHEME_DIRS = mldsa slhdsa fndsa classic hawk mayo faest sdith snova uov qruov mqom
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
schemes/snova/.stamp:   schemes/snova/Makefile schemes/snova/shim_template.c schemes/snova/params.tsv
schemes/uov/.stamp:     schemes/uov/Makefile   schemes/uov/shim_template.c   schemes/uov/params.tsv
schemes/qruov/.stamp:   schemes/qruov/Makefile schemes/qruov/shim_template.c schemes/qruov/params.tsv
schemes/mqom/.stamp:    schemes/mqom/Makefile  schemes/mqom/shim_template.c  schemes/mqom/params.tsv

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
$(SNOVA_SOS):   schemes/snova/.stamp
$(UOV_SOS):     schemes/uov/.stamp
$(QRUOV_SOS):   schemes/qruov/.stamp
$(MQOM_SOS):    schemes/mqom/.stamp

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
