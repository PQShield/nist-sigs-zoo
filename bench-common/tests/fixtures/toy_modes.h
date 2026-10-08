/* Misbehaviour modes shared by toy_sig.c and toy_kem.c. */
#pragma once

#define TOY_OK 0       /* works; prints chatter on stdout from keygen */
#define TOY_ABORT 1    /* SIGABRT in sign / encaps */
#define TOY_SEGV 2     /* SIGSEGV in sign / encaps */
#define TOY_HANG 3     /* never returns from sign / encaps */
#define TOY_BADCHECK 4 /* fails the warm-up correctness check */
#define TOY_FPE 5      /* SIGFPE (integer division by zero) in sign / encaps */

#ifndef TOY_MODE
#define TOY_MODE TOY_OK
#endif

/* Candidates sometimes print to stdout; that must never end up in a results row. */
#define TOY_CHATTER() do { printf("toy chatter on stdout 1 2 3 4 5 6 7 8\n"); fflush(stdout); } while (0)

static volatile int toy_zero = 0, toy_num = 12345;

#define TOY_MISBEHAVE() do {                                   \
    if (TOY_MODE == TOY_ABORT) abort();                        \
    if (TOY_MODE == TOY_SEGV) *(volatile int *)(uintptr_t)toy_zero = 1; \
    if (TOY_MODE == TOY_HANG) for (;;) pause();                \
    if (TOY_MODE == TOY_FPE) toy_zero = toy_num / toy_zero;        \
} while (0)
