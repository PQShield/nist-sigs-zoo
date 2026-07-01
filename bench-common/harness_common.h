#pragma once
/*
 * Shared harness machinery for bench/ (signatures) and bench-kem/ (KEMs):
 * the cycle counter, the median/stat helpers, and the per-operation timing
 * macro. The two harnesses differ only in their op contract (keygen/sign/verify
 * vs keygen/enc/dec) and output columns, which live in each `harness.h`; the
 * timing/counter logic below is identical and lives here so it stays in sync.
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ---------- cycle counter ----------
 *
 * Reference counter: rdtsc (x86) / cntvct_el0 (aarch64). On modern x86 rdtsc is a
 * constant-rate *off-core* clock, not a cycle counter, so its "cycles" diverge
 * from real executed cycles whenever the core clock differs from the TSC rate
 * (turbo, frequency scaling). When possible we instead read the core's actual
 * CPU_CYCLES PMC via rdpmc through a perf_event mmap page — no root needed, and
 * using the kernel-assigned counter index (not the fixed-counter number) avoids
 * the hybrid P/E-core rdpmc index bug. Falls back to the reference counter when
 * perf/rdpmc is unavailable or when BENCH_CYCLES=tsc. Counting excludes kernel
 * time (forced by perf_event_paranoid >= 2), i.e. user-space cycles. */

#if defined(__x86_64__) || defined(__i386__)
static inline uint64_t cpucycles_ref(void) {
    uint32_t lo, hi;
    __asm__ __volatile__("lfence\n\trdtsc" : "=a"(lo), "=d"(hi) :: "memory");
    return ((uint64_t)hi << 32) | lo;
}
#elif defined(__aarch64__)
static inline uint64_t cpucycles_ref(void) {
    uint64_t val;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(val));
    return val;
}
#else
#error "cpucycles: unsupported architecture"
#endif

#if defined(__x86_64__) && defined(__linux__)
#include <unistd.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>

static struct perf_event_mmap_page *g_pmc_page = NULL;
static int g_pmc_fd = -1;
static int g_use_pmc = 0;

static inline uint64_t rdpmc_raw(uint32_t counter) {
    uint32_t lo, hi;
    __asm__ __volatile__("rdpmc" : "=a"(lo), "=d"(hi) : "c"(counter));
    return ((uint64_t)hi << 32) | lo;
}

/* Read the core cycle counter via the perf mmap seqlock protocol. */
static inline uint64_t cpucycles_pmc(void) {
    const struct perf_event_mmap_page *pc = g_pmc_page;
    uint64_t count, offset;
    uint32_t seq, idx, width;
    do {
        seq = pc->lock;
        __atomic_thread_fence(__ATOMIC_ACQUIRE);
        idx = pc->index;
        offset = pc->offset;
        /* index == 0 means the event is not currently scheduled (rdpmc would be
         * stale); fall back to the reference counter for this one read. */
        if (!pc->cap_user_rdpmc || !idx) return cpucycles_ref();
        width = pc->pmc_width;
        count = rdpmc_raw(idx - 1);
        count <<= 64 - width;
        count = (uint64_t)((int64_t)count >> (64 - width)); /* sign-extend to width */
        count += offset;
        __atomic_thread_fence(__ATOMIC_ACQUIRE);
    } while (pc->lock != seq);
    return count;
}

static inline uint64_t cpucycles(void) {
    return g_use_pmc ? cpucycles_pmc() : cpucycles_ref();
}

/* Pin the calling thread to one core (BENCH_CPU, default 0): required for the
 * per-core PMC, and good hygiene for the reference counter too. */
static void cycles_pin(void) {
    const char *e = getenv("BENCH_CPU");
    int cpu = (e && atoi(e) >= 0) ? atoi(e) : 0;
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    (void)sched_setaffinity(0, sizeof set, &set);
}

/* Open the core-cycles PMC for the calling thread. Returns 1 if rdpmc is usable. */
static int cycles_setup(void) {
    cycles_pin();
    const char *force = getenv("BENCH_CYCLES");
    if (force && strcmp(force, "tsc") == 0) { g_use_pmc = 0; return 0; }

    struct perf_event_attr attr;
    memset(&attr, 0, sizeof attr);
    attr.type = PERF_TYPE_HARDWARE;
    attr.size = sizeof attr;
    attr.config = PERF_COUNT_HW_CPU_CYCLES;
    attr.exclude_kernel = 1;   /* required when perf_event_paranoid >= 2 */
    attr.exclude_hv = 1;
    g_pmc_fd = (int)syscall(__NR_perf_event_open, &attr, 0, -1, -1, 0);
    if (g_pmc_fd < 0) { g_use_pmc = 0; return 0; }

    long pg = sysconf(_SC_PAGESIZE);
    void *m = mmap(NULL, (size_t)pg, PROT_READ, MAP_SHARED, g_pmc_fd, 0);
    if (m == MAP_FAILED) { close(g_pmc_fd); g_pmc_fd = -1; g_use_pmc = 0; return 0; }
    g_pmc_page = (struct perf_event_mmap_page *)m;

    if (!g_pmc_page->cap_user_rdpmc || !g_pmc_page->index) {
        munmap(m, (size_t)pg); close(g_pmc_fd);
        g_pmc_page = NULL; g_pmc_fd = -1; g_use_pmc = 0; return 0;
    }
    g_use_pmc = 1;
    return 1;
}

static void cycles_teardown(void) {
    if (g_pmc_page) { munmap(g_pmc_page, (size_t)sysconf(_SC_PAGESIZE)); g_pmc_page = NULL; }
    if (g_pmc_fd >= 0) { close(g_pmc_fd); g_pmc_fd = -1; }
    g_use_pmc = 0;
}

static const char *cycles_counter_name(void) {
    return g_use_pmc ? "rdpmc CPU_CYCLES (real core cycles, user-space)"
                     : "rdtsc (reference cycles, constant TSC rate)";
}
#else  /* non-x86_64 or non-Linux: reference counter only */
#define cpucycles cpucycles_ref
static int cycles_setup(void) { return 0; }
static void cycles_teardown(void) {}
static const char *cycles_counter_name(void) { return "reference counter (rdtsc / cntvct_el0)"; }
#endif

/* ---------- stats ---------- */

static int bench_cmp_u64(const void *a, const void *b) {
    uint64_t x = *(const uint64_t *)a;
    uint64_t y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}

static uint64_t bench_median(uint64_t *arr, int n) {
    qsort(arr, (size_t)n, sizeof(uint64_t), bench_cmp_u64);
    return (n & 1) ? arr[n / 2] : arr[n / 2 - 1] + (arr[n / 2] - arr[n / 2 - 1]) / 2;
}

/* ---------- iteration count + per-op timing ---------- */

#ifndef BENCH_ITER
#define BENCH_ITER 1000
#endif

/* Time one operation into cyc[i] (cycles) and ns[i] (wall-clock nanoseconds). */
#define BENCH_TIME_OP(cyc, ns, i, op) do {                                \
    struct timespec _ts0, _ts1;                                           \
    uint64_t _t0 = cpucycles();                                           \
    clock_gettime(CLOCK_MONOTONIC, &_ts0);                                \
    (op);                                                                 \
    uint64_t _t1 = cpucycles();                                           \
    clock_gettime(CLOCK_MONOTONIC, &_ts1);                                \
    (cyc)[i] = _t1 - _t0;                                                  \
    (ns)[i]  = (uint64_t)(_ts1.tv_sec  - _ts0.tv_sec)  * 1000000000ULL    \
             + (uint64_t)(_ts1.tv_nsec - _ts0.tv_nsec);                   \
} while (0)
