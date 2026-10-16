import { describe, expect, it } from 'vitest';
import {
	decodeCommonFilters,
	defaultCommonFilter,
	encodeCommonFilters,
	filterSummary,
	listedSchemeCount,
	passesCommonFilters,
	presentProgrammeGroups,
	rowProgrammeGroups,
	type FilterableRow
} from '$lib/compareFilters';
import type { CommonFilter, NgccLevel, NistLevel, ProgrammeGroup } from '$lib/types';

function row(overrides: Partial<FilterableRow> = {}): FilterableRow {
	return {
		programmes: [],
		classical: false,
		broken: false,
		warning: false,
		implementation: false,
		recommended: true,
		level: 1,
		ngccLevel: null,
		...overrides
	};
}

const GROUPS: ProgrammeGroup[] = ['nist-standards', 'nist-onramp', 'china-ngcc', 'kpqc', 'research', 'pre-quantum'];
const ALL = defaultCommonFilter(GROUPS);

describe('rowProgrammeGroups', () => {
	it('splits NIST PQC into standards and earlier rounds', () => {
		expect(rowProgrammeGroups(row({ programmes: [{ programme: 'nist-pqc', stage: 'standardised' }] }))).toEqual([
			'nist-standards'
		]);
		expect(rowProgrammeGroups(row({ programmes: [{ programme: 'nist-pqc', stage: 'selected' }] }))).toEqual([
			'nist-standards'
		]);
		expect(rowProgrammeGroups(row({ programmes: [{ programme: 'nist-pqc', stage: 'round-3' }] }))).toEqual([
			'nist-pqc-rounds'
		]);
	});

	it('lists every programme a scheme is in, in display order', () => {
		const r = row({
			programmes: [
				{ programme: 'kpqc', stage: 'selected' },
				{ programme: 'nist-onramp', stage: 'round-1' }
			]
		});
		expect(rowProgrammeGroups(r)).toEqual(['nist-onramp', 'kpqc']);
	});

	it('files schemes outside every programme under research, pre-quantum ones under pre-quantum', () => {
		expect(rowProgrammeGroups(row())).toEqual(['research']);
		expect(rowProgrammeGroups(row({ classical: true, broken: 'classical', level: 'Pre-Quantum' }))).toEqual([
			'pre-quantum'
		]);
	});
});

describe('presentProgrammeGroups', () => {
	it('returns the groups that occur, in display order', () => {
		const rows = [
			row({ classical: true, level: 'Pre-Quantum' }),
			row({ programmes: [{ programme: 'iso', stage: 'in-progress' }] }),
			row({ programmes: [{ programme: 'nist-pqc', stage: 'standardised' }] })
		];
		expect(presentProgrammeGroups(rows)).toEqual(['nist-standards', 'iso', 'pre-quantum']);
	});
});

describe('passesCommonFilters', () => {
	it('keeps a row when any of its programmes is selected', () => {
		const r = row({
			programmes: [
				{ programme: 'nist-onramp', stage: 'round-1' },
				{ programme: 'kpqc', stage: 'selected' }
			]
		});
		expect(passesCommonFilters(r, { ...ALL, programmes: new Set<ProgrammeGroup>(['kpqc']) })).toBe(true);
		expect(passesCommonFilters(r, { ...ALL, programmes: new Set<ProgrammeGroup>(['china-ngcc']) })).toBe(false);
	});

	it('matches NGCC targets only against the NGCC group, never a NIST level', () => {
		const ngcc = row({ level: null, ngccLevel: 128, programmes: [{ programme: 'china-ngcc', stage: 'round-1' }] });
		expect(passesCommonFilters(ngcc, ALL)).toBe(true);
		expect(passesCommonFilters(ngcc, { ...ALL, ngccLevels: new Set() })).toBe(false);
		expect(passesCommonFilters(ngcc, { ...ALL, levels: new Set(), ngccLevels: new Set<NgccLevel>([128]) })).toBe(true);
		expect(
			passesCommonFilters(row({ level: 1 }), { ...ALL, levels: new Set(), ngccLevels: new Set<NgccLevel>([128]) })
		).toBe(false);
	});

	it('drops a row that states no level on either scale', () => {
		expect(passesCommonFilters(row({ level: null, ngccLevel: null }), ALL)).toBe(false);
	});

	it('shows only recommended parameter sets by default, all on request', () => {
		const extra = row({ recommended: false });
		expect(passesCommonFilters(extra, ALL)).toBe(false);
		expect(passesCommonFilters(extra, { ...ALL, paramSets: 'all' })).toBe(true);
	});

	it('hides design breaks but never pre-quantum schemes with "Hide broken schemes"', () => {
		expect(passesCommonFilters(row({ broken: 'key recovery' }), { ...ALL, hideBroken: true })).toBe(false);
		const ecdsa = row({ broken: 'classical', classical: true, level: 'Pre-Quantum' });
		expect(passesCommonFilters(ecdsa, { ...ALL, hideBroken: true })).toBe(true);
	});

	it('hides implementation findings separately from design breaks', () => {
		const buggy = row({ implementation: 'stack overflow in verify' });
		expect(passesCommonFilters(buggy, { ...ALL, hideBroken: true })).toBe(true);
		expect(passesCommonFilters(buggy, { ...ALL, hideImplementation: true })).toBe(false);
	});

	it('hides warnings', () => {
		expect(passesCommonFilters(row({ warning: 'weaker than claimed' }), { ...ALL, hideWarnings: true })).toBe(false);
	});
});

describe('common URL codec', () => {
	function encode(state: CommonFilter): URLSearchParams {
		const p = new URLSearchParams();
		encodeCommonFilters(state, ALL, p);
		return p;
	}

	it('encodes nothing for the defaults', () => {
		expect(encode(ALL).toString()).toBe('');
	});

	it('round-trips every field', () => {
		const state: CommonFilter = {
			programmes: new Set<ProgrammeGroup>(['kpqc', 'nist-onramp']),
			levels: new Set<NistLevel>([1, 3]),
			ngccLevels: new Set<NgccLevel>([256]),
			paramSets: 'all',
			hideBroken: true,
			hideImplementation: true,
			hideWarnings: true,
			chart: 'speed'
		};
		const p = encode(state);
		expect(p.get('p')).toBe('nist-onramp,kpqc');
		expect(p.get('l')).toBe('1,3');
		expect(p.get('nl')).toBe('256');
		expect(p.get('ps')).toBe('all');
		expect([p.get('hb'), p.get('hi'), p.get('hw')]).toEqual(['1', '1', '1']);
		expect(p.get('v')).toBe('speed');
		expect(decodeCommonFilters(ALL, p)).toEqual(state);
	});

	it('reads an old level-only link as "no NGCC targets"', () => {
		const s = decodeCommonFilters(ALL, new URLSearchParams('l=1,2'));
		expect([...s.levels]).toEqual([1, 2]);
		expect(s.ngccLevels.size).toBe(0);
	});

	it('always sends nl with a changed NIST level list, so the pair round-trips', () => {
		const p = encode({ ...ALL, levels: new Set<NistLevel>([1]) });
		expect(p.get('nl')).toBe('128,256,384,512');
		expect(decodeCommonFilters(ALL, p).ngccLevels.size).toBe(4);
	});

	it('encodes Pre-Quantum as PQ', () => {
		expect(encode({ ...ALL, levels: new Set<NistLevel>(['Pre-Quantum']) }).get('l')).toBe('PQ');
	});

	it('ignores unknown values', () => {
		const s = decodeCommonFilters(ALL, new URLSearchParams('p=bogus,kpqc&l=1,9,x&nl=100,512&ps=some&hb=yes&v=3d'));
		expect([...s.programmes]).toEqual(['kpqc']);
		expect([...s.levels]).toEqual([1]);
		expect([...s.ngccLevels]).toEqual([512]);
		expect(s.paramSets).toBe('recommended');
		expect(s.hideBroken).toBe(false);
		expect(s.chart).toBe('sizes');
	});

	it('ignores unknown hide-flag values and lets 0 switch a flag off', () => {
		const on: CommonFilter = { ...ALL, hideBroken: true, hideImplementation: true, hideWarnings: true };
		const s = decodeCommonFilters(on, new URLSearchParams('hb=0&hi=yes&hw=2'));
		expect(s.hideBroken).toBe(false);
		expect(s.hideImplementation).toBe(true);
		expect(s.hideWarnings).toBe(true);
	});

	it('decodes an empty programme list as "none selected"', () => {
		expect(decodeCommonFilters(ALL, new URLSearchParams('p=')).programmes.size).toBe(0);
	});

	it('leaves the defaults object unmutated', () => {
		decodeCommonFilters(ALL, new URLSearchParams('p=kpqc&l=1&nl=128'));
		expect(ALL.programmes.size).toBe(GROUPS.length);
		expect(ALL.levels.size).toBe(6);
		expect(ALL.ngccLevels.size).toBe(4);
	});
});

describe('listedSchemeCount', () => {
	it('counts ticked schemes only when their programme is selected', () => {
		const schemes = [
			{ ...row({ programmes: [{ programme: 'nist-onramp', stage: 'round-2' }] }), scheme: 'A' },
			{ ...row({ programmes: [{ programme: 'china-ngcc', stage: 'round-1' }] }), scheme: 'B' },
			{ ...row({ programmes: [{ programme: 'nist-onramp', stage: 'round-1' }] }), scheme: 'C' }
		];
		const f = { programmes: new Set<ProgrammeGroup>(['nist-onramp']), schemes: new Set(['A', 'B']) };
		expect(listedSchemeCount(schemes, f)).toBe(1);
		expect(listedSchemeCount(schemes, { ...f, programmes: new Set(GROUPS) })).toBe(2);
		expect(listedSchemeCount(schemes, { ...f, programmes: new Set() })).toBe(0);
	});
});

describe('filterSummary', () => {
	it('summarises programmes, schemes and levels', () => {
		expect(filterSummary(ALL, 18)).toBe('6 programmes · 18 schemes · all levels');
		expect(filterSummary({ ...ALL, levels: new Set<NistLevel>([1]), ngccLevels: new Set() }, 1)).toBe(
			'6 programmes · 1 scheme · 1 level'
		);
	});
});
