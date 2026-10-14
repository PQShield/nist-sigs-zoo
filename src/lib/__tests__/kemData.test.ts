import { describe, expect, it } from 'vitest';
import { computeKemRanges, processKemSchemes } from '$lib/kemData';
import { decodeCompareFilter, defaultCompareFilter, encodeCompareFilter, filterCompareRows } from '$lib/compare';
import { KEM_FILTER } from '$lib/primitives';
import { presentProgrammeGroups } from '$lib/compareFilters';
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

	it('carries scheme-level programmes onto schemes and parameter sets', () => {
		const data = [{ ...mlkem, name: 'FrodoKEM', programmes: { 'nist-pqc': 'round-3', iso: 'in-progress' } }];
		const { schemes, parameterSets } = processKemSchemes(data);
		const expected = [{ programme: 'nist-pqc', stage: 'round-3' }, { programme: 'iso', stage: 'in-progress' }];
		expect(schemes[0].programmes).toEqual(expected);
		expect(parameterSets.every((p) => JSON.stringify(p.programmes) === JSON.stringify(expected))).toBe(true);
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
		expect(computeKemRanges([])).toEqual({
			pk: [0, 0], ct: [0, 0], pkPlusCt: [0, 0], keygenUs: null, encapsUs: null, decapsUs: null
		});
	});

	it('default filter passes every row through', () => {
		const ranges = computeKemRanges(parameterSets);
		const filter = defaultCompareFilter(KEM_FILTER, ranges, schemes.map((s) => s.scheme), presentProgrammeGroups(parameterSets));
		expect(filterCompareRows(KEM_FILTER, parameterSets, filter)).toHaveLength(parameterSets.length);
	});

	it('filters by scheme', () => {
		const ranges = computeKemRanges(parameterSets);
		const filter = defaultCompareFilter(KEM_FILTER, ranges, schemes.map((s) => s.scheme), presentProgrammeGroups(parameterSets));
		filter.schemes = new Set(['ECDH']);
		const out = filterCompareRows(KEM_FILTER, parameterSets, filter);
		expect(out.every((r) => r.scheme === 'ECDH')).toBe(true);
		expect(out).toHaveLength(1);
	});

	it('filters by level', () => {
		const ranges = computeKemRanges(parameterSets);
		const filter = defaultCompareFilter(KEM_FILTER, ranges, schemes.map((s) => s.scheme), presentProgrammeGroups(parameterSets));
		filter.levels = new Set([1]);
		const out = filterCompareRows(KEM_FILTER, parameterSets, filter);
		expect(out.map((r) => r.parameterset)).toEqual(['ML-KEM-512']);
	});

	it('filters by ciphertext size bounds', () => {
		const ranges = computeKemRanges(parameterSets);
		const filter = defaultCompareFilter(KEM_FILTER, ranges, schemes.map((s) => s.scheme), presentProgrammeGroups(parameterSets));
		filter.maxCt = 100;
		const out = filterCompareRows(KEM_FILTER, parameterSets, filter);
		expect(out.map((r) => r.parameterset)).toEqual(['X25519']);
	});

	it('matches an NGCC-scale KEM against the NGCC targets only', () => {
		const ngcc = { ...mlkem, name: 'Aigis-Enc+', parametersets: [{ name: 'Aigis-enc1', ngcc_level: 128 as const, pk: 656, ct: 896 }] };
		const { schemes, parameterSets } = processKemSchemes([mlkem, ngcc]);
		const filter = defaultCompareFilter(KEM_FILTER, computeKemRanges(parameterSets), schemes.map((s) => s.scheme), presentProgrammeGroups(parameterSets));
		expect(filterCompareRows(KEM_FILTER, parameterSets, filter).map((r) => r.scheme)).toContain('Aigis-Enc+');
		expect(filterCompareRows(KEM_FILTER, parameterSets, { ...filter, ngccLevels: new Set() }).map((r) => r.scheme)).not.toContain('Aigis-Enc+');
		expect(filterCompareRows(KEM_FILTER, parameterSets, { ...filter, levels: new Set() }).map((r) => r.scheme)).toEqual(['Aigis-Enc+']);
	});
});

describe('KEM URL codec', () => {
	const { schemes, parameterSets } = processKemSchemes([mlkem, ecdh]);
	const ranges = computeKemRanges(parameterSets);
	const names = schemes.map((s) => s.scheme);
	const defaults = defaultCompareFilter(KEM_FILTER, ranges, names, presentProgrammeGroups(parameterSets));

	it('encodes nothing when state equals defaults', () => {
		expect(encodeCompareFilter(KEM_FILTER, defaultCompareFilter(KEM_FILTER, ranges, names, presentProgrammeGroups(parameterSets)), defaults).toString()).toBe('');
	});

	it('round-trips a modified filter through encode → decode', () => {
		const modified = defaultCompareFilter(KEM_FILTER, ranges, names, presentProgrammeGroups(parameterSets));
		modified.schemes = new Set(['ECDH']);
		modified.levels = new Set(['Pre-Quantum', 3]);
		modified.maxCt = 100;
		modified.minPk = 64;

		const params = encodeCompareFilter(KEM_FILTER, modified, defaults);
		expect(params.get('s')).toBe('ECDH');
		expect(params.get('l')).toBe('PQ,3');
		expect(params.get('ctMax')).toBe('100');
		expect(params.get('pkMin')).toBe('64');

		const restored = decodeCompareFilter(KEM_FILTER, defaults, params);
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
		decodeCompareFilter(KEM_FILTER, defaults, params);
		expect(defaults.schemes.has('ML-KEM')).toBe(true);
	});
});

describe('KEM timings', () => {
	const timed: KemSchemeYaml = {
		...mlkem,
		parametersets: [
			{ name: 'ML-KEM-512', level: 1, pk: 800, ct: 768, encaps_us: 10 },
			{ name: 'ML-KEM-768', level: 3, pk: 1184, ct: 1088, encaps_cycles: 50_000 }
		]
	};
	const { schemes, parameterSets } = processKemSchemes([timed, ecdh]);
	const ranges = computeKemRanges(parameterSets);
	const defaults = defaultCompareFilter(KEM_FILTER, ranges, schemes.map((s) => s.scheme), presentProgrammeGroups(parameterSets));

	it('computes µs ranges over measured and extrapolated times', () => {
		expect(ranges.encapsUs).toEqual([10, 20]);
		expect(ranges.keygenUs).toBeNull();
		expect([defaults.minEncapsUs, defaults.maxEncapsUs]).toEqual([10, 20]);
		expect([defaults.minKeygenUs, defaults.maxKeygenUs]).toEqual([0, Infinity]);
	});

	it('filters by µs and keeps rows that have not been benchmarked', () => {
		const out = filterCompareRows(KEM_FILTER, parameterSets, { ...defaults, maxEncapsUs: 15 });
		expect(out.map((r) => r.parameterset)).toEqual(['ML-KEM-512', 'X25519']);
	});

	it('round-trips µs bounds and common keys through the URL', () => {
		const state = { ...defaults, maxEncapsUs: 15, minDecapsUs: 1, hideWarnings: true, chart: 'speed' as const };
		const p = encodeCompareFilter(KEM_FILTER, state, defaults);
		expect(p.get('euMax')).toBe('15');
		expect(p.get('duMin')).toBe('1');
		expect(p.get('hw')).toBe('1');
		expect(p.get('v')).toBe('speed');
		const back = decodeCompareFilter(KEM_FILTER, defaults, p);
		expect(back.maxEncapsUs).toBe(15);
		expect(back.minDecapsUs).toBe(1);
		expect(back.hideWarnings).toBe(true);
		expect(back.chart).toBe('speed');
	});

	it('shows only recommended parameter sets by default', () => {
		const flagged: KemSchemeYaml = { ...mlkem, parametersets: [{ ...mlkem.parametersets[0], recommended: true }, mlkem.parametersets[1]] };
		const rows = processKemSchemes([flagged]).parameterSets;
		const f = defaultCompareFilter(KEM_FILTER, computeKemRanges(rows), ['ML-KEM'], presentProgrammeGroups(rows));
		expect(filterCompareRows(KEM_FILTER, rows, f).map((r) => r.parameterset)).toEqual(['ML-KEM-512']);
		expect(filterCompareRows(KEM_FILTER, rows, { ...f, paramSets: 'all' })).toHaveLength(2);
	});
});
