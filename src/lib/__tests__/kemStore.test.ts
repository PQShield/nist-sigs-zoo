import { describe, it, expect } from 'vitest';
import { buildUrlParams } from '$lib/kemStore';
import type { KemFilterState } from '$lib/types';

function makeDefaults(): KemFilterState {
	return {
		schemes: new Set(['ML-KEM', 'HQC', 'ECDH']),
		levels: new Set(['Pre-Quantum', 1, 2, 3, 4, 5] as const),
		sortCol: 'scheme',
		sortDir: 'asc',
	};
}

describe('buildUrlParams (KEM)', () => {
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

	it('encodes ct sort column', () => {
		const defaults = makeDefaults();
		const state = { ...defaults, sortCol: 'ct' as const };
		const params = buildUrlParams(state, defaults);
		expect(params.get('sort')).toBe('ct');
	});

	it('encodes timing sort columns', () => {
		const defaults = makeDefaults();
		for (const col of ['keygenUs', 'encapsUs', 'decapsUs'] as const) {
			const params = buildUrlParams({ ...defaults, sortCol: col }, defaults);
			expect(params.get('sort')).toBe(col);
		}
	});

	it('encodes non-default sortDir', () => {
		const defaults = makeDefaults();
		const state = { ...defaults, sortDir: 'desc' as const };
		const params = buildUrlParams(state, defaults);
		expect(params.get('dir')).toBe('desc');
	});

	it('encodes filtered scheme set', () => {
		const defaults = makeDefaults();
		const state = { ...defaults, schemes: new Set(['ML-KEM']) };
		const params = buildUrlParams(state, defaults);
		expect(params.get('s')).toBe('ML-KEM');
	});

	it('encodes filtered level set', () => {
		const defaults = makeDefaults();
		const state = { ...defaults, levels: new Set([1, 5] as const) };
		const params = buildUrlParams(state, defaults);
		expect(params.get('l')).toBe('1,5');
	});

	it('encodes Pre-Quantum level as PQ', () => {
		const defaults = makeDefaults();
		const state = { ...defaults, levels: new Set(['Pre-Quantum'] as const) };
		const params = buildUrlParams(state, defaults);
		expect(params.get('l')).toBe('PQ');
	});

	it('does not encode size params (none exist for KEMs)', () => {
		const defaults = makeDefaults();
		const params = buildUrlParams(defaults, defaults);
		expect(params.has('pkMin')).toBe(false);
		expect(params.has('ctMin')).toBe(false);
	});

	it('round-trips: modified state encodes', () => {
		const defaults = makeDefaults();
		const state = {
			...defaults,
			sortCol: 'pkPlusCt' as const,
			sortDir: 'desc' as const,
			schemes: new Set(['HQC']),
		};
		const params = buildUrlParams(state, defaults);
		expect(params.get('sort')).toBe('pkPlusCt');
		expect(params.get('dir')).toBe('desc');
		expect(params.get('s')).toBe('HQC');
	});
});
