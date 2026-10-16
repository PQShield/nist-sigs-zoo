/* PATCH (nist-sigs-zoo): unmodified copy of the submitted cpu_features.c. The submission keeps it one directory above the
 * yuanyang-<n> instance directories, but the build copies only the instance directory. */
#include "cpu_features.h"

#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#include <intrin.h>
#define YUANYANG_X86 1
#elif (defined(__GNUC__) || defined(__clang__)) \
	&& (defined(__i386__) || defined(__x86_64__))
#include <cpuid.h>
#define YUANYANG_X86 1
#else
#define YUANYANG_X86 0
#endif

static atomic_int cpu_init_state;
static yuanyang_cpu_info cpu_info;

#if YUANYANG_X86
static void
cpuid_leaf(unsigned leaf, unsigned subleaf, unsigned out[4])
{
#if defined(_MSC_VER)
	int x[4];

	__cpuidex(x, (int)leaf, (int)subleaf);
	out[0] = (unsigned)x[0];
	out[1] = (unsigned)x[1];
	out[2] = (unsigned)x[2];
	out[3] = (unsigned)x[3];
#else
	__cpuid_count(leaf, subleaf, out[0], out[1], out[2], out[3]);
#endif
}

static uint64_t
xgetbv0(void)
{
#if defined(_MSC_VER)
	return _xgetbv(0);
#else
	unsigned lo, hi;

	__asm__ volatile ("xgetbv" : "=a" (lo), "=d" (hi) : "c" (0));
	return ((uint64_t)hi << 32) | lo;
#endif
}
#endif

static void
detect_cpu(yuanyang_cpu_info *info)
{
	const char *override;

	memset(info, 0, sizeof *info);
#if YUANYANG_X86
	{
		unsigned r0[4], r1[4], r7[4];
		unsigned max_leaf;
		int osxsave;

		cpuid_leaf(0, 0, r0);
		max_leaf = r0[0];
		if (max_leaf >= 1) {
			cpuid_leaf(1, 0, r1);
			info->sse2 = (int)((r1[3] >> 26) & 1u);
			info->pclmul = (int)((r1[2] >> 1) & 1u);
			info->fma = (int)((r1[2] >> 12) & 1u);
			osxsave = (int)((r1[2] >> 27) & 1u);
			info->avx = (int)((r1[2] >> 28) & 1u);
			if (osxsave && info->avx) {
				info->os_avx_state = (xgetbv0() & 6u) == 6u;
			}
		}
		if (max_leaf >= 7) {
			cpuid_leaf(7, 0, r7);
			info->avx2 = (int)((r7[1] >> 5) & 1u);
			info->bmi2 = (int)((r7[1] >> 8) & 1u);
		}
		info->avx2 &= info->os_avx_state;
		info->fma &= info->os_avx_state;
	}
#endif

	override = getenv("YUANYANG_CPU");
	if (override == NULL || override[0] == '\0'
		|| strcmp(override, "auto") == 0)
	{
#if YUANYANG_HAVE_AVX2_IMPL
		if (info->avx2) {
			info->selected = YUANYANG_CPU_AVX2;
			return;
		}
#endif
		info->selected = info->sse2
			? YUANYANG_CPU_SSE2 : YUANYANG_CPU_GENERIC;
		return;
	}
	if (strcmp(override, "generic") == 0) {
		info->selected = YUANYANG_CPU_GENERIC;
		return;
	}
	if (strcmp(override, "sse2") == 0) {
		if (info->sse2) {
			info->selected = YUANYANG_CPU_SSE2;
		} else {
			info->selected = YUANYANG_CPU_GENERIC;
			info->override_rejected = 1;
		}
		return;
	}
	if (strcmp(override, "avx2") == 0) {
#if YUANYANG_HAVE_AVX2_IMPL
		if (info->avx2) {
			info->selected = YUANYANG_CPU_AVX2;
		} else
#endif
		{
			info->selected = info->sse2
				? YUANYANG_CPU_SSE2 : YUANYANG_CPU_GENERIC;
			info->override_rejected = 1;
		}
		return;
	}
	info->selected = YUANYANG_CPU_GENERIC;
	info->override_rejected = 1;
}

const yuanyang_cpu_info *
yuanyang_cpu_get_info(void)
{
	int state;

	state = atomic_load_explicit(&cpu_init_state, memory_order_acquire);
	if (state != 2) {
		int expected;

		expected = 0;
		if (atomic_compare_exchange_strong_explicit(&cpu_init_state,
			&expected, 1, memory_order_acq_rel, memory_order_acquire))
		{
			detect_cpu(&cpu_info);
			atomic_store_explicit(&cpu_init_state, 2, memory_order_release);
		} else {
			while (atomic_load_explicit(&cpu_init_state,
				memory_order_acquire) != 2)
			{
				/* Feature detection is short and runs only once. */
			}
		}
	}
	return &cpu_info;
}

yuanyang_cpu_path
yuanyang_cpu_get_path(void)
{
	return yuanyang_cpu_get_info()->selected;
}

const char *
yuanyang_cpu_path_name(yuanyang_cpu_path path)
{
	switch (path) {
	case YUANYANG_CPU_AVX2:
		return "avx2";
	case YUANYANG_CPU_SSE2:
		return "sse2";
	default:
		return "generic";
	}
}
