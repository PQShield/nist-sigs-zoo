/* EXPORT void aes128_ctrle_set_key_avx2(void* round_keys, const void* aes128key); */

aes128_ctrle_keygenassist_aux_xmm0_xmm1:
  pshufd $0xff, %xmm1, %xmm1      # X:X:X:X
  pxor %xmm0, %xmm1               # X+r3:X+r2:X+r1:r4
  pslldq $4, %xmm0                # r2:r1:r0:0
  pxor %xmm0, %xmm1               # X+r3+r2:X+r2+r1:r5:r4
  pslldq $4, %xmm0                # etc
  pxor %xmm0, %xmm1
  pslldq $4, %xmm0
  pxor %xmm1, %xmm0               # update xmm0 for next time!
  ret
.size	aes128_ctrle_keygenassist_aux_xmm0_xmm1, .-aes128_ctrle_keygenassist_aux_xmm0_xmm1

.globl aes128_ctrle_set_key_avx2
aes128_ctrle_set_key_avx2:
  movdqu (%rsi), %xmm0      /* copy the original key */
  movdqu %xmm0, (%rdi)      /* as round key 0 */
  aeskeygenassist $0x01, %xmm0, %xmm1
  call aes128_ctrle_keygenassist_aux_xmm0_xmm1
  movdqu %xmm0, 0x10(%rdi)
  aeskeygenassist $0x02, %xmm0, %xmm1
  call aes128_ctrle_keygenassist_aux_xmm0_xmm1
  movdqu %xmm0, 0x20(%rdi)
  aeskeygenassist $0x04, %xmm0, %xmm1
  call aes128_ctrle_keygenassist_aux_xmm0_xmm1
  movdqu %xmm0, 0x30(%rdi)
  aeskeygenassist $0x08, %xmm0, %xmm1
  call aes128_ctrle_keygenassist_aux_xmm0_xmm1
  movdqu %xmm0, 0x40(%rdi)
  aeskeygenassist $0x10, %xmm0, %xmm1
  call aes128_ctrle_keygenassist_aux_xmm0_xmm1
  movdqu %xmm0, 0x50(%rdi)
  aeskeygenassist $0x20, %xmm0, %xmm1
  call aes128_ctrle_keygenassist_aux_xmm0_xmm1
  movdqu %xmm0, 0x60(%rdi)
  aeskeygenassist $0x40, %xmm0, %xmm1
  call aes128_ctrle_keygenassist_aux_xmm0_xmm1
  movdqu %xmm0, 0x70(%rdi)
  aeskeygenassist $0x80, %xmm0, %xmm1
  call aes128_ctrle_keygenassist_aux_xmm0_xmm1
  movdqu %xmm0, 0x80(%rdi)
  aeskeygenassist $0x1B, %xmm0, %xmm1
  call aes128_ctrle_keygenassist_aux_xmm0_xmm1
  movdqu %xmm0, 0x90(%rdi)
  aeskeygenassist $0x36, %xmm0, %xmm1
  call aes128_ctrle_keygenassist_aux_xmm0_xmm1
  movdqu %xmm0, 0xa0(%rdi)
  ret
.size	aes128_ctrle_set_key_avx2, .-aes128_ctrle_set_key_avx2

/* EXPORT void aes128_ctrle_encrypt_nblocks_avx2(void* out, void* out_last128, void* in_out_ctr128,  //
                                                 const void* round_keys, uint64_t nblocks); */

/*
 * EDIT: We are not retaining this design, although it has less cycles, it turns out to be slower
 * than the loop approach.
 *
.globl aes128_ctrle_encrypt_nblocks_avx2XXXX
aes128_ctrle_encrypt_nblocks_avx2XXXX:
 movdqu (%rdx),%xmm2       # load the counter in xmm2
 cmpq    $5, %r8
 jb 4f
 5:
   # 5 or more blocks left
   # increase 4 counters
   vinsertf128 $1,%xmm2, %ymm2, %ymm2
   vmovapd %ymm2, %ymm7
   vpaddq p_two_three(%rip), %ymm2, %ymm4
   vpaddq p_four_one(%rip), %ymm2, %ymm2
   vpandn %ymm4, %ymm7, %ymm6
   vpandn %ymm2, %ymm7, %ymm5
   vpslldq $8, %ymm6, %ymm6
   vpslldq $8, %ymm5, %ymm5
   vpsrlq $63, %ymm6, %ymm6
   vpsrlq $63, %ymm5, %ymm5
   vpaddq %ymm6, %ymm4, %ymm4
   vpaddq %ymm5, %ymm2, %ymm2
   vextracti128 $1,%ymm2,%xmm3
   vextracti128 $1,%ymm4,%xmm5
   # encrypt 4 blocks
   movupd (%rcx), %xmm0    # round0
   pxor  %xmm0, %xmm7
   pxor  %xmm0, %xmm3
   pxor  %xmm0, %xmm4
   pxor  %xmm0, %xmm5
   movupd 0x10(%rcx), %xmm0    # round1
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x20(%rcx), %xmm0    # round2
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x30(%rcx), %xmm0    # round3
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x40(%rcx), %xmm0    # round4
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x50(%rcx), %xmm0    # round5
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x60(%rcx), %xmm0    # round6
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x70(%rcx), %xmm0    # round7
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x80(%rcx), %xmm0    # round8
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x90(%rcx), %xmm0    # round9
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0xa0(%rcx), %xmm0    # round10
   aesenclast  %xmm0, %xmm7
   aesenclast  %xmm0, %xmm3
   aesenclast  %xmm0, %xmm4
   aesenclast  %xmm0, %xmm5
   #save
   movupd %xmm7, (%rdi)
   movupd %xmm3, 0x10(%rdi)
   movupd %xmm4, 0x20(%rdi)
   movupd %xmm5, 0x30(%rdi)
   addq $0x40, %rdi
   subq $4,%r8
   cmpq    $5, %r8
   jge 5b
 4:
   cmpq    $3, %r8
   jb 2f
   je 3f
   # 4 blocks left
   # increase 4 counters
   vinsertf128 $1,%xmm2, %ymm2, %ymm2
   vmovapd %ymm2, %ymm7
   vpaddq p_two_three(%rip), %ymm2, %ymm4
   vpaddq p_four_one(%rip), %ymm2, %ymm2
   vpandn %ymm4, %ymm7, %ymm6
   vpandn %ymm2, %ymm7, %ymm5
   vpslldq $8, %ymm6, %ymm6
   vpslldq $8, %ymm5, %ymm5
   vpsrlq $63, %ymm6, %ymm6
   vpsrlq $63, %ymm5, %ymm5
   vpaddq %ymm6, %ymm4, %ymm4
   vpaddq %ymm5, %ymm2, %ymm2
   vextracti128 $1,%ymm2,%xmm3
   vextracti128 $1,%ymm4,%xmm5
   # encrypt 4 blocks
   movupd (%rcx), %xmm0    # round0
   pxor  %xmm0, %xmm7
   pxor  %xmm0, %xmm3
   pxor  %xmm0, %xmm4
   pxor  %xmm0, %xmm5
   movupd 0x10(%rcx), %xmm0    # round1
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x20(%rcx), %xmm0    # round2
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x30(%rcx), %xmm0    # round3
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x40(%rcx), %xmm0    # round4
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x50(%rcx), %xmm0    # round5
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x60(%rcx), %xmm0    # round6
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x70(%rcx), %xmm0    # round7
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x80(%rcx), %xmm0    # round8
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x90(%rcx), %xmm0    # round9
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0xa0(%rcx), %xmm0    # round10
   aesenclast  %xmm0, %xmm7
   aesenclast  %xmm0, %xmm3
   aesenclast  %xmm0, %xmm4
   aesenclast  %xmm0, %xmm5
   #save
   movupd %xmm7, (%rdi)
   movupd %xmm3, 0x10(%rdi)
   movupd %xmm4, 0x20(%rdi)
   movupd %xmm5, (%rsi)    # last block goes to rsi
   jmp 0f
 3:
   # 3 blocks left
   # increase 3 counters
   vinsertf128 $1,%xmm2, %ymm2, %ymm2
   vmovapd %ymm2, %ymm7
   vpaddq p_one_two(%rip), %ymm2, %ymm4
   vpaddq p_three(%rip), %xmm2, %xmm2
   vpandn %ymm4, %ymm7, %ymm6
   vpandn %xmm2, %xmm7, %xmm5
   vpslldq $8, %ymm6, %ymm6
   vpslldq $8, %xmm5, %xmm5
   vpsrlq $63, %ymm6, %ymm6
   vpsrlq $63, %xmm5, %xmm5
   vpaddq %ymm6, %ymm4, %ymm4
   vpaddq %xmm5, %xmm2, %xmm2
   vextracti128 $1,%ymm4,%xmm5
   # encrypt 3 blocks
   movupd (%rcx), %xmm0    # round0
   pxor  %xmm0, %xmm7
   pxor  %xmm0, %xmm4
   pxor  %xmm0, %xmm5
   movupd 0x10(%rcx), %xmm0    # round1
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x20(%rcx), %xmm0    # round2
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x30(%rcx), %xmm0    # round3
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x40(%rcx), %xmm0    # round4
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x50(%rcx), %xmm0    # round5
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x60(%rcx), %xmm0    # round6
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x70(%rcx), %xmm0    # round7
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x80(%rcx), %xmm0    # round8
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x90(%rcx), %xmm0    # round9
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0xa0(%rcx), %xmm0    # round10
   aesenclast  %xmm0, %xmm7
   aesenclast  %xmm0, %xmm4
   aesenclast  %xmm0, %xmm5
   #save
   movupd %xmm7, (%rdi)
   movupd %xmm4, 0x10(%rdi)
   movupd %xmm5, (%rsi)    # last block goes to rsi
   jmp 0f
 2:
   cmpq    $1, %r8
   jb 0f
   je 1f
   # 2 blocks left
   # increase 2 counters
   vinsertf128 $1,%xmm2, %ymm2, %ymm2
   vmovapd %ymm2, %ymm7
   vpaddq p_two_one(%rip), %ymm2, %ymm2
   vpandn %ymm2, %ymm7, %ymm5
   vpslldq $8, %ymm5, %ymm5
   vpsrlq $63, %ymm5, %ymm5
   vpaddq %ymm5, %ymm2, %ymm2
   vextracti128 $1,%ymm2,%xmm3
   # encrypt 2 blocks
   movupd (%rcx), %xmm0    # round0
   pxor  %xmm0, %xmm7
   pxor  %xmm0, %xmm3
   movupd 0x10(%rcx), %xmm0    # round1
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   movupd 0x20(%rcx), %xmm0    # round2
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   movupd 0x30(%rcx), %xmm0    # round3
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   movupd 0x40(%rcx), %xmm0    # round4
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   movupd 0x50(%rcx), %xmm0    # round5
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   movupd 0x60(%rcx), %xmm0    # round6
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   movupd 0x70(%rcx), %xmm0    # round7
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   movupd 0x80(%rcx), %xmm0    # round8
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   movupd 0x90(%rcx), %xmm0    # round9
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   movupd 0xa0(%rcx), %xmm0    # round10
   aesenclast  %xmm0, %xmm7
   aesenclast  %xmm0, %xmm3
   #save
   movupd %xmm7, (%rdi)
   movupd %xmm3, (%rsi)    # last block goes to rsi
   jmp 0f
 1:
   # 1 blocks left
   # increase 1 counters
   vmovapd %xmm2, %xmm7
   vpaddq p_one_two(%rip), %xmm2, %xmm2
   vpandn %xmm2, %xmm7, %xmm5
   vpslldq $8, %xmm5, %xmm5
   vpsrlq $63, %xmm5, %xmm5
   vpaddq %xmm5, %xmm2, %xmm2
   # encrypt 1 blocks
   movupd (%rcx), %xmm0    # round0
   pxor  %xmm0, %xmm7
   movupd 0x10(%rcx), %xmm0    # round1
   aesenc  %xmm0, %xmm7
   movupd 0x20(%rcx), %xmm0    # round2
   aesenc  %xmm0, %xmm7
   movupd 0x30(%rcx), %xmm0    # round3
   aesenc  %xmm0, %xmm7
   movupd 0x40(%rcx), %xmm0    # round4
   aesenc  %xmm0, %xmm7
   movupd 0x50(%rcx), %xmm0    # round5
   aesenc  %xmm0, %xmm7
   movupd 0x60(%rcx), %xmm0    # round6
   aesenc  %xmm0, %xmm7
   movupd 0x70(%rcx), %xmm0    # round7
   aesenc  %xmm0, %xmm7
   movupd 0x80(%rcx), %xmm0    # round8
   aesenc  %xmm0, %xmm7
   movupd 0x90(%rcx), %xmm0    # round9
   aesenc  %xmm0, %xmm7
   movupd 0xa0(%rcx), %xmm0    # round10
   aesenclast  %xmm0, %xmm7
   #save
   movupd %xmm7, (%rsi)    # last block goes to rsi
 0:
   # 0 blocks left
   vmovupd %xmm2,(%rdx)
   vzeroall
   retq
 .balign 32
 p_four_one:  .quad 4,0,1,0
 p_two_three: .quad 2,0,3,0
 p_two_one: .quad 2,0,1,0
 p_one_two: .quad 1,0,2,0
 p_three: .quad 3,0,0,0
.size	aes128_ctrle_encrypt_nblocks_avx2XXXX, .-aes128_ctrle_encrypt_nblocks_avx2XXXX
*/

/* EXPORT void aes128_ctrle_oneshot_encrypt_2blocks_avx2(void* out256, const void* aes128key, const void* ctr128); */

.globl aes128_ctrle_oneshot_encrypt_2blocks_avx2
aes128_ctrle_oneshot_encrypt_2blocks_avx2:
   movq    (%rdx), %rax           # increase the counter -> xmm3
   movq    8(%rdx), %rcx
   addq    $1, %rax
   adcq    $0, %rcx
   movq    %rcx, %xmm0
   movq    %rax, %xmm3
   punpcklqdq  %xmm0, %xmm3
   movdqu (%rdx), %xmm2           # original counter -> xmm2

  movdqu (%rsi), %xmm0      # load the original key -> xmm0
  pxor  %xmm0, %xmm2        # round0
  pxor  %xmm0, %xmm3        # round0

  aeskeygenassist $0x01, %xmm0, %xmm1
  call aes128_ctrle_keygenassist_aux_xmm0_xmm1
  aesenc %xmm0, %xmm2
  aesenc %xmm0, %xmm3

  aeskeygenassist $0x02, %xmm0, %xmm1
  call aes128_ctrle_keygenassist_aux_xmm0_xmm1
  aesenc %xmm0, %xmm2
  aesenc %xmm0, %xmm3

  aeskeygenassist $0x04, %xmm0, %xmm1
  call aes128_ctrle_keygenassist_aux_xmm0_xmm1
  aesenc %xmm0, %xmm2
  aesenc %xmm0, %xmm3

  aeskeygenassist $0x08, %xmm0, %xmm1
  call aes128_ctrle_keygenassist_aux_xmm0_xmm1
  aesenc %xmm0, %xmm2
  aesenc %xmm0, %xmm3

  aeskeygenassist $0x10, %xmm0, %xmm1
  call aes128_ctrle_keygenassist_aux_xmm0_xmm1
  aesenc %xmm0, %xmm2
  aesenc %xmm0, %xmm3

  aeskeygenassist $0x20, %xmm0, %xmm1
  call aes128_ctrle_keygenassist_aux_xmm0_xmm1
  aesenc %xmm0, %xmm2
  aesenc %xmm0, %xmm3

  aeskeygenassist $0x40, %xmm0, %xmm1
  call aes128_ctrle_keygenassist_aux_xmm0_xmm1
  aesenc %xmm0, %xmm2
  aesenc %xmm0, %xmm3

  aeskeygenassist $0x80, %xmm0, %xmm1
  call aes128_ctrle_keygenassist_aux_xmm0_xmm1
  aesenc %xmm0, %xmm2
  aesenc %xmm0, %xmm3

  aeskeygenassist $0x1B, %xmm0, %xmm1
  call aes128_ctrle_keygenassist_aux_xmm0_xmm1
  aesenc %xmm0, %xmm2
  aesenc %xmm0, %xmm3

  aeskeygenassist $0x36, %xmm0, %xmm1
  call aes128_ctrle_keygenassist_aux_xmm0_xmm1
  aesenclast %xmm0, %xmm2
  aesenclast %xmm0, %xmm3

  movdqu %xmm2,(%rdi)
  movdqu %xmm3,0x10(%rdi)
  ret
.size	aes128_ctrle_oneshot_encrypt_2blocks_avx2, .-aes128_ctrle_oneshot_encrypt_2blocks_avx2


.globl aes128_encrypt_1block_avx2
aes128_encrypt_1block_avx2:
   # encrypt 1 blocks
   movupd (%rsi), %xmm7 #data
   movupd (%rdx), %xmm0    # round0
   pxor  %xmm0, %xmm7
   movupd 0x10(%rdx), %xmm0    # round1
   aesenc  %xmm0, %xmm7
   movupd 0x20(%rdx), %xmm0    # round2
   aesenc  %xmm0, %xmm7
   movupd 0x30(%rdx), %xmm0    # round3
   aesenc  %xmm0, %xmm7
   movupd 0x40(%rdx), %xmm0    # round4
   aesenc  %xmm0, %xmm7
   movupd 0x50(%rdx), %xmm0    # round5
   aesenc  %xmm0, %xmm7
   movupd 0x60(%rdx), %xmm0    # round6
   aesenc  %xmm0, %xmm7
   movupd 0x70(%rdx), %xmm0    # round7
   aesenc  %xmm0, %xmm7
   movupd 0x80(%rdx), %xmm0    # round8
   aesenc  %xmm0, %xmm7
   movupd 0x90(%rdx), %xmm0    # round9
   aesenc  %xmm0, %xmm7
   movupd 0xa0(%rdx), %xmm0    # round10
   aesenclast  %xmm0, %xmm7
   #save
   movupd %xmm7, (%rdi)
   ret
.size	aes128_encrypt_1block_avx2, .-aes128_encrypt_1block_avx2

.globl aes128_encrypt_2blocks_avx2
aes128_encrypt_2blocks_avx2:
   movupd (%rsi), %xmm7
   movupd 0x10(%rsi), %xmm3
   # encrypt 4 blocks
   movupd (%rdx), %xmm0    # round0
   pxor  %xmm0, %xmm7
   pxor  %xmm0, %xmm3
   movupd 0x10(%rdx), %xmm0    # round1
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   movupd 0x20(%rdx), %xmm0    # round2
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   movupd 0x30(%rdx), %xmm0    # round3
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   movupd 0x40(%rdx), %xmm0    # round4
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   movupd 0x50(%rdx), %xmm0    # round5
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   movupd 0x60(%rdx), %xmm0    # round6
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   movupd 0x70(%rdx), %xmm0    # round7
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   movupd 0x80(%rdx), %xmm0    # round8
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   movupd 0x90(%rdx), %xmm0    # round9
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   movupd 0xa0(%rdx), %xmm0    # round10
   aesenclast  %xmm0, %xmm7
   aesenclast  %xmm0, %xmm3
   #save
   movupd %xmm7, (%rdi)
   movupd %xmm3, 0x10(%rdi)
   ret
.size	aes128_encrypt_2blocks_avx2, .-aes128_encrypt_2blocks_avx2

.globl aes128_encrypt_4blocks_avx2
aes128_encrypt_4blocks_avx2:
   movupd (%rsi), %xmm7
   movupd 0x10(%rsi), %xmm3
   movupd 0x20(%rsi), %xmm4
   movupd 0x30(%rsi), %xmm5
   # encrypt 4 blocks
   movupd (%rdx), %xmm0    # round0
   pxor  %xmm0, %xmm7
   pxor  %xmm0, %xmm3
   pxor  %xmm0, %xmm4
   pxor  %xmm0, %xmm5
   movupd 0x10(%rdx), %xmm0    # round1
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x20(%rdx), %xmm0    # round2
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x30(%rdx), %xmm0    # round3
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x40(%rdx), %xmm0    # round4
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x50(%rdx), %xmm0    # round5
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x60(%rdx), %xmm0    # round6
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x70(%rdx), %xmm0    # round7
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x80(%rdx), %xmm0    # round8
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0x90(%rdx), %xmm0    # round9
   aesenc  %xmm0, %xmm7
   aesenc  %xmm0, %xmm3
   aesenc  %xmm0, %xmm4
   aesenc  %xmm0, %xmm5
   movupd 0xa0(%rdx), %xmm0    # round10
   aesenclast  %xmm0, %xmm7
   aesenclast  %xmm0, %xmm3
   aesenclast  %xmm0, %xmm4
   aesenclast  %xmm0, %xmm5
   #save
   movupd %xmm7, (%rdi)
   movupd %xmm3, 0x10(%rdi)
   movupd %xmm4, 0x20(%rdi)
   movupd %xmm5, 0x30(%rdi)
   ret
.size	aes128_encrypt_4blocks_avx2, .-aes128_encrypt_4blocks_avx2
.section .note.GNU-stack,"",@progbits
