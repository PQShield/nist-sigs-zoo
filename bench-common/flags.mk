# The one compiler flag set of every benchmarked scheme, in bench/ and bench-kem/, NGCC
# included. Included by both suite Makefiles, every scheme Makefile and ngcc.mk. Policy and
# reasons: "Compiler flags" in bench/CLAUDE.md. A change here re-stales every timed row
# (bench-common/fingerprint.py hashes this file into the harness component).
#
# Scheme Makefiles put $(BENCH_CFLAGS) after their own flags, add -fPIC (every scheme is
# a shared object) and keep only flags a build needs: -std, -m ISA flags for intrinsics,
# -D configuration, -W warnings, and any recorded exception.
BENCH_CFLAGS := -O3 -march=native -mtune=native -fno-semantic-interposition -DNDEBUG \
    -fno-stack-protector -U_FORTIFY_SOURCE -fno-stack-clash-protection -fcf-protection=none
# C++: the same flags; each build adds its own -std.
BENCH_CXXFLAGS := $(BENCH_CFLAGS)
