#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.10"
# dependencies = ["ruamel.yaml>=0.18"]
# ///
"""Parse a bench results file and update data/schemes/*.yaml with measured cycles.

Also adds the run's environment to data/benchmark_env.yaml (each parameter set with timings
names its environment in `bench_env`) and records each row's input fingerprint in
bench/results/fingerprints.tsv (bench-common/fingerprint.py), from which
scripts/aws-bench-split decides which rows need re-running.

Usage:
    uv run bench/update_scheme_data.py [results_file]
    uv run bench/update_scheme_data.py --ngcc-sizes bench/results/ngcc_sizes.tsv

If no results_file is given, uses the most recent file in bench/results/.
"""

import re
import sys
from pathlib import Path

from ruamel.yaml import YAML

REPO_ROOT = Path(__file__).parent.parent
SCHEMES_DIR = REPO_ROOT / "data" / "schemes"
BENCH_ENV_FILE = REPO_ROOT / "data" / "benchmark_env.yaml"

# Map bench output name → target(s) in data/schemes/:
#   (yaml_filename, parameterset_name)           the FIRST version that has this set
#                                               (the newest one: versions are newest first)
#   (yaml_filename, parameterset_name, version)  exactly that version; `version` is its
#                                               `version:` label, or "label@YYYY-MM-DD"
#                                               when the label alone is not unique
#   [target, ...]                               several versions built from identical code
# Only entries where the bench measurement corresponds to the YAML parameterset.
# Older NIST on-ramp rounds of the schemes still in the competition are benchmarked
# from their own round's code (bench/schemes/<x>_r1, <x>_r2): their rows are named
# <scheme>-R1-… / <scheme>-R2-… and target their version explicitly.
Target = tuple[str, str] | tuple[str, str, str]
BENCH_TO_YAML: dict[str, Target | list[Target]] = {
    # ML-DSA (FIPS 204)
    "ML-DSA-44": ("ML-DSA.yaml", "ML-DSA-44"),
    "ML-DSA-65": ("ML-DSA.yaml", "ML-DSA-65"),
    "ML-DSA-87": ("ML-DSA.yaml", "ML-DSA-87"),
    # SLH-DSA (FIPS 205) — only SHAKE variants present in YAML
    "SLH-DSA-SHAKE-128s": ("SLH-DSA.yaml", "SHAKE-128s"),
    "SLH-DSA-SHAKE-128f": ("SLH-DSA.yaml", "SHAKE-128f"),
    "SLH-DSA-SHAKE-192s": ("SLH-DSA.yaml", "SHAKE-192s"),
    "SLH-DSA-SHAKE-192f": ("SLH-DSA.yaml", "SHAKE-192f"),
    "SLH-DSA-SHAKE-256s": ("SLH-DSA.yaml", "SHAKE-256s"),
    "SLH-DSA-SHAKE-256f": ("SLH-DSA.yaml", "SHAKE-256f"),
    # SP 800-230 ipd limited-signature sets (2^24 signatures per key)
    "SLH-DSA-SHAKE-128-24": ("SLH-DSA.yaml", "SHAKE-128-24"),
    "SLH-DSA-SHAKE-192-24": ("SLH-DSA.yaml", "SHAKE-192-24"),
    "SLH-DSA-SHAKE-256-24": ("SLH-DSA.yaml", "SHAKE-256-24"),
    # FN-DSA / Falcon
    "FN-DSA-512": ("Falcon.yaml", "512"),
    "FN-DSA-1024": ("Falcon.yaml", "1024"),
    # HAWK
    "HAWK-512": ("HAWK.yaml", "512"),
    "HAWK-1024": ("HAWK.yaml", "1024"),
    # MAYO
    "MAYO-one": ("MAYO.yaml", "one"),
    "MAYO-two": ("MAYO.yaml", "two"),  # round 3: MAYO-two re-parameterised
    "MAYO-two (broken)": ("MAYO.yaml", "two"),  # round-2 results files
    "MAYO-three": ("MAYO.yaml", "three"),
    "MAYO-five": ("MAYO.yaml", "five"),
    # FAEST
    "FAEST-128s": ("FAEST.yaml", "128s"),
    "FAEST-128f": ("FAEST.yaml", "128f"),
    "FAEST-EM-128s": ("FAEST.yaml", "EM-128s"),
    "FAEST-EM-128f": ("FAEST.yaml", "EM-128f"),
    "FAEST-192s": ("FAEST.yaml", "192s"),
    "FAEST-192f": ("FAEST.yaml", "192f"),
    "FAEST-EM-192s": ("FAEST.yaml", "EM-192s"),
    "FAEST-EM-192f": ("FAEST.yaml", "EM-192f"),
    "FAEST-256s": ("FAEST.yaml", "256s"),
    "FAEST-256f": ("FAEST.yaml", "256f"),
    "FAEST-EM-256s": ("FAEST.yaml", "EM-256s"),
    "FAEST-EM-256f": ("FAEST.yaml", "EM-256f"),
    # SDitH — bench implements v1.0 hypercube/threshold parameters
    "SDitH-Hypercube-L1-GF256": ("SDitH.yaml", "gf256-L1-hyp"),
    "SDitH-Hypercube-L1-P251": ("SDitH.yaml", "gf251-L1-hyp"),
    "SDitH-Hypercube-L3-GF256": ("SDitH.yaml", "gf256-L3-hyp"),
    "SDitH-Hypercube-L3-P251": ("SDitH.yaml", "gf251-L3-hyp"),
    "SDitH-Hypercube-L5-GF256": ("SDitH.yaml", "gf256-L5-hyp"),
    "SDitH-Hypercube-L5-P251": ("SDitH.yaml", "gf251-L5-hyp"),
    "SDitH-Threshold-L1-GF256": ("SDitH.yaml", "gf256-L1-thr"),
    "SDitH-Threshold-L1-P251": ("SDitH.yaml", "gf251-L1-thr"),
    "SDitH-Threshold-L3-GF256": ("SDitH.yaml", "gf256-L3-thr"),
    "SDitH-Threshold-L3-P251": ("SDitH.yaml", "gf251-L3-thr"),
    "SDitH-Threshold-L5-GF256": ("SDitH.yaml", "gf256-L5-thr"),
    "SDitH-Threshold-L5-P251": ("SDitH.yaml", "gf251-L5-thr"),
    # SNOVA v3.0 (round 3) — bench output name matches YAML parameterset name
    "SNOVA_I_K": ("SNOVA.yaml", "SNOVA_I_K"),
    "SNOVA_I_B": ("SNOVA.yaml", "SNOVA_I_B"),
    "SNOVA_I_S": ("SNOVA.yaml", "SNOVA_I_S"),
    "SNOVA_III_K": ("SNOVA.yaml", "SNOVA_III_K"),
    "SNOVA_III_B": ("SNOVA.yaml", "SNOVA_III_B"),
    "SNOVA_III_S": ("SNOVA.yaml", "SNOVA_III_S"),
    "SNOVA_V_K": ("SNOVA.yaml", "SNOVA_V_K"),
    "SNOVA_V_B": ("SNOVA.yaml", "SNOVA_V_B"),
    "SNOVA_V_S": ("SNOVA.yaml", "SNOVA_V_S"),
    # SNOVA v3.0 alternative parameter sets (spec Table 6); odd-q sets use the reference implementation
    "SNOVA (27 3 16 4 7 5 84)": ("SNOVA.yaml", "SNOVA (27 3 16 4 7 5 84)"),
    "SNOVA (27 4 16 4 5 5 80)": ("SNOVA.yaml", "SNOVA (27 4 16 4 5 5 80)"),
    "SNOVA (27 4 19 4 6 5 96)": ("SNOVA.yaml", "SNOVA (27 4 19 4 6 5 96)"),
    "SNOVA (27 5 13 4 4 5 80)": ("SNOVA.yaml", "SNOVA (27 5 13 4 4 5 80)"),
    "SNOVA (50 17 16 2 2 17 68)": ("SNOVA.yaml", "SNOVA (50 17 16 2 2 17 68)"),
    "SNOVA (27 4 16 5 5 4 100)": ("SNOVA.yaml", "SNOVA (27 4 16 5 5 4 100)"),
    "SNOVA (33 5 16 4 5 6 100)": ("SNOVA.yaml", "SNOVA (33 5 16 4 5 6 100)"),
    "SNOVA (39 5 11 4 6 7 120)": ("SNOVA.yaml", "SNOVA (39 5 11 4 6 7 120)"),
    "SNOVA (38 5 19 4 6 7 120)": ("SNOVA.yaml", "SNOVA (38 5 19 4 6 7 120)"),
    "SNOVA (76 25 16 2 2 25 100)": ("SNOVA.yaml", "SNOVA (76 25 16 2 2 25 100)"),
    "SNOVA (40 5 19 5 6 6 150)": ("SNOVA.yaml", "SNOVA (40 5 19 5 6 6 150)"),
    "SNOVA (47 6 19 4 6 9 144)": ("SNOVA.yaml", "SNOVA (47 6 19 4 6 9 144)"),
    "SNOVA (104 33 16 2 2 33 132)": ("SNOVA.yaml", "SNOVA (104 33 16 2 2 33 132)"),
    # UOV
    "UOV-Ip-pkc": ("UOV.yaml", "Ip-pkc"),
    "UOV-Ip-classic": ("UOV.yaml", "Ip-classic"),
    "UOV-Is-pkc": ("UOV.yaml", "Is-pkc"),
    "UOV-Is-classic": ("UOV.yaml", "Is-classic"),
    "UOV-III-pkc": ("UOV.yaml", "III-pkc"),
    "UOV-III-classic": ("UOV.yaml", "III-classic"),
    "UOV-V-pkc": ("UOV.yaml", "V-pkc"),
    "UOV-V-classic": ("UOV.yaml", "V-classic"),
    # QR-UOV Round 3 — bench uses commas, YAML uses spaces inside parens.
    # Round 3 dropped the q=31/q=7 sets, so only the six q=127 sets are built;
    # the Round 2 version entry keeps its own (Round 2 implementation) numbers.
    "QR-UOV-I-(127,156,54,3)": ("QR-UOV.yaml", "I-(127 156 54 3)"),
    "QR-UOV-I-(127,540,60,10)": ("QR-UOV.yaml", "I-(127 540 60 10)"),
    "QR-UOV-III-(127,228,78,3)": ("QR-UOV.yaml", "III-(127 228 78 3)"),
    "QR-UOV-III-(127,820,90,10)": ("QR-UOV.yaml", "III-(127 820 90 10)"),
    "QR-UOV-V-(127,306,105,3)": ("QR-UOV.yaml", "V-(127 306 105 3)"),
    "QR-UOV-V-(127,1040,110,10)": ("QR-UOV.yaml", "V-(127 1040 110 10)"),
    # Round 3 Appendix B sets (q = 7 / 31), still built by upstream
    "QR-UOV-I-(7,740,100,10)": ("QR-UOV.yaml", "I-(7 740 100 10)"),
    "QR-UOV-I-(31,165,60,3)": ("QR-UOV.yaml", "I-(31 165 60 3)"),
    "QR-UOV-I-(31,600,70,10)": ("QR-UOV.yaml", "I-(31 600 70 10)"),
    "QR-UOV-III-(7,1100,140,10)": ("QR-UOV.yaml", "III-(7 1100 140 10)"),
    "QR-UOV-III-(31,246,87,3)": ("QR-UOV.yaml", "III-(31 246 87 3)"),
    "QR-UOV-III-(31,890,100,10)": ("QR-UOV.yaml", "III-(31 890 100 10)"),
    "QR-UOV-V-(7,1490,190,10)": ("QR-UOV.yaml", "V-(7 1490 190 10)"),
    "QR-UOV-V-(31,324,114,3)": ("QR-UOV.yaml", "V-(31 324 114 3)"),
    "QR-UOV-V-(31,1120,120,10)": ("QR-UOV.yaml", "V-(31 1120 120 10)"),
    # MQOM v3.0 (round 3) — bench MQOM3- prefix dropped in YAML
    "MQOM3-L1-gf2-shorter-ct": ("MQOM.yaml", "L1-gf2-shorter-ct"),
    "MQOM3-L1-gf2-shorter-ot": ("MQOM.yaml", "L1-gf2-shorter-ot"),
    "MQOM3-L1-gf16-short-ct": ("MQOM.yaml", "L1-gf16-short-ct"),
    "MQOM3-L1-gf16-short-ot": ("MQOM.yaml", "L1-gf16-short-ot"),
    "MQOM3-L1-gf16-fast-ct": ("MQOM.yaml", "L1-gf16-fast-ct"),
    "MQOM3-L1-gf16-fast-ot": ("MQOM.yaml", "L1-gf16-fast-ot"),
    "MQOM3-L3-gf2-shorter-ct": ("MQOM.yaml", "L3-gf2-shorter-ct"),
    "MQOM3-L3-gf2-shorter-ot": ("MQOM.yaml", "L3-gf2-shorter-ot"),
    "MQOM3-L3-gf16-short-ct": ("MQOM.yaml", "L3-gf16-short-ct"),
    "MQOM3-L3-gf16-short-ot": ("MQOM.yaml", "L3-gf16-short-ot"),
    "MQOM3-L3-gf16-fast-ct": ("MQOM.yaml", "L3-gf16-fast-ct"),
    "MQOM3-L3-gf16-fast-ot": ("MQOM.yaml", "L3-gf16-fast-ot"),
    "MQOM3-L5-gf2-shorter-ct": ("MQOM.yaml", "L5-gf2-shorter-ct"),
    "MQOM3-L5-gf2-shorter-ot": ("MQOM.yaml", "L5-gf2-shorter-ot"),
    "MQOM3-L5-gf16-short-ct": ("MQOM.yaml", "L5-gf16-short-ct"),
    "MQOM3-L5-gf16-short-ot": ("MQOM.yaml", "L5-gf16-short-ot"),
    "MQOM3-L5-gf16-fast-ct": ("MQOM.yaml", "L5-gf16-fast-ct"),
    "MQOM3-L5-gf16-fast-ot": ("MQOM.yaml", "L5-gf16-fast-ot"),
    # AIMer v3.0 (KpqC; reference implementation) — names match the YAML
    "AIMer-128f": ("AIMer.yaml", "AIMer-128f"),
    "AIMer-128s": ("AIMer.yaml", "AIMer-128s"),
    "AIMer-192f": ("AIMer.yaml", "AIMer-192f"),
    "AIMer-192s": ("AIMer.yaml", "AIMer-192s"),
    "AIMer-256f": ("AIMer.yaml", "AIMer-256f"),
    "AIMer-256s": ("AIMer.yaml", "AIMer-256s"),
    # HAETAE 1.2.0 (KS draft; reference implementation) — names match the YAML
    "HAETAE-2": ("HAETAE.yaml", "HAETAE-2"),
    "HAETAE-3": ("HAETAE.yaml", "HAETAE-3"),
    "HAETAE-5": ("HAETAE.yaml", "HAETAE-5"),
    # Mirath v2.1.0, RYDE v2.1.0, PERK v2.2.0 (round 2; optimised AVX2 implementations)
    "Mirath-1a-Short": ("Mirath.yaml", "1a-Short"),
    "Mirath-1a-Fast": ("Mirath.yaml", "1a-Fast"),
    "Mirath-1b-Short": ("Mirath.yaml", "1b-Short"),
    "Mirath-1b-Fast": ("Mirath.yaml", "1b-Fast"),
    "Mirath-3a-Short": ("Mirath.yaml", "3a-Short"),
    "Mirath-3a-Fast": ("Mirath.yaml", "3a-Fast"),
    "Mirath-3b-Short": ("Mirath.yaml", "3b-Short"),
    "Mirath-3b-Fast": ("Mirath.yaml", "3b-Fast"),
    "Mirath-5a-Short": ("Mirath.yaml", "5a-Short"),
    "Mirath-5a-Fast": ("Mirath.yaml", "5a-Fast"),
    "Mirath-5b-Short": ("Mirath.yaml", "5b-Short"),
    "Mirath-5b-Fast": ("Mirath.yaml", "5b-Fast"),
    "RYDE-1-Short": ("RYDE.yaml", "1-Short"),
    "RYDE-1-Fast": ("RYDE.yaml", "1-Fast"),
    "RYDE-1-Faster": ("RYDE.yaml", "1-Faster"),
    "RYDE-3-Short": ("RYDE.yaml", "3-Short"),
    "RYDE-3-Fast": ("RYDE.yaml", "3-Fast"),
    "RYDE-3-Faster": ("RYDE.yaml", "3-Faster"),
    "RYDE-5-Short": ("RYDE.yaml", "5-Short"),
    "RYDE-5-Fast": ("RYDE.yaml", "5-Fast"),
    "RYDE-5-Faster": ("RYDE.yaml", "5-Faster"),
    "PERK-1-short": ("PERK.yaml", "1-short"),
    "PERK-1-fast": ("PERK.yaml", "1-fast"),
    "PERK-3-short": ("PERK.yaml", "3-short"),
    "PERK-3-fast": ("PERK.yaml", "3-fast"),
    "PERK-5-short": ("PERK.yaml", "5-short"),
    "PERK-5-fast": ("PERK.yaml", "5-fast"),
    # CROSS v2.2 and LESS v2.0 (round 2; optimised AVX2 implementations)
    "CROSS-RSDP-1-fast": ("CROSS.yaml", "R-SDP 1 fast"),
    "CROSS-RSDP-1-balanced": ("CROSS.yaml", "R-SDP 1 balanced"),
    "CROSS-RSDP-1-small": ("CROSS.yaml", "R-SDP 1 small"),
    "CROSS-RSDPG-1-fast": ("CROSS.yaml", "R-SDP(G) 1 fast"),
    "CROSS-RSDPG-1-balanced": ("CROSS.yaml", "R-SDP(G) 1 balanced"),
    "CROSS-RSDPG-1-small": ("CROSS.yaml", "R-SDP(G) 1 small"),
    "CROSS-RSDP-3-fast": ("CROSS.yaml", "R-SDP 3 fast"),
    "CROSS-RSDP-3-balanced": ("CROSS.yaml", "R-SDP 3 balanced"),
    "CROSS-RSDP-3-small": ("CROSS.yaml", "R-SDP 3 small"),
    "CROSS-RSDPG-3-fast": ("CROSS.yaml", "R-SDP(G) 3 fast"),
    "CROSS-RSDPG-3-balanced": ("CROSS.yaml", "R-SDP(G) 3 balanced"),
    "CROSS-RSDPG-3-small": ("CROSS.yaml", "R-SDP(G) 3 small"),
    "CROSS-RSDP-5-fast": ("CROSS.yaml", "R-SDP 5 fast"),
    "CROSS-RSDP-5-balanced": ("CROSS.yaml", "R-SDP 5 balanced"),
    "CROSS-RSDP-5-small": ("CROSS.yaml", "R-SDP 5 small"),
    "CROSS-RSDPG-5-fast": ("CROSS.yaml", "R-SDP(G) 5 fast"),
    "CROSS-RSDPG-5-balanced": ("CROSS.yaml", "R-SDP(G) 5 balanced"),
    "CROSS-RSDPG-5-small": ("CROSS.yaml", "R-SDP(G) 5 small"),
    "LESS-252-192": ("LESS.yaml", "252-192"),
    "LESS-252-68": ("LESS.yaml", "252-68"),
    "LESS-252-45": ("LESS.yaml", "252-45"),
    "LESS-400-220": ("LESS.yaml", "400-220"),
    "LESS-400-102": ("LESS.yaml", "400-102"),
    "LESS-548-345": ("LESS.yaml", "548-345"),
    "LESS-548-137": ("LESS.yaml", "548-137"),
    # SDitH v3.0.0 (round 3) gf2 sets; bench output name matches the shim's @NAME@
    "SDitH3-L1-gf2-short": ("SDitH.yaml", "SDitH3-L1-gf2-short"),
    "SDitH2-L1-gf2-short": (
        "SDitH.yaml",
        "SDitH3-L1-gf2-short",
    ),  # results files before 2026-10-14 (same v3.0.0 code)
    "SDitH3-L1-gf2-fast": ("SDitH.yaml", "SDitH3-L1-gf2-fast"),
    "SDitH2-L1-gf2-fast": (
        "SDitH.yaml",
        "SDitH3-L1-gf2-fast",
    ),  # results files before 2026-10-14 (same v3.0.0 code)
    "SDitH3-L3-gf2-short": ("SDitH.yaml", "SDitH3-L3-gf2-short"),
    "SDitH2-L3-gf2-short": (
        "SDitH.yaml",
        "SDitH3-L3-gf2-short",
    ),  # results files before 2026-10-14 (same v3.0.0 code)
    "SDitH3-L3-gf2-fast": ("SDitH.yaml", "SDitH3-L3-gf2-fast"),
    "SDitH2-L3-gf2-fast": (
        "SDitH.yaml",
        "SDitH3-L3-gf2-fast",
    ),  # results files before 2026-10-14 (same v3.0.0 code)
    "SDitH3-L5-gf2-short": ("SDitH.yaml", "SDitH3-L5-gf2-short"),
    "SDitH2-L5-gf2-short": (
        "SDitH.yaml",
        "SDitH3-L5-gf2-short",
    ),  # results files before 2026-10-14 (same v3.0.0 code)
    "SDitH3-L5-gf2-fast": ("SDitH.yaml", "SDitH3-L5-gf2-fast"),
    "SDitH2-L5-gf2-fast": (
        "SDitH.yaml",
        "SDitH3-L5-gf2-fast",
    ),  # results files before 2026-10-14 (same v3.0.0 code)
    # SDitH v3.0.0 alternative instances (cipher-based grinding, spec Table 5)
    "SDitH3-L1-gf2-short-cipherpow": ("SDitH.yaml", "SDitH3-L1-gf2-short-cipherpow"),
    "SDitH3-L1-gf2-fast-cipherpow": ("SDitH.yaml", "SDitH3-L1-gf2-fast-cipherpow"),
    "SDitH3-L3-gf2-short-cipherpow": ("SDitH.yaml", "SDitH3-L3-gf2-short-cipherpow"),
    "SDitH3-L3-gf2-fast-cipherpow": ("SDitH.yaml", "SDitH3-L3-gf2-fast-cipherpow"),
    "SDitH3-L5-gf2-short-cipherpow": ("SDitH.yaml", "SDitH3-L5-gf2-short-cipherpow"),
    "SDitH3-L5-gf2-fast-cipherpow": ("SDitH.yaml", "SDitH3-L5-gf2-fast-cipherpow"),
    # SQIsign
    "SQIsign-I": ("SQIsign.yaml", "I"),
    "SQIsign-III": ("SQIsign.yaml", "III"),
    "SQIsign-V": ("SQIsign.yaml", "V"),
    # Classic
    "RSA-2048 (PSS)": ("RSA.yaml", "2048"),
    "RSA-3072 (PSS)": ("RSA.yaml", "3072"),
    "RSA-4096 (PSS)": ("RSA.yaml", "4096"),
    "ECDSA-P256 (SHA-256)": ("ECDSA.yaml", "P-256"),
    "ECDSA-P384 (SHA-384)": ("ECDSA.yaml", "P-384"),
    "ECDSA-P521 (SHA-512)": ("ECDSA.yaml", "P-521"),
    "Ed25519": ("EdDSA.yaml", "Ed25519"),
    "Ed448": ("EdDSA.yaml", "Ed448"),
}

# China NGCC round-1 instances: bench names are NGCC-<instance>. The mapping comes
# from data/ngcc/candidates.yaml (see bench-common/ngcc/ngcc_data.py).
sys.path.insert(0, str(REPO_ROOT / "bench-common" / "ngcc"))
sys.path.insert(0, str(REPO_ROOT / "bench-common"))
import bench_env
import fingerprint
import ngcc_data

NGCC_CANDIDATES = REPO_ROOT / "data" / "ngcc" / "candidates.yaml"
NGCC_STATUS = REPO_ROOT / "bench" / "schemes" / "ngcc" / "status.tsv"
if NGCC_CANDIDATES.exists():
    BENCH_TO_YAML.update(
        ngcc_data.bench_to_yaml(ngcc_data.load_candidates(NGCC_CANDIDATES), "signature")
    )


def import_ngcc_sizes(sizes_file: Path) -> None:
    """Create or refresh the NGCC signature YAML files from `--sizes NGCC` output."""
    cands = ngcc_data.load_candidates(NGCC_CANDIDATES)
    try:
        written = ngcc_data.write_files(
            cands,
            "signature",
            ngcc_data.load_sizes(sizes_file),
            ngcc_data.load_status(NGCC_STATUS),
            SCHEMES_DIR,
            ngcc_data.load_untimed(NGCC_STATUS.with_name("params.tsv")),
            ngcc_data.load_rows(NGCC_STATUS.with_name("params.tsv")),
        )
    except ngcc_data.NgccDataError as e:
        sys.exit(f"NGCC import failed:\n{e}")
    print(
        f"Wrote {len(written)} NGCC signature files in {SCHEMES_DIR.relative_to(REPO_ROOT)}"
    )


# Results rows: "<name> <iters> kg_cyc kg_us sign_cyc sign_us verify_cyc verify_us",
# or a DNF row (scheme crashed, timed out or failed its warm-up check) whose six
# timing columns read DNF, followed by a one-word reason. The name may contain
# spaces. Anything else (stderr lines merged in by run_bench.sh) is skipped.
_NUM = r"(\d+)\s+(\d+\.\d+)"  # cycles, then µs printed as %.1f
ROW_RE = re.compile(
    rf"^(?P<name>\S.*?)\s+(?P<iters>\d+)\s+{_NUM}\s+{_NUM}\s+{_NUM}\s*$"
)
DNF_RE = re.compile(
    r"^(?P<name>\S.*?)\s+(?P<iters>\d+|-)(?:\s+DNF){6}\s+(?P<reason>\S+)\s*$"
)


def parse_results_file(path: Path) -> tuple[dict, list[dict]]:
    """Return (env_meta, rows). rows: dicts with name/iters/dnf and, unless dnf, the timings."""
    meta: dict[str, str] = {}
    rows: list[dict] = []
    in_data = False

    with open(path) as f:
        for line in f:
            line = line.rstrip("\n")
            if line.startswith("# "):
                # header metadata lines: "# key: value"
                src_m = re.match(r"^#\s+schemes/(\w+)(?:/ref)?:\s+(https?://\S+)", line)
                if src_m:
                    meta.setdefault("_sources", {})[src_m.group(1)] = src_m.group(2)
                else:
                    m = re.match(r"^#\s+([\w_]+):\s*(.*)", line)
                    if m:
                        meta[m.group(1)] = m.group(2).strip()
            elif line.startswith("scheme") or line.startswith("------"):
                in_data = True
            elif in_data and (m := DNF_RE.match(line)):
                iters = m["iters"]
                rows.append(
                    {
                        "name": m["name"],
                        "iters": int(iters) if iters.isdigit() else None,
                        "dnf": m["reason"],
                    }
                )
            elif in_data and (m := ROW_RE.match(line)):
                _kg_cyc, _kg_us, sign_cyc, sign_us, verify_cyc, verify_us = m.groups()[
                    2:
                ]
                rows.append(
                    {
                        "name": m["name"],
                        "iters": int(m["iters"]),
                        "dnf": None,
                        "keygen_cycles": int(_kg_cyc),
                        "signing_cycles": int(sign_cyc),
                        "verification_cycles": int(verify_cyc),
                        "signing_us": float(sign_us),
                        "verification_us": float(verify_us),
                    }
                )
    return meta, rows


SIG_TIMING = ngcc_data.TIMING["signature"]


def targets(value) -> list[tuple[str, str, str | None]]:
    """A BENCH_TO_YAML value as a list of (yaml_filename, parameterset_name, version)
    targets; version is None for "the first version that has the set"."""
    vals = value if isinstance(value, list) else [value]
    return [(t[0], t[1], t[2] if len(t) > 2 else None) for t in vals]


def select_version(yaml_data, selector: str):
    """The one version whose `version` label (and, after "@", `date`) matches selector.

    Raises ValueError unless exactly one version matches: a mapping that names a version
    must never silently land on another one.
    """
    label, _, date = selector.partition("@")
    found = [
        v
        for v in yaml_data.get("versions") or []
        if str(v.get("version", "")) == label
        and (not date or str(v.get("date")) == date)
    ]
    if len(found) != 1:
        raise ValueError(f"version {selector!r} matches {len(found)} versions")
    return found[0]


def find_parameterset(yaml_data, ps_name: str, version: str | None = None):
    """(version, parameterset) for a target, or (None, None) if the set is not there.

    Without a version selector: the first version containing ps_name."""
    versions = (
        [select_version(yaml_data, version)]
        if version is not None
        else yaml_data.get("versions") or []
    )
    return next(
        (
            (v, p)
            for v in versions
            for p in v.get("parametersets") or []
            if str(p.get("name", "")) == ps_name
        ),
        (None, None),
    )


def apply_row(
    yaml_data,
    ps_name: str,
    row: dict,
    env: str | None = None,
    version: str | None = None,
) -> tuple[bool, str | None]:
    """Apply one results row to ps_name in `version` (a select_version selector) or,
    without one, in the first version containing ps_name.

    Returns (changed, message); message is None if ps_name was not found.
    A DNF row on an NGCC parameter set removes its timings and notes why; on any
    other scheme it only warns, so a crash never overwrites measured timings.
    """
    version_doc, ps = find_parameterset(yaml_data, ps_name, version)
    if ps is None:
        return False, None
    ngcc = row["name"].startswith(ngcc_data.PREFIX)
    if row["dnf"]:
        if ngcc:
            ngcc_data.mark_dnf(ps, "signature", row["dnf"])
            return True, f"DNF ({row['dnf']}): timings removed, noted"
        return False, f"WARNING: DNF ({row['dnf']}); existing timings kept"
    for field in SIG_TIMING:
        ps[field] = row[field]
    if env:
        bench_env.set_env(ps, env, SIG_TIMING)
    if ngcc:
        ngcc_data.clear_dnf(ps)
    ngcc_data.set_perf_source(version_doc, SIG_TIMING)
    return True, (
        f"sign={row['signing_cycles']:>12,} ({row['signing_us']} µs)  "
        f"verify={row['verification_cycles']:>12,} ({row['verification_us']} µs)"
    )


def target_errors() -> list[str]:
    """Every BENCH_TO_YAML target that does not resolve to a parameter set in the data
    (missing file, ambiguous or unknown version, set not in that version)."""
    yaml = YAML(typ="safe")
    docs: dict[str, object] = {}
    errors = []
    for name, value in sorted(BENCH_TO_YAML.items()):
        for f, ps_name, version in targets(value):
            if f not in docs:
                path = SCHEMES_DIR / f
                docs[f] = yaml.load(path.read_text()) if path.exists() else None
            if docs[f] is None:
                errors.append(f"{name}: {f} does not exist")
                continue
            try:
                _, ps = find_parameterset(docs[f], ps_name, version)
            except ValueError as e:
                errors.append(f"{name}: {f}: {e}")
                continue
            if ps is None:
                where = f" in version {version!r}" if version else ""
                errors.append(f"{name}: {f} has no parameter set {ps_name!r}{where}")
    return errors


def mapped_sets(yaml_data, yaml_filename: str) -> set[tuple[int, str]]:
    """(version index, parameterset name) of every set in one YAML file that some
    BENCH_TO_YAML target lands on."""
    versions = yaml_data.get("versions") or []
    out: set[tuple[int, str]] = set()
    for value in BENCH_TO_YAML.values():
        for f, ps_name, version in targets(value):
            if f != yaml_filename:
                continue
            v, ps = find_parameterset(yaml_data, ps_name, version)
            if ps is not None:
                out.add((next(i for i, x in enumerate(versions) if x is v), ps_name))
    return out


NO_PROGRAMME = "no programme"


def membership_label(programmes) -> str:
    """A version's programme memberships as one report label, e.g. "nist-onramp round-2"."""
    if not programmes:
        return NO_PROGRAMME
    parts = []
    for programme, stage in sorted(programmes.items()):
        stages = stage if isinstance(stage, list) else [stage]
        parts.append(f"{programme} {'/'.join(str(s) for s in stages)}")
    return ", ".join(parts)


def record_fingerprints(results_file: Path, rows: list[dict]) -> None:
    """Record each timed row (and NGCC DNF rows, which the YAML now shows as DNF) in
    bench/results/fingerprints.tsv with the fingerprint of the commit its run built.
    A DNF row of any other scheme is not recorded: its old timings stay, and so does
    its staleness."""
    outcomes = {
        r["name"]: f"dnf:{r['dnf']}" if r["dnf"] else "ok"
        for r in rows
        if not r["dnf"] or r["name"].startswith(ngcc_data.PREFIX)
    }
    warnings = fingerprint.record(REPO_ROOT, "bench", Path(results_file), outcomes)
    print(f"Recorded {len(outcomes)} rows in bench/results/fingerprints.tsv")
    for w in warnings:
        print(f"  WARNING: {w}")


def round_trip_yaml() -> YAML:
    yaml = YAML()
    yaml.preserve_quotes = True
    yaml.width = 4096
    return yaml


def import_rows(rows: list[dict], env: str | None) -> tuple[int, int, list[str]]:
    """Write the rows' timings into SCHEMES_DIR; (updated, skipped, unmapped names)."""
    yaml = round_trip_yaml()
    updated = 0
    skipped = 0
    not_mapped = []

    # Group updates by yaml file to minimise file reads/writes
    file_updates: dict[str, list[tuple[str, str | None, dict]]] = {}
    for row in rows:
        name = row["name"]
        if name not in BENCH_TO_YAML:
            not_mapped.append(name)
            continue
        for yaml_file, ps_name, version in targets(BENCH_TO_YAML[name]):
            file_updates.setdefault(yaml_file, []).append((ps_name, version, row))

    for yaml_filename, updates in sorted(file_updates.items()):
        yaml_path = SCHEMES_DIR / yaml_filename
        if not yaml_path.exists():
            print(f"  MISSING: {yaml_filename}")
            skipped += len(updates)
            continue

        with open(yaml_path) as f:
            data = yaml.load(f)

        changed = False
        for ps_name, version, row in updates:
            where = f"{ps_name} [{version}]" if version else ps_name
            ps_changed, msg = apply_row(data, ps_name, row, env, version)
            if msg is None:
                print(f"  NOT FOUND: {yaml_filename}  '{where}'")
                skipped += 1
                continue
            print(f"  {yaml_filename}  {where}: {msg}")
            if ps_changed:
                updated += 1
                changed = True
            else:
                skipped += 1

        if changed:
            with open(yaml_path, "w") as f:
                yaml.dump(data, f)
    return updated, skipped, not_mapped


def main() -> None:
    if len(sys.argv) == 3 and sys.argv[1] == "--ngcc-sizes":
        import_ngcc_sizes(Path(sys.argv[2]))
        return
    results_dir = REPO_ROOT / "bench" / "results"

    if len(sys.argv) > 1:
        results_file = Path(sys.argv[1])
    else:
        files = sorted(results_dir.glob("*.txt"))
        if not files:
            sys.exit("No results files found in bench/results/")
        results_file = files[-1]
        print(f"Using {results_file.name}")

    errors = target_errors()
    if errors:
        sys.exit("BENCH_TO_YAML targets that do not resolve:\n  " + "\n  ".join(errors))

    meta, rows = parse_results_file(results_file)
    print(f"Parsed {len(rows)} benchmark rows")
    env = bench_env.env_id(meta)

    updated, skipped, not_mapped = import_rows(rows, env)
    yaml = round_trip_yaml()

    print(f"\nUpdated {updated} parametersets, skipped {skipped}")
    if not_mapped:
        print(f"\nNo YAML mapping for {len(not_mapped)} bench names:")
        for n in not_mapped:
            print(f"  {n}")

    # Report what was NOT updated
    run_bench_names = {row["name"] for row in rows}

    # Bench-mapped parametersets absent from this run's results
    not_in_run: dict[str, list[str]] = {}
    for bench_name, value in sorted(BENCH_TO_YAML.items()):
        if bench_name not in run_bench_names:
            for yaml_file, ps_name, version in targets(value):
                label = f"{ps_name} [{version}]" if version else ps_name
                not_in_run.setdefault(yaml_file, []).append(label)

    # Parametersets in YAML files with no bench mapping, grouped by their version's
    # programme memberships (e.g. "nist-onramp round-2"); "no programme" last.
    no_bench: dict[str, dict[str, list[str]]] = {}
    for yaml_path in sorted(SCHEMES_DIR.glob("*.yaml")):
        with open(yaml_path) as f:
            data = yaml.load(f)
        mapped = mapped_sets(data, yaml_path.name)
        for i, version in enumerate(data.get("versions") or []):
            group = membership_label(version.get("programmes"))
            for ps in version.get("parametersets") or []:
                if (i, str(ps.get("name", ""))) not in mapped:
                    no_bench.setdefault(group, {}).setdefault(
                        yaml_path.name, []
                    ).append(f"{ps.get('name', '')} [{version.get('version')}]")

    if not_in_run:
        total = sum(len(v) for v in not_in_run.values())
        print(f"\nBench-mapped but not in this run ({total}):")
        for yaml_file, names in sorted(not_in_run.items()):
            print(f"  {yaml_file}: {', '.join(names)}")

    total_no_bench = sum(len(n) for g in no_bench.values() for n in g.values())
    if total_no_bench:
        print(f"\nNo bench mapping ({total_no_bench}):")
        for group in sorted(no_bench, key=lambda g: (g == NO_PROGRAMME, g)):
            by_file = no_bench[group]
            subtotal = sum(len(v) for v in by_file.values())
            print(f"\n  {group} ({subtotal}):")
            for yaml_file, names in sorted(by_file.items()):
                print(f"    {yaml_file}: {', '.join(names)}")

    bench_env.update_env_file(BENCH_ENV_FILE, bench_env.make_env(meta), SCHEMES_DIR)
    record_fingerprints(results_file, rows)


if __name__ == "__main__":
    main()
