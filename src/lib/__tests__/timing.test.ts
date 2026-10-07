import { describe, expect, it } from 'vitest';
import { displayUs, kemTimes, sigTimes, timeRange } from '$lib/timing';
import { CYCLES_PER_US } from '$lib/constants';

describe('displayUs', () => {
	it('prefers a measured time', () => {
		expect(displayUs(12.5, 99_999)).toBe(12.5);
	});
	it('extrapolates from cycles at the reference clock', () => {
		expect(displayUs(null, 25_000)).toBe(25_000 / CYCLES_PER_US);
	});
	it('is null when there is no timing at all', () => {
		expect(displayUs(null, 0)).toBeNull();
	});
});

describe('sigTimes / kemTimes', () => {
	it('derive every displayed time of a row', () => {
		expect(sigTimes({ signingUs: null, signingCycles: 2500, verificationUs: 3, verificationCycles: 0 })).toEqual({ sign: 1, verify: 3 });
		expect(kemTimes({ keygenUs: null, keygenCycles: 0, encapsUs: 2, encapsCycles: 0, decapsUs: null, decapsCycles: 5000 })).toEqual({ keygen: null, encaps: 2, decaps: 2 });
	});
});

describe('timeRange', () => {
	it('spans the values, rounded outwards to 0.1 µs', () => {
		expect(timeRange([13.42, null, 8.0064])).toEqual([8, 13.5]);
	});
	it('is null without any timing', () => {
		expect(timeRange([null, null])).toBeNull();
	});
});
