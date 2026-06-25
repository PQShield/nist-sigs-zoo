import { describe, expect, it } from 'vitest';
import {
	computeKemRanges,
	defaultKemFilter,
	filterKemRows,
	processKemSchemes
} from '$lib/kemData';
import type { KemSchemeYaml } from '$lib/types';

const mlkem: KemSchemeYaml = {
	name: 'ML-KEM',
	website: 'https://example.org/ml-kem',
	category: 'Lattices',
	assumption: 'MLWE',
	status: 'FIPS',
	parametersets: [
		{ name: 'ML-KEM-512', level: 1, pk: 800, ct: 768 },
		{ name: 'ML-KEM-768', level: 3, pk: 1184, ct: 1088 }
	]
};

const ecdh: KemSchemeYaml = {
	name: 'ECDH',
	website: 'https://example.org/ecdh',
	category: 'Pre-Quantum',
	assumption: 'Elliptic Curves',
	status: 'Classic cryptography',
	broken: 'classical',
	parametersets: [{ name: 'X25519', level: 'Pre-Quantum', pk: 32, ct: 32 }]
};

describe('processKemSchemes', () => {
	it('flattens parameter sets and computes pk+ct', () => {
		const { schemes, parameterSets } = processKemSchemes([mlkem]);
		expect(schemes).toHaveLength(1);
		expect(parameterSets).toHaveLength(2);

		const ps512 = parameterSets.find((p) => p.parameterset === 'ML-KEM-512')!;
		expect(ps512.pk).toBe(800);
		expect(ps512.ct).toBe(768);
		expect(ps512.pkPlusCt).toBe(1568);
		expect(ps512.scheme).toBe('ML-KEM');
		expect(ps512.category).toBe('Lattices');
	});

	it('marks pre-quantum schemes as classical via the broken flag', () => {
		const { schemes, parameterSets } = processKemSchemes([ecdh]);
		expect(schemes[0].classical).toBe(true);
		expect(parameterSets[0].classical).toBe(true);
		expect(parameterSets[0].broken).toBe('classical');
		expect(parameterSets[0].level).toBe('Pre-Quantum');
	});

	it('propagates scheme-level flags and allows per-parameter-set overrides', () => {
		const withFlags: KemSchemeYaml = {
			...mlkem,
			warning: 'scheme-wide warning',
			parametersets: [
				{ name: 'A', level: 1, pk: 1, ct: 1 },
				{ name: 'B', level: 1, pk: 2, ct: 2, warning: 'specific warning' }
			]
		};
		const { parameterSets } = processKemSchemes([withFlags]);
		expect(parameterSets.find((p) => p.parameterset === 'A')!.warning).toBe('scheme-wide warning');
		expect(parameterSets.find((p) => p.parameterset === 'B')!.warning).toBe('specific warning');
	});

	it('skips schemes with no parameter sets', () => {
		const empty: KemSchemeYaml = { ...mlkem, parametersets: [] };
		const { schemes, parameterSets } = processKemSchemes([empty]);
		expect(schemes).toHaveLength(0);
		expect(parameterSets).toHaveLength(0);
	});
});

describe('KEM filtering', () => {
	const { schemes, parameterSets } = processKemSchemes([mlkem, ecdh]);

	it('computeKemRanges spans all rows', () => {
		const ranges = computeKemRanges(parameterSets);
		expect(ranges.pk).toEqual([32, 1184]);
		expect(ranges.ct).toEqual([32, 1088]);
		expect(ranges.pkPlusCt).toEqual([64, 2272]);
	});

	it('computeKemRanges handles an empty dataset', () => {
		expect(computeKemRanges([])).toEqual({ pk: [0, 0], ct: [0, 0], pkPlusCt: [0, 0] });
	});

	it('default filter passes every row through', () => {
		const ranges = computeKemRanges(parameterSets);
		const filter = defaultKemFilter(ranges, schemes.map((s) => s.scheme));
		expect(filterKemRows(parameterSets, filter)).toHaveLength(parameterSets.length);
	});

	it('filters by scheme', () => {
		const ranges = computeKemRanges(parameterSets);
		const filter = defaultKemFilter(ranges, schemes.map((s) => s.scheme));
		filter.schemes = new Set(['ECDH']);
		const out = filterKemRows(parameterSets, filter);
		expect(out.every((r) => r.scheme === 'ECDH')).toBe(true);
		expect(out).toHaveLength(1);
	});

	it('filters by level', () => {
		const ranges = computeKemRanges(parameterSets);
		const filter = defaultKemFilter(ranges, schemes.map((s) => s.scheme));
		filter.levels = new Set([1]);
		const out = filterKemRows(parameterSets, filter);
		expect(out.map((r) => r.parameterset)).toEqual(['ML-KEM-512']);
	});

	it('filters by ciphertext size bounds', () => {
		const ranges = computeKemRanges(parameterSets);
		const filter = defaultKemFilter(ranges, schemes.map((s) => s.scheme));
		filter.maxCt = 100;
		const out = filterKemRows(parameterSets, filter);
		expect(out.map((r) => r.parameterset)).toEqual(['X25519']);
	});
});
