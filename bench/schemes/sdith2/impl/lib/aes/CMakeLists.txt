set(AES_SRCS_GENERIC
        rijndael256.c
        rijndael256.h
        aes_ansi_ref.c
        aes128_ctrle.h
        aes128_ctrle_ref.c
        aes128_ctrle_special_ref.c
        aes128_ctrle_generics.impl.h
        rijndael256_ctrle.h
        rijndael256_ctrle_generics.impl.h
        rijndael256_ctrle_ref.c
        rijndael256_ctrle_special_ref.c
)
set(AES_SRCS_AVX2
        rijndael256_avx.c
        rijndael256_avx.h
        aes128_ctrle_special_avx2_unix.s
        aes128_ctrle_avx2.c
        rijndael256_ctrle_avx2.c
)
set_source_files_properties(${AES_SRCS_AVX2} PROPERTIES COMPILE_OPTIONS "-mpclmul;-msse2;-mavx2;-maes")


if (X86_LINUX)
    add_library(aes STATIC ${AES_SRCS_GENERIC} ${AES_SRCS_AVX2})
    target_include_directories(aes PUBLIC .)
    target_link_libraries(aes)
else ()
    add_library(aes STATIC ${AES_SRCS_GENERIC})
    target_include_directories(aes PUBLIC .)
    target_link_libraries(aes)
endif ()
