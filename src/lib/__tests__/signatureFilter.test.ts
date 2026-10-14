import { describe, it, expect } from 'vitest';
import {
	compareRows,
	decodeCompareFilter,
	defaultCompareFilter,
	encodeCompareFilter,
	filterCompareRows,
	sortCompareRows
} from '$lib/compare';
import { processYamlSchemes } from '$lib/data';
import { defaultCommonFilter, presentProgrammeGroups } from '$lib/compareFilters';
import { SIGNATURE_FILTER } from '$lib/primitives';
import type { DataRanges, FilterState, ParameterSet, ParameterSetYaml, SchemeYaml } from '$lib/types';

const buildUrlParams = (state: FilterState, defaults: FilterState) => encodeCompareFilter(SIGNATURE_FILTER, state, defaults);

/** The signatures page's filter for `params` over these rows, and the rows it shows (sorted like the table). */
function signaturePage(parameterSets: ParameterSet[], ranges: DataRanges, params: string) {
	const defaults = defaultCompareFilter(
		SIGNATURE_FILTER,
		ranges,
		parameterSets.map((r) => r.scheme),
		presentProgrammeGroups(parameterSets)
	);
	const filter = decodeCompareFilter(SIGNATURE_FILTER, defaults, new URLSearchParams(params));
	const shown = (f: FilterState) =>
		sortCompareRows(SIGNATURE_FILTER, filterCompareRows(SIGNATURE_FILTER, parameterSets, f), f.sortCol, f.sortDir);
	return { defaults, filter, shown };
}

function makeDefaults(): FilterState {
	return {
		...defaultCommonFilter(['nist-onramp', 'pre-quantum']),
		schemes: new Set(['SchemeA', 'SchemeB']),
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

describe('NGCC-scale rows and the NIST level filter', () => {
	it('match NGCC targets only, never a NIST level', () => {
		const data: SchemeYaml[] = [
			{ name: 'Nist', website: '', category: 'Lattices', assumption: '', versions: [{ version: 'v1', date: '2024-01-01', status: 'On-ramp', parametersets: [{ name: 'L1', level: 1, pk: 100, sig: 200, signing_cycles: 1000, verification_cycles: 1000 }] }] },
			{ name: 'Ngcc', website: '', category: 'Lattices', assumption: '', versions: [{ version: 'R1', date: '2026-09-20', status: 'NGCC', parametersets: [{ name: 'X-128', ngcc_level: 128, pk: 100, sig: 200, signing_cycles: 1000, verification_cycles: 1000 }] }] }
		];
		const { parameterSets, ranges } = processYamlSchemes(data);
		const { filter, shown } = signaturePage(parameterSets, ranges, '');
		expect(shown(filter).map((r) => r.scheme)).toEqual(['Ngcc', 'Nist']);
		expect(shown({ ...filter, ngccLevels: new Set() }).map((r) => r.scheme)).toEqual(['Nist']);
		expect(shown({ ...filter, levels: new Set(), ngccLevels: new Set([128 as const]) }).map((r) => r.scheme)).toEqual(['Ngcc']);
	});

	it('sort by level keeps NGCC-scale rows last without throwing', () => {
		const data: SchemeYaml[] = [
			{ name: 'A', website: '', category: 'Lattices', assumption: '', versions: [{ version: 'v', date: '2026-01-01', status: 'x', parametersets: [
				{ name: 'n', ngcc_level: 256, pk: 1, sig: 1, signing_cycles: 1, verification_cycles: 1 },
				{ name: 'l', level: 3, pk: 1, sig: 1, signing_cycles: 1, verification_cycles: 1 }
			] }] }
		];
		const { parameterSets } = processYamlSchemes(data);
		const sorted = [...parameterSets].sort((a, b) => compareRows(SIGNATURE_FILTER, a, b, 'level', 'asc'));
		expect(sorted.map((r) => r.parameterset)).toEqual(['l', 'n']);
	});
});

describe('programme, parameter-set and status filters', () => {
	const ps = (name: string, extra: Partial<ParameterSetYaml> = {}): ParameterSetYaml => ({
		name, level: 1, pk: 10, sig: 20, signing_cycles: 100, verification_cycles: 100, ...extra
	});
	const DATA: SchemeYaml[] = [
		{ name: 'OnRamp', website: '', category: 'Lattices', assumption: '', versions: [{ version: 'v1', date: '2025-01-01', status: 'On-ramp', programmes: { 'nist-onramp': 'round-2' }, parametersets: [ps('fast', { recommended: true }), ps('small')] }] },
		{ name: 'Broken', website: '', category: 'Lattices', assumption: '', versions: [{ version: 'v1', date: '2025-01-01', status: 'KpqC', programmes: { kpqc: 'round-2' }, broken: 'key recovery', parametersets: [ps('b')] }] },
		{ name: 'Buggy', website: '', category: 'Lattices', assumption: '', versions: [{ version: 'v1', date: '2025-01-01', status: 'KpqC', programmes: { kpqc: 'round-1' }, implementation: 'buffer overflow', parametersets: [ps('x')] }] }
	];
	const { parameterSets, ranges } = processYamlSchemes(DATA);
	const shown = (params: string) => {
		const { filter, shown } = signaturePage(parameterSets, ranges, params);
		return shown(filter).map((r) => `${r.scheme}/${r.parameterset}`);
	};

	it('shows recommended parameter sets of every programme by default', () => {
		expect(shown('')).toEqual(['Broken/b', 'Buggy/x', 'OnRamp/fast']);
	});

	it('shows every parameter set with ps=all', () => {
		expect(shown('ps=all')).toContain('OnRamp/small');
	});

	it('filters by programme from the URL', () => {
		expect(shown('p=nist-onramp')).toEqual(['OnRamp/fast']);
	});

	it('hides design breaks and implementation findings independently', () => {
		expect(shown('hb=1')).toEqual(['Buggy/x', 'OnRamp/fast']);
		expect(shown('hi=1')).toEqual(['Broken/b', 'OnRamp/fast']);
	});

	it('round-trips the new keys through buildUrlParams', () => {
		const { filter, defaults } = signaturePage(parameterSets, ranges, 'p=kpqc&ps=all&hw=1&v=speed');
		const p = buildUrlParams(filter, defaults);
		expect(p.get('p')).toBe('kpqc');
		expect(p.get('ps')).toBe('all');
		expect(p.get('hw')).toBe('1');
		expect(p.get('v')).toBe('speed');
	});
});

describe('cycle filters and unbenchmarked rows', () => {
	const ps = (name: string, cycles: number | undefined): ParameterSetYaml => ({
		name, level: 1, pk: 10, sig: 20, signing_cycles: cycles, verification_cycles: cycles
	});
	const DATA: SchemeYaml[] = [
		{ name: 'Fast', website: '', category: 'Lattices', assumption: '', versions: [{ version: 'v1', date: '2025-01-01', status: 'On-ramp', parametersets: [ps('a', 1000), ps('b', 9000)] }] },
		{ name: 'Unbenched', website: '', category: 'Lattices', assumption: '', versions: [{ version: 'v1', date: '2025-01-01', status: 'On-ramp', parametersets: [ps('u', undefined)] }] }
	];
	const { parameterSets, ranges } = processYamlSchemes(DATA);
	const shown = (params: string) => {
		const { filter, shown } = signaturePage(parameterSets, ranges, params);
		return shown(filter).map((r) => `${r.scheme}/${r.parameterset}`);
	};

	it('keeps unbenchmarked rows by default', () => {
		expect(shown('ps=all')).toContain('Unbenched/u');
	});
	it('still applies cycle bounds to benchmarked rows, keeping unbenchmarked ones', () => {
		const rows = shown('ps=all&scMin=5000');
		expect(rows).toContain('Unbenched/u');
		expect(rows).toContain('Fast/b');
		expect(rows).not.toContain('Fast/a');
	});
});
