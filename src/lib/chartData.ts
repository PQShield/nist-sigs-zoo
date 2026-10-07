import { fmtTime } from './format';
import { displayUs, kemTimes, sigTimes } from './timing';
import type { AxisField, KemAxisField, KemParameterSet, ParameterSet } from './types';

/** Chart value of a signature row on an axis; null when the row has no value there. */
export function sigAxisValue(row: ParameterSet, field: AxisField): number | null {
	switch (field) {
		case 'signingTime':
			return sigTimes(row).sign;
		case 'verificationTime':
			return sigTimes(row).verify;
		case 'signingCycles':
		case 'verificationCycles':
			return row[field] > 0 ? row[field] : null;
		default:
			return row[field];
	}
}

/** Chart value of a KEM row on an axis; null when the row has no value there. */
export function kemAxisValue(row: KemParameterSet, field: KemAxisField): number | null {
	switch (field) {
		case 'keygenTime':
			return kemTimes(row).keygen;
		case 'encapsTime':
			return kemTimes(row).encaps;
		case 'decapsTime':
			return kemTimes(row).decaps;
		case 'keygenCycles':
		case 'encapsCycles':
		case 'decapsCycles':
			return row[field] > 0 ? row[field] : null;
		default:
			return row[field];
	}
}

/** Split rows into those with a value on both axes and those left out of the chart. */
export function partitionPlottable<T, F>(
	rows: T[],
	valueOf: (row: T, field: F) => number | null,
	x: F,
	y: F
): { plotted: T[]; omitted: T[] } {
	const plotted: T[] = [];
	const omitted: T[] = [];
	for (const row of rows) {
		(valueOf(row, x) != null && valueOf(row, y) != null ? plotted : omitted).push(row);
	}
	return { plotted, omitted };
}

export function omittedNote(n: number): string {
	return n === 1
		? '1 parameter set is not shown: not yet benchmarked.'
		: `${n} parameter sets are not shown: not yet benchmarked.`;
}

/** Tooltip entries for operation times: formatted like the tables, marked when extrapolated from cycles. */
export function timeTooltip(
	ops: { label: string; us: number | null; cycles: number }[]
): Record<string, string> {
	const out: Record<string, string> = {};
	for (const { label, us, cycles } of ops) {
		const t = displayUs(us, cycles);
		if (t == null) continue;
		out[`${label} time`] = us == null ? `${fmtTime(t)} (estimated from cycles)` : fmtTime(t);
	}
	return out;
}
