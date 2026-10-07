import { describe, expect, it } from 'vitest';
import { chartLegend, familyKey, FAMILY_SHAPES, levelColourScale, levelDescription, levelKey, STANDARDISED } from '$lib/chartLegend';

describe('levelKey', () => {
	it('names NIST levels and NGCC targets on their own scales', () => {
		expect(levelKey(3, null)).toBe('Level 3');
		expect(levelKey('Pre-Quantum', null)).toBe('Pre-quantum');
		expect(levelKey(null, 128)).toBe('128-bit');
		expect(levelKey(null, 128)).not.toMatch(/Level/);
	});
	it('describes levels for tooltips', () => {
		expect(levelDescription(1, null)).toBe('NIST level 1');
		expect(levelDescription(null, 256)).toBe('NGCC target: 256-bit classical');
	});
});

describe('levelColourScale', () => {
	it('keeps only present keys, NIST first', () => {
		expect(levelColourScale(['512-bit', 'Level 3', 'Level 1'], false)).toEqual({
			domain: ['Level 1', 'Level 3', '512-bit'],
			range: ['#1D4ED8', '#D97706', '#4C1D95']
		});
	});
	it('never gives an NGCC target a NIST level colour', () => {
		const nist = levelColourScale(['Pre-quantum', 'Level 1', 'Level 2', 'Level 3', 'Level 4', 'Level 5'], false).range;
		const ngcc = levelColourScale(['128-bit', '256-bit', '384-bit', '512-bit'], false).range;
		expect(ngcc.filter((c) => nist.includes(c))).toEqual([]);
		const nistDark = levelColourScale(['Level 1', 'Level 5'], true).range;
		const ngccDark = levelColourScale(['128-bit', '512-bit'], true).range;
		expect(ngccDark.filter((c) => nistDark.includes(c))).toEqual([]);
	});
});

describe('familyKey', () => {
	it('stars standards and pending standards', () => {
		expect(familyKey({ status: 'FIPS', category: 'Lattices' })).toBe(STANDARDISED);
		expect(familyKey({ status: 'To be standardized', category: 'Code-based' })).toBe(STANDARDISED);
	});
	it('maps unknown categories to Other', () => {
		expect(familyKey({ status: 'On-ramp', category: 'Weird' })).toBe('Other');
		expect(FAMILY_SHAPES[familyKey({ status: 'On-ramp', category: 'Lattices' })]).toBeDefined();
	});
});

describe('chartLegend', () => {
	it('adds an NGCC target group with its note only when NGCC points are shown', () => {
		const groups = chartLegend([
			{ family: 'Lattices', level: 'Level 1', security: 'ok' },
			{ family: 'Lattices', level: '128-bit', security: 'broken' }
		], false);
		expect(groups.map((g) => g.title)).toEqual(['Family', 'NIST category', 'NGCC target', 'Security']);
		const ngcc = groups.find((g) => g.title === 'NGCC target')!;
		expect(ngcc.items.map((i) => i.label)).toEqual(['128-bit classical']);
		expect(ngcc.note).toBe('Not comparable to NIST levels.');
		expect(groups.find((g) => g.title === 'Security')!.items.map((i) => i.label)).toEqual(['Broken by design']);
	});
	it('has no NGCC group without NGCC points', () => {
		const groups = chartLegend([{ family: STANDARDISED, level: 'Level 2', security: 'warning' }], false);
		expect(groups.map((g) => g.title)).toEqual(['Family', 'NIST category', 'Security']);
	});
	it('is empty for an empty chart', () => {
		expect(chartLegend([], true)).toEqual([]);
	});
});
