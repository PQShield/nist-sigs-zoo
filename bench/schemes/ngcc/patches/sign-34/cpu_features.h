/* PATCH (nist-sigs-zoo): unmodified copy of the submitted cpu_features.h. The submission keeps it one directory above the
 * yuanyang-<n> instance directories, but the build copies only the instance directory. */
#ifndef YUANYANG_CPU_FEATURES_H
#define YUANYANG_CPU_FEATURES_H

typedef enum {
	YUANYANG_CPU_GENERIC = 0,
	YUANYANG_CPU_SSE2 = 1,
	YUANYANG_CPU_AVX2 = 2
} yuanyang_cpu_path;

typedef struct {
	int sse2;
	int avx;
	int avx2;
	int bmi2;
	int pclmul;
	int fma;
	int os_avx_state;
	int override_rejected;
	yuanyang_cpu_path selected;
} yuanyang_cpu_info;

/* Thread-safe, process-wide feature detection and path selection. */
const yuanyang_cpu_info *yuanyang_cpu_get_info(void);
yuanyang_cpu_path yuanyang_cpu_get_path(void);
const char *yuanyang_cpu_path_name(yuanyang_cpu_path path);

#endif
