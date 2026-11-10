import { CYCLES_PER_US } from './constants';

/** Displayed time in µs: the measured value, else extrapolated from cycles, else null ("not yet benchmarked"). */
export function displayUs(us: number | null, cycles: number): number | null {
	if (us != null) return us;
	return cycles > 0 ? cycles / CYCLES_PER_US : null;
}

export function sigTimes(r: {
	signingUs: number | null;
	signingCycles: number;
	verificationUs: number | null;
	verificationCycles: number;
}): { sign: number | null; verify: number | null } {
	return {
		sign: displayUs(r.signingUs, r.signingCycles),
		verify: displayUs(r.verificationUs, r.verificationCycles)
	};
}

export function kemTimes(r: {
	keygenUs: number | null;
	keygenCycles: number;
	encapsUs: number | null;
	encapsCycles: number;
	decapsUs: number | null;
	decapsCycles: number;
}): { keygen: number | null; encaps: number | null; decaps: number | null } {
	return {
		keygen: displayUs(r.keygenUs, r.keygenCycles),
		encaps: displayUs(r.encapsUs, r.encapsCycles),
		decaps: displayUs(r.decapsUs, r.decapsCycles)
	};
}

/** Min/max of the known times, rounded outwards to 0.1 µs so filter inputs show short numbers. */
export function timeRange(values: (number | null)[]): [number, number] | null {
	const known = values.filter((v): v is number => v != null);
	if (known.length === 0) return null;
	return [Math.floor(Math.min(...known) * 10) / 10, Math.ceil(Math.max(...known) * 10) / 10];
}

/** Reasons that mean "too slow for our limits": the run timeout, the per-operation limit, and
 * parameter sets skipped because a single run takes hours (written as `timeout`). */
const TIME_LIMIT = /^(?:timeout|(?:operation|sign-or-verify)-over-\d+s)$/;

/** How the timing cells of a parameter set whose benchmark did not finish read: "timeout" for
 * time limits, "DNF" otherwise; the title gives the exact reason. */
export function dnfCell(reason: string): { text: 'timeout' | 'DNF'; title: string } {
	return {
		text: TIME_LIMIT.test(reason) ? 'timeout' : 'DNF',
		title: `Benchmark did not finish (${reason.replace(/-/g, ' ')})`
	};
}

/** How the timing cells of a parameter set without any implementation (`no_code`) read: "no code",
 * with the reason as the title. Takes precedence over "DNF" and "not yet benchmarked". */
export function noCodeCell(reason: string): { text: 'no code'; title: string } {
	return { text: 'no code', title: reason };
}
