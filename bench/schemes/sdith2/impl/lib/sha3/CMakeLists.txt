set(SHA3_SRCS
        KeccakHash.c
        KeccakHash.h
        KeccakSponge.c
        KeccakSponge.h
        KeccakSpongetimes4.c
        KeccakSpongetimes4.h
        KeccakHashtimes4.c
        KeccakHashtimes4.h
        align.h
        brg_endian.h
        config.h
        endian_compat.h
        macros.h
        s390_cpacf.h
        SnP-Relaned.h
)
set(SHA3_AVX2_SRCS
        avx2/KeccakP-1600-AVX2.s
        avx2/KeccakP-1600-times4-SIMD256.c
        avx2/KeccakP-1600-SnP.h
        avx2/KeccakP-1600-times4-SnP.h
        avx2/SIMD256-config.h
)
set(SHA3_NEON_SRCS
        armv8a-neon/KeccakP-1600-armv8a-neon.s
        armv8a-neon/KeccakP-1600-times4-on1.c
        armv8a-neon/KeccakP-1600-SnP.h
        armv8a-neon/KeccakP-1600-times4-SnP.h
)
set(SHA3_OPT64_SRCS
        opt64/KeccakP-1600-opt64.c
        opt64/KeccakP-1600-opt64-config.h
        opt64/KeccakP-1600-SnP.h
        opt64/KeccakP-1600-times4-on1.c
        opt64/KeccakP-1600-times4-SnP.h
)
set_source_files_properties(avx2/KeccakP-1600-AVX2.s PROPERTIES COMPILE_OPTIONS "-mpclmul;-msse2;-mavx2")
set_source_files_properties(avx2/KeccakP-1600-times4-SIMD256.c PROPERTIES COMPILE_OPTIONS "-mpclmul;-msse2;-mavx2")


if (X86_LINUX)
    add_library(sha3 STATIC ${SHA3_SRCS} ${SHA3_AVX2_SRCS})
    target_include_directories(sha3 PUBLIC avx2)
    target_include_directories(sha3 PUBLIC .)
else()
    add_library(sha3 STATIC ${SHA3_SRCS} ${SHA3_OPT64_SRCS})
    target_include_directories(sha3 PUBLIC opt64)
    target_include_directories(sha3 PUBLIC .)
endif()
