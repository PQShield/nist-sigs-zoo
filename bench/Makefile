include ../bench-common/flags.mk

CC     ?= cc
# The harness is built with the schemes' flag set too (no -fPIC: it is the executable).
CFLAGS := $(BENCH_CFLAGS) -Wall -Wextra
BUILD   = build

MLDSA_SOS   = $(addprefix schemes/mldsa/build/,    mldsa44.so mldsa65.so mldsa87.so)
SLHDSA_SOS  = $(addprefix schemes/slhdsa/build/,   \
    slhdsa_sha2_128s.so  slhdsa_shake_128s.so       \
    slhdsa_sha2_128f.so  slhdsa_shake_128f.so       \
    slhdsa_sha2_192s.so  slhdsa_shake_192s.so       \
    slhdsa_sha2_192f.so  slhdsa_shake_192f.so       \
    slhdsa_sha2_256s.so  slhdsa_shake_256s.so       \
    slhdsa_sha2_256f.so  slhdsa_shake_256f.so       \
    slhdsa_shake_128_24.so slhdsa_shake_192_24.so    \
    slhdsa_shake_256_24.so)
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
    snova_i_k.so   snova_i_b.so   snova_i_s.so   \
    snova_iii_k.so snova_iii_b.so snova_iii_s.so \
    snova_v_k.so   snova_v_b.so   snova_v_s.so   \
    snova_27_3_16_4_7_5_84.so     snova_27_4_16_4_5_5_80.so   \
    snova_27_4_19_4_6_5_96.so     snova_27_5_13_4_4_5_80.so   \
    snova_50_17_16_2_2_17_68.so   snova_27_4_16_5_5_4_100.so  \
    snova_33_5_16_4_5_6_100.so    snova_39_5_11_4_6_7_120.so  \
    snova_38_5_19_4_6_7_120.so    snova_76_25_16_2_2_25_100.so \
    snova_40_5_19_5_6_6_150.so    snova_47_6_19_4_6_9_144.so  \
    snova_104_33_16_2_2_33_132.so)
UOV_SOS     = $(addprefix schemes/uov/build/, \
    uov_Ip_pkc.so uov_Ip_classic.so \
    uov_Is_pkc.so uov_Is_classic.so \
    uov_III_pkc.so uov_III_classic.so \
    uov_V_pkc.so uov_V_classic.so)
QRUOV_SOS   = $(addprefix schemes/qruov/build/, \
    qruov1q127L3v156m54.so   qruov1q127L10v540m60.so  \
    qruov3q127L3v228m78.so   qruov3q127L10v820m90.so  \
    qruov5q127L3v306m105.so  qruov5q127L10v1040m110.so \
    qruov1q7L10v740m100.so \
    qruov1q31L3v165m60.so \
    qruov1q31L10v600m70.so \
    qruov3q7L10v1100m140.so \
    qruov3q31L3v246m87.so \
    qruov3q31L10v890m100.so \
    qruov5q7L10v1490m190.so \
    qruov5q31L3v324m114.so \
    qruov5q31L10v1120m120.so)
SQISIGN_SOS = $(addprefix schemes/sqisign/build/, \
    sqisign_lvl1.so sqisign_lvl3.so sqisign_lvl5.so)
SDITH2_SOS  = $(addprefix schemes/sdith2/build/, \
    sdith2_cat1_short.so sdith2_cat1_fast.so \
    sdith2_cat3_short.so sdith2_cat3_fast.so \
    sdith2_cat5_short.so sdith2_cat5_fast.so \
    sdith2_cat1_short_cipherpow.so sdith2_cat1_fast_cipherpow.so \
    sdith2_cat3_short_cipherpow.so sdith2_cat3_fast_cipherpow.so \
    sdith2_cat5_short_cipherpow.so sdith2_cat5_fast_cipherpow.so)
AIMER_SOS   = $(addprefix schemes/aimer/build/, \
    aimer128f.so aimer128s.so aimer192f.so aimer192s.so aimer256f.so aimer256s.so)
HAETAE_SOS  = $(addprefix schemes/haetae/build/, haetae2.so haetae3.so haetae5.so)
MIRATH_SOS  = $(addprefix schemes/mirath/build/, \
    mirath_1a_short.so mirath_1a_fast.so mirath_1b_short.so mirath_1b_fast.so \
    mirath_3a_short.so mirath_3a_fast.so mirath_3b_short.so mirath_3b_fast.so \
    mirath_5a_short.so mirath_5a_fast.so mirath_5b_short.so mirath_5b_fast.so)
RYDE_SOS    = $(addprefix schemes/ryde/build/, \
    ryde1s.so ryde1f.so ryde1ff.so ryde3s.so ryde3f.so ryde3ff.so ryde5s.so ryde5f.so ryde5ff.so)
PERK_SOS    = $(addprefix schemes/perk/build/, perk1s.so perk1f.so perk3s.so perk3f.so perk5s.so perk5f.so)
CROSS_SOS   = $(addprefix schemes/cross/build/, \
    cross_rsdp_1_fast.so cross_rsdp_1_balanced.so cross_rsdp_1_small.so \
    cross_rsdpg_1_fast.so cross_rsdpg_1_balanced.so cross_rsdpg_1_small.so \
    cross_rsdp_3_fast.so cross_rsdp_3_balanced.so cross_rsdp_3_small.so \
    cross_rsdpg_3_fast.so cross_rsdpg_3_balanced.so cross_rsdpg_3_small.so \
    cross_rsdp_5_fast.so cross_rsdp_5_balanced.so cross_rsdp_5_small.so \
    cross_rsdpg_5_fast.so cross_rsdpg_5_balanced.so cross_rsdpg_5_small.so)
LESS_SOS    = $(addprefix schemes/less/build/, \
    less_252_192.so less_252_68.so less_252_45.so less_400_220.so less_400_102.so \
    less_548_345.so less_548_137.so)
MQOM_SOS    = $(addprefix schemes/mqom/build/, \
    mqom3_cat1_gf2_shorter_ct.so mqom3_cat1_gf2_shorter_ot.so \
    mqom3_cat1_gf16_short_ct.so  mqom3_cat1_gf16_short_ot.so  \
    mqom3_cat1_gf16_fast_ct.so   mqom3_cat1_gf16_fast_ot.so   \
    mqom3_cat3_gf2_shorter_ct.so mqom3_cat3_gf2_shorter_ot.so \
    mqom3_cat3_gf16_short_ct.so  mqom3_cat3_gf16_short_ot.so  \
    mqom3_cat3_gf16_fast_ct.so   mqom3_cat3_gf16_fast_ot.so   \
    mqom3_cat5_gf2_shorter_ct.so mqom3_cat5_gf2_shorter_ot.so \
    mqom3_cat5_gf16_short_ct.so  mqom3_cat5_gf16_short_ot.so  \
    mqom3_cat5_gf16_fast_ct.so   mqom3_cat5_gf16_fast_ot.so)
MAYO_R1_SOS = $(addprefix schemes/mayo_r1/build/, \
    mayo_r1_1.so mayo_r1_2.so mayo_r1_3.so \
    mayo_r1_5.so)
MAYO_R2_SOS = $(addprefix schemes/mayo_r2/build/, \
    mayo_r2_1.so mayo_r2_2.so mayo_r2_3.so \
    mayo_r2_5.so)

ALL_SOS     = $(MLDSA_SOS) $(SLHDSA_SOS) $(FNDSA_SOS) $(CLASSIC_SOS) $(HAWK_SOS) $(MAYO_SOS) $(FAEST_SOS) $(SDITH_SOS) $(SDITH2_SOS) $(SNOVA_SOS) $(UOV_SOS) $(QRUOV_SOS) $(MQOM_SOS) $(SQISIGN_SOS) $(AIMER_SOS) $(HAETAE_SOS) $(MIRATH_SOS) $(RYDE_SOS) $(PERK_SOS) $(CROSS_SOS) $(LESS_SOS) $(MAYO_R1_SOS) $(MAYO_R2_SOS)
SCHEME_DIRS = mldsa slhdsa fndsa classic hawk mayo faest sdith sdith2 snova uov qruov mqom sqisign aimer haetae mirath ryde perk cross less mayo_r1 mayo_r2
STAMPS      = $(addprefix schemes/, $(addsuffix /.stamp, $(SCHEME_DIRS)))

# China NGCC round-1 candidates: one generic ICCS-API shim, data-driven from
# schemes/ngcc/params.tsv. Best effort: instances that fail to build are skipped,
# and only the .so files in schemes/ngcc/build/built.list are benchmarked.
NGCC_DIR    = schemes/ngcc
NGCC_STAMP  = $(NGCC_DIR)/.stamp

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
schemes/sdith2/.stamp:  schemes/sdith2/Makefile  schemes/sdith2/shim_template.c  schemes/sdith2/params.tsv
schemes/sqisign/.stamp: schemes/sqisign/Makefile schemes/sqisign/shim_template.c schemes/sqisign/params.tsv
schemes/aimer/.stamp:   schemes/aimer/Makefile   schemes/aimer/shim_template.c   schemes/aimer/params.tsv
schemes/haetae/.stamp:  schemes/haetae/Makefile  schemes/haetae/shim_template.c  schemes/haetae/params.tsv schemes/haetae/.source
schemes/mirath/.stamp:  schemes/mirath/Makefile  schemes/mirath/shim_template.c  schemes/mirath/params.tsv schemes/mirath/.source
schemes/ryde/.stamp:    schemes/ryde/Makefile    schemes/ryde/shim_template.c    schemes/ryde/params.tsv   schemes/ryde/.source
schemes/perk/.stamp:    schemes/perk/Makefile    schemes/perk/shim_template.c    schemes/perk/params.tsv   schemes/perk/.source
schemes/cross/.stamp:   schemes/cross/Makefile   schemes/cross/shim_template.c   schemes/cross/params.tsv
schemes/less/.stamp:    schemes/less/Makefile    schemes/less/shim_template.c    schemes/less/params.tsv
schemes/mayo_r1/.stamp: schemes/mayo_r1/Makefile schemes/mayo_r1/shim_template.c schemes/mayo_r1/params.tsv
schemes/mayo_r2/.stamp: schemes/mayo_r2/Makefile schemes/mayo_r2/shim_template.c schemes/mayo_r2/params.tsv
$(NGCC_STAMP): $(NGCC_DIR)/Makefile $(NGCC_DIR)/shim_template.c $(NGCC_DIR)/params.tsv \
    ../bench-common/ngcc/ngcc.mk ../bench-common/ngcc/gen_rules.py \
    $(wildcard $(NGCC_DIR)/patches/*/*) $(wildcard ../bench-common/ngcc/src/.extracted)
	@if [ "$(NGCC_BUILD)" = 1 ]; then $(MAKE) -C $(NGCC_DIR) CC=$(CC); \
	else echo "NGCC: skipped outside the container (set NGCC_BUILD=1; use scripts/ngcc-run)"; fi
	touch $@

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
$(SDITH2_SOS):  schemes/sdith2/.stamp
$(SNOVA_SOS):   schemes/snova/.stamp
$(UOV_SOS):     schemes/uov/.stamp
$(QRUOV_SOS):   schemes/qruov/.stamp
$(MQOM_SOS):    schemes/mqom/.stamp
$(SQISIGN_SOS): schemes/sqisign/.stamp
$(AIMER_SOS):   schemes/aimer/.stamp
$(HAETAE_SOS):  schemes/haetae/.stamp
$(MIRATH_SOS):  schemes/mirath/.stamp
$(RYDE_SOS):    schemes/ryde/.stamp
$(PERK_SOS):    schemes/perk/.stamp
$(CROSS_SOS):   schemes/cross/.stamp
$(LESS_SOS):    schemes/less/.stamp
$(MAYO_R1_SOS): schemes/mayo_r1/.stamp
$(MAYO_R2_SOS): schemes/mayo_r2/.stamp

# Generate the SO path list included by main.c.
$(BUILD)/so_paths.h: Makefile $(NGCC_STAMP) | $(BUILD)
	@printf '"%s",\n' $(ALL_SOS) > $@
ifeq ($(NGCC_BUILD),1)
	@if [ -f $(NGCC_DIR)/build/built.list ]; then sed 's/.*/"&",/' $(NGCC_DIR)/build/built.list >> $@; fi
endif

$(BUILD)/main.o: main.c harness.h ../bench-common/harness_common.h ../bench-common/isolate.h loader.h scheme.h $(BUILD)/so_paths.h | $(BUILD)
	$(CC) $(CFLAGS) -I$(BUILD) -c $< -o $@

$(BUILD)/loader.o: loader.c loader.h scheme.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

bench: $(BUILD)/main.o $(BUILD)/loader.o $(ALL_SOS) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(BUILD)/main.o $(BUILD)/loader.o -ldl -lpthread

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	rm -f $(STAMPS)
	for d in $(SCHEME_DIRS); do $(MAKE) -C schemes/$$d clean; done
	$(MAKE) -C $(NGCC_DIR) clean
	rm -f $(NGCC_STAMP)
