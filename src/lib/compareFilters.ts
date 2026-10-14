import { ALL_LEVELS, NGCC_LEVELS } from './constants';
import { STANDARDS_PROGRAMME } from './programmes';
import type { CommonFilter, NgccLevel, NistLevel, ProgrammeGroup, ProgrammeMembership } from './types';

/** Display order of the programme filter. */
export const PROGRAMME_GROUP_ORDER: readonly ProgrammeGroup[] = [
	'nist-standards',
	'nist-pqc-rounds',
	'nist-onramp',
	'china-ngcc',
	'kpqc',
	'iso',
	'research',
	'pre-quantum'
];

export const PROGRAMME_GROUP_LABELS: Record<ProgrammeGroup, string> = {
	'nist-standards': 'NIST standards',
	'nist-pqc-rounds': 'NIST PQC rounds 1–4',
	'nist-onramp': 'NIST on-ramp',
	'china-ngcc': 'China NGCC',
	kpqc: 'KpqC',
	iso: 'ISO/IEC',
	research: 'Research proposals',
	'pre-quantum': 'Pre-quantum reference'
};

/** NIST PQC stages that mean "a NIST standard, or about to be one". */
const STANDARD_STAGES = new Set(['selected', 'standardised']);

/** The row fields the shared filters read. ParameterSet and KemParameterSet both satisfy it. */
export interface FilterableRow {
	programmes: ProgrammeMembership[];
	classical: boolean;
	broken: false | string;
	warning: false | string;
	implementation: false | string;
	recommended: boolean;
	level: NistLevel | null;
	ngccLevel: NgccLevel | null;
}

export function isProgrammeGroup(v: string): v is ProgrammeGroup {
	return (PROGRAMME_GROUP_ORDER as readonly string[]).includes(v);
}

function isNistLevel(v: unknown): v is NistLevel {
	return (ALL_LEVELS as readonly unknown[]).includes(v);
}

function isNgccLevel(v: unknown): v is NgccLevel {
	return (NGCC_LEVELS as readonly unknown[]).includes(v);
}

export function rowProgrammeGroups(row: Pick<FilterableRow, 'programmes' | 'classical'>): ProgrammeGroup[] {
	const groups = new Set<ProgrammeGroup>();
	for (const m of row.programmes) {
		if (m.programme === STANDARDS_PROGRAMME) {
			groups.add(STANDARD_STAGES.has(m.stage) ? 'nist-standards' : 'nist-pqc-rounds');
		} else if (isProgrammeGroup(m.programme)) {
			groups.add(m.programme);
		}
	}
	if (row.classical) groups.add('pre-quantum');
	if (groups.size === 0) groups.add('research');
	return PROGRAMME_GROUP_ORDER.filter((g) => groups.has(g));
}

export function presentProgrammeGroups(rows: Pick<FilterableRow, 'programmes' | 'classical'>[]): ProgrammeGroup[] {
	const present = new Set(rows.flatMap(rowProgrammeGroups));
	return PROGRAMME_GROUP_ORDER.filter((g) => present.has(g));
}

export function defaultCommonFilter(groups: Iterable<ProgrammeGroup>): CommonFilter {
	return {
		programmes: new Set(groups),
		levels: new Set<NistLevel>(ALL_LEVELS),
		ngccLevels: new Set<NgccLevel>(NGCC_LEVELS),
		paramSets: 'recommended',
		hideBroken: false,
		hideImplementation: false,
		hideWarnings: false,
		chart: 'sizes'
	};
}

export function passesCommonFilters(row: FilterableRow, f: CommonFilter): boolean {
	if (!rowProgrammeGroups(row).some((g) => f.programmes.has(g))) return false;
	// Each scale is matched only against its own filter group; the scales are never converted.
	const levelOk =
		(row.level !== null && f.levels.has(row.level)) || (row.ngccLevel !== null && f.ngccLevels.has(row.ngccLevel));
	if (!levelOk) return false;
	if (f.paramSets === 'recommended' && !row.recommended) return false;
	if (f.hideBroken && row.broken !== false && !row.classical) return false;
	if (f.hideImplementation && row.implementation !== false) return false;
	if (f.hideWarnings && row.warning !== false) return false;
	return true;
}

function sameSet<T>(a: Set<T>, b: Set<T>): boolean {
	return a.size === b.size && [...a].every((x) => b.has(x));
}

function list(v: string | null): string[] {
	return v ? v.split(',') : [];
}

const FLAGS = [
	['hb', 'hideBroken'],
	['hi', 'hideImplementation'],
	['hw', 'hideWarnings']
] as const;

/** Add the non-default common filter fields to `params`. */
export function encodeCommonFilters(state: CommonFilter, defaults: CommonFilter, params: URLSearchParams): void {
	if (!sameSet(state.programmes, defaults.programmes)) {
		params.set('p', PROGRAMME_GROUP_ORDER.filter((g) => state.programmes.has(g)).join(','));
	}
	const levelsChanged = !sameSet(state.levels, defaults.levels);
	if (levelsChanged) {
		params.set('l', [...state.levels].map((l) => (l === 'Pre-Quantum' ? 'PQ' : String(l))).join(','));
	}
	// decode reads `l` without `nl` as an old link (no NGCC targets), so `l` always travels with `nl`
	if (levelsChanged || !sameSet(state.ngccLevels, defaults.ngccLevels)) {
		params.set('nl', NGCC_LEVELS.filter((l) => state.ngccLevels.has(l)).join(','));
	}
	if (state.paramSets !== defaults.paramSets) params.set('ps', state.paramSets);
	for (const [key, field] of FLAGS) {
		if (state[field] !== defaults[field]) params.set(key, state[field] ? '1' : '0');
	}
	if (state.chart !== defaults.chart) params.set('v', state.chart);
}

/** Common filter fields from `params`, starting from `state`. Unknown values are ignored. */
export function decodeCommonFilters(state: CommonFilter, params: URLSearchParams): CommonFilter {
	const s: CommonFilter = {
		programmes: new Set(state.programmes),
		levels: new Set(state.levels),
		ngccLevels: new Set(state.ngccLevels),
		paramSets: state.paramSets,
		hideBroken: state.hideBroken,
		hideImplementation: state.hideImplementation,
		hideWarnings: state.hideWarnings,
		chart: state.chart
	};
	if (params.has('p')) s.programmes = new Set(list(params.get('p')).filter(isProgrammeGroup));
	if (params.has('l')) {
		s.levels = new Set(
			list(params.get('l'))
				.map((v) => (v === 'PQ' ? 'Pre-Quantum' : Number(v)))
				.filter(isNistLevel)
		);
		// Links from before NGCC targets existed: their level list was the whole level filter.
		if (!params.has('nl')) s.ngccLevels = new Set();
	}
	if (params.has('nl')) s.ngccLevels = new Set(list(params.get('nl')).map(Number).filter(isNgccLevel));
	const ps = params.get('ps');
	if (ps === 'recommended' || ps === 'all') s.paramSets = ps;
	for (const [key, field] of FLAGS) {
		const v = params.get(key);
		if (v === '1' || v === '0') s[field] = v === '1';
	}
	const v = params.get('v');
	if (v === 'sizes' || v === 'speed') s.chart = v;
	return s;
}

/** One-line summary for the collapsed mobile filter header, e.g. "4 programmes · 18 schemes · all levels". */
export function filterSummary(f: CommonFilter, schemeCount: number): string {
	const plural = (n: number, word: string) => `${n} ${word}${n === 1 ? '' : 's'}`;
	const allLevels = f.levels.size === ALL_LEVELS.length && f.ngccLevels.size === NGCC_LEVELS.length;
	const levels = allLevels ? 'all levels' : plural(f.levels.size + f.ngccLevels.size, 'level');
	return `${plural(f.programmes.size, 'programme')} · ${plural(schemeCount, 'scheme')} · ${levels}`;
}
