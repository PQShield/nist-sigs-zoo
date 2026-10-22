export const CPUSPEED = 2_500_000_000;

/** Cycles per microsecond at the reference clock (for cycles ↔ µs extrapolation). */
export const CYCLES_PER_US = CPUSPEED / 1_000_000;

/** Human-readable reference clock, e.g. "2.5 GHz". */
export const CPU_GHZ_LABEL = `${CPUSPEED / 1_000_000_000} GHz`;

/** Shown wherever wall-clock times (µs) appear. */
export const TIMING_VARIABILITY_NOTE =
	'Wall-clock times vary between runs and machines (CPU frequency scaling, other tenants on cloud VMs); cycle counts are the steadier basis for comparison.';

export const NIST_LEVELS = [1, 2, 3, 4, 5] as const;

export const ALL_LEVELS = ['Pre-Quantum', 1, 2, 3, 4, 5] as const;

/** China NGCC claimed classical security targets, in bits. Never mapped onto NIST levels. */
export const NGCC_LEVELS = [128, 256, 384, 512] as const;

/** Display name of the site. Plan 5 (rename) changes it here only. */
export const SITE_NAME = 'NIST Signatures Zoo';
