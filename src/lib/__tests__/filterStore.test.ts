import { describe, it, expect } from 'vitest';
import { buildUrlParams } from '$lib/filterStore';
import type { FilterState, DataRanges } from '$lib/types';

function makeDefaults(): FilterState {
	return {
		schemes: new Set(['SchemeA', 'SchemeB']),
		levels: new Set(['Pre-Quantum', 1, 2, 3, 4, 5] as const),
		minPk: 100, maxPk: 50000,
		minSig: 200, maxSig: 80000,
		minPkPlusSig: 300, maxPkPlusSig: 130000,
		minSigningCycles: 10000, maxSigningCycles: 5000000,
		minVerificationCycles: 5000, maxVerificationCycles: 1000000,
		minSigningUs: 0, maxSigningUs: Infinity,
		minVerificationUs: 0, maxVerificationUs: Infinity,
		sortCol: 'scheme',
		sortDir: 'asc',
	};
}

describe('buildUrlParams', () => {
	it('returns empty params for default state', () => {
		const defaults = makeDefaults();
		const params = buildUrlParams(defaults, defaults);
		expect(params.toString()).toBe('');
	});

	it('encodes non-default sortCol', () => {
		const defaults = makeDefaults();
		const state = { ...defaults, sortCol: 'pk' as const };
		const params = buildUrlParams(state, defaults);
		expect(params.get('sort')).toBe('pk');
	});

	it('encodes non-default sortDir', () => {
		const defaults = makeDefaults();
		const state = { ...defaults, sortDir: 'desc' as const };
		const params = buildUrlParams(state, defaults);
		expect(params.get('dir')).toBe('desc');
	});

	it('encodes filtered scheme set', () => {
		const defaults = makeDefaults();
		const state = { ...defaults, schemes: new Set(['SchemeA']) };
		const params = buildUrlParams(state, defaults);
		expect(params.get('s')).toBe('SchemeA');
	});

	it('encodes filtered level set', () => {
		const defaults = makeDefaults();
		const state = { ...defaults, levels: new Set([1, 3] as const) };
		const params = buildUrlParams(state, defaults);
		expect(params.get('l')).toBe('1,3');
	});

	it('encodes Pre-Quantum level as PQ', () => {
		const defaults = makeDefaults();
		const state = { ...defaults, levels: new Set(['Pre-Quantum'] as const) };
		const params = buildUrlParams(state, defaults);
		expect(params.get('l')).toBe('PQ');
	});

	it('encodes pk range when changed', () => {
		const defaults = makeDefaults();
		const state = { ...defaults, minPk: 500, maxPk: 10000 };
		const params = buildUrlParams(state, defaults);
		expect(params.get('pkMin')).toBe('500');
		expect(params.get('pkMax')).toBe('10000');
	});

	it('does not encode pk range when at defaults', () => {
		const defaults = makeDefaults();
		const params = buildUrlParams(defaults, defaults);
		expect(params.has('pkMin')).toBe(false);
		expect(params.has('pkMax')).toBe(false);
	});

	it('encodes sig range when changed', () => {
		const defaults = makeDefaults();
		const state = { ...defaults, minSig: 1000 };
		const params = buildUrlParams(state, defaults);
		expect(params.get('sigMin')).toBe('1000');
		expect(params.has('sigMax')).toBe(false);
	});

	it('round-trips: modified state encodes and back', () => {
		const defaults = makeDefaults();
		const state = { ...defaults, sortCol: 'sig' as const, sortDir: 'desc' as const, minPk: 300 };
		const params = buildUrlParams(state, defaults);
		expect(params.get('sort')).toBe('sig');
		expect(params.get('dir')).toBe('desc');
		expect(params.get('pkMin')).toBe('300');
	});
});
