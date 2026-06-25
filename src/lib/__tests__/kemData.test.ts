import { describe, expect, it } from 'vitest';
import {
	applyKemUrlParams,
	buildKemUrlParams,
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

	it('carries benchmark fields and extrapolates cycles from µs when absent', () => {
		const withPerf: KemSchemeYaml = {
			...mlkem,
			parametersets: [
				// full cycles + us present → used directly
				{
					name: 'A',
					level: 1,
					pk: 1,
					ct: 1,
					keygen_cycles: 11049,
					encaps_cycles: 11813,
					decaps_cycles: 12288,
					keygen_us: 7.4,
					encaps_us: 7.9,
					decaps_us: 8.2
				},
				// only us → cycles extrapolated at 2.5 GHz; no perf → 0/null
				{ name: 'B', level: 1, pk: 1, ct: 1, decaps_us: 100 }
			]
		};
		const { parameterSets } = processKemSchemes([withPerf]);
		const a = parameterSets.find((p) => p.parameterset === 'A')!;
		expect(a.decapsCycles).toBe(12288);
		expect(a.decapsUs).toBe(8.2);

		const b = parameterSets.find((p) => p.parameterset === 'B')!;
		expect(b.keygenUs).toBeNull();
		expect(b.keygenCycles).toBe(0);
		expect(b.decapsUs).toBe(100);
		expect(b.decapsCycles).toBe(250000); // round(2.5e9 * 100 / 1e6)
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

describe('KEM URL codec', () => {
	const { schemes, parameterSets } = processKemSchemes([mlkem, ecdh]);
	const ranges = computeKemRanges(parameterSets);
	const names = schemes.map((s) => s.scheme);
	const defaults = defaultKemFilter(ranges, names);

	it('encodes nothing when state equals defaults', () => {
		expect(buildKemUrlParams(defaultKemFilter(ranges, names), defaults).toString()).toBe('');
	});

	it('round-trips a modified filter through encode → decode', () => {
		const modified = defaultKemFilter(ranges, names);
		modified.schemes = new Set(['ECDH']);
		modified.levels = new Set(['Pre-Quantum', 3]);
		modified.maxCt = 100;
		modified.minPk = 64;

		const params = buildKemUrlParams(modified, defaults);
		expect(params.get('s')).toBe('ECDH');
		expect(params.get('l')).toBe('PQ,3');
		expect(params.get('ctMax')).toBe('100');
		expect(params.get('pkMin')).toBe('64');

		const restored = applyKemUrlParams(defaults, params);
		expect([...restored.schemes]).toEqual(['ECDH']);
		expect(restored.levels.size).toBe(2);
		expect(restored.levels.has(3)).toBe(true);
		expect(restored.levels.has('Pre-Quantum')).toBe(true);
		expect(restored.maxCt).toBe(100);
		expect(restored.minPk).toBe(64);
		// untouched fields keep their defaults
		expect(restored.maxPk).toBe(defaults.maxPk);
	});

	it('decode leaves the defaults object unmutated', () => {
		const params = new URLSearchParams('s=ECDH');
		applyKemUrlParams(defaults, params);
		expect(defaults.schemes.has('ML-KEM')).toBe(true);
	});
});
