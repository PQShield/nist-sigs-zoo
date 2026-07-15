import { derived, writable } from 'svelte/store';
import type { Readable, Writable } from 'svelte/store';
import type { DataRanges, FilterState, NistLevel, ParameterSet, SortableColumn } from './types';
import { CYCLES_PER_US } from './constants';

const DEFAULT_SORT_COL: SortableColumn = 'scheme';
const DEFAULT_SORT_DIR = 'asc' as const;

function defaultFilter(ranges: DataRanges, allSchemes: Set<string>): FilterState {
	return {
		schemes: new Set(allSchemes),
		levels: new Set(['Pre-Quantum', 1, 2, 3, 4, 5] as NistLevel[]),
		minPk: ranges.pk[0],
		maxPk: ranges.pk[1],
		minSig: ranges.sig[0],
		maxSig: ranges.sig[1],
		minPkPlusSig: ranges.pkPlusSig[0],
		maxPkPlusSig: ranges.pkPlusSig[1],
		minSigningCycles: ranges.signingCycles[0],
		maxSigningCycles: ranges.signingCycles[1],
		minVerificationCycles: ranges.verificationCycles[0],
		maxVerificationCycles: ranges.verificationCycles[1],
		minSigningUs: ranges.signingUs?.[0] ?? 0,
		maxSigningUs: ranges.signingUs?.[1] ?? Infinity,
		minVerificationUs: ranges.verificationUs?.[0] ?? 0,
		maxVerificationUs: ranges.verificationUs?.[1] ?? Infinity,
		sortCol: DEFAULT_SORT_COL,
		sortDir: DEFAULT_SORT_DIR,
	};
}

function applyUrlParams(state: FilterState, params: URLSearchParams): FilterState {
	const s = { ...state };

	const schemesParam = params.get('s');
	if (schemesParam) {
		s.schemes = new Set(schemesParam.split(',').map(decodeURIComponent));
	}

	const levelsParam = params.get('l');
	if (levelsParam) {
		s.levels = new Set(
			levelsParam.split(',').map((v) => (v === 'PQ' ? 'Pre-Quantum' : Number(v))) as NistLevel[]
		);
	}

	if (params.has('pkMin')) s.minPk = Number(params.get('pkMin'));
	if (params.has('pkMax')) s.maxPk = Number(params.get('pkMax'));
	if (params.has('sigMin')) s.minSig = Number(params.get('sigMin'));
	if (params.has('sigMax')) s.maxSig = Number(params.get('sigMax'));
	if (params.has('psMin')) s.minPkPlusSig = Number(params.get('psMin'));
	if (params.has('psMax')) s.maxPkPlusSig = Number(params.get('psMax'));
	if (params.has('scMin')) s.minSigningCycles = Number(params.get('scMin'));
	if (params.has('scMax')) s.maxSigningCycles = Number(params.get('scMax'));
	if (params.has('vcMin')) s.minVerificationCycles = Number(params.get('vcMin'));
	if (params.has('vcMax')) s.maxVerificationCycles = Number(params.get('vcMax'));
	if (params.has('suMin')) s.minSigningUs = Number(params.get('suMin'));
	if (params.has('suMax')) s.maxSigningUs = Number(params.get('suMax'));
	if (params.has('vuMin')) s.minVerificationUs = Number(params.get('vuMin'));
	if (params.has('vuMax')) s.maxVerificationUs = Number(params.get('vuMax'));

	const sortParam = params.get('sort');
	if (sortParam) s.sortCol = sortParam as SortableColumn;
	const dirParam = params.get('dir');
	if (dirParam === 'asc' || dirParam === 'desc') s.sortDir = dirParam;

	return s;
}

export function buildUrlParams(state: FilterState, defaults: FilterState): URLSearchParams {
	const params = new URLSearchParams();

	const allSchemes = defaults.schemes;
	if (state.schemes.size !== allSchemes.size || ![...state.schemes].every((s) => allSchemes.has(s))) {
		params.set('s', [...state.schemes].map(encodeURIComponent).join(','));
	}

	const allLevels = defaults.levels;
	if (
		state.levels.size !== allLevels.size ||
		![...state.levels].every((l) => allLevels.has(l))
	) {
		params.set(
			'l',
			[...state.levels].map((l) => (l === 'Pre-Quantum' ? 'PQ' : String(l))).join(',')
		);
	}

	if (state.minPk !== defaults.minPk) params.set('pkMin', String(state.minPk));
	if (state.maxPk !== defaults.maxPk) params.set('pkMax', String(state.maxPk));
	if (state.minSig !== defaults.minSig) params.set('sigMin', String(state.minSig));
	if (state.maxSig !== defaults.maxSig) params.set('sigMax', String(state.maxSig));
	if (state.minPkPlusSig !== defaults.minPkPlusSig) params.set('psMin', String(state.minPkPlusSig));
	if (state.maxPkPlusSig !== defaults.maxPkPlusSig) params.set('psMax', String(state.maxPkPlusSig));
	if (state.minSigningCycles !== defaults.minSigningCycles) params.set('scMin', String(state.minSigningCycles));
	if (state.maxSigningCycles !== defaults.maxSigningCycles) params.set('scMax', String(state.maxSigningCycles));
	if (state.minVerificationCycles !== defaults.minVerificationCycles) params.set('vcMin', String(state.minVerificationCycles));
	if (state.maxVerificationCycles !== defaults.maxVerificationCycles) params.set('vcMax', String(state.maxVerificationCycles));
	if (state.minSigningUs !== defaults.minSigningUs) params.set('suMin', String(state.minSigningUs));
	if (state.maxSigningUs !== defaults.maxSigningUs) params.set('suMax', String(state.maxSigningUs));
	if (state.minVerificationUs !== defaults.minVerificationUs) params.set('vuMin', String(state.minVerificationUs));
	if (state.maxVerificationUs !== defaults.maxVerificationUs) params.set('vuMax', String(state.maxVerificationUs));

	if (state.sortCol !== DEFAULT_SORT_COL) params.set('sort', state.sortCol);
	if (state.sortDir !== DEFAULT_SORT_DIR) params.set('dir', state.sortDir);

	return params;
}

/** Displayed time in µs, or null when the row has no timing data (shown as "—"). */
function timeValue(row: ParameterSet, col: 'signingUs' | 'verificationUs'): number | null {
	if (col === 'signingUs') {
		return row.signingUs ?? (row.signingCycles > 0 ? row.signingCycles / CYCLES_PER_US : null);
	}
	return row.verificationUs ?? (row.verificationCycles > 0 ? row.verificationCycles / CYCLES_PER_US : null);
}

function compareValues(a: ParameterSet, b: ParameterSet, col: SortableColumn): number {
	const levelOrder: Record<string, number> = {
		'Pre-Quantum': 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5,
	};

	switch (col) {
		case 'level': {
			const av = levelOrder[String(a.level)] ?? 99;
			const bv = levelOrder[String(b.level)] ?? 99;
			return av - bv;
		}
		case 'pk': return a.pk - b.pk;
		case 'sig': return a.sig - b.sig;
		case 'pkPlusSig': return a.pkPlusSig - b.pkPlusSig;
		case 'signingCycles': return a.signingCycles - b.signingCycles;
		case 'verificationCycles': return a.verificationCycles - b.verificationCycles;
		default: {
			const av = String(a[col] ?? '').toLowerCase();
			const bv = String(b[col] ?? '').toLowerCase();
			return av < bv ? -1 : av > bv ? 1 : 0;
		}
	}
}

/** Sort comparator: time columns keep rows without data last regardless of direction. */
export function sortRows(a: ParameterSet, b: ParameterSet, col: SortableColumn, dir: 'asc' | 'desc'): number {
	if (col === 'signingUs' || col === 'verificationUs') {
		const av = timeValue(a, col);
		const bv = timeValue(b, col);
		if (av == null && bv == null) return 0;
		if (av == null) return 1;
		if (bv == null) return -1;
		return dir === 'asc' ? av - bv : bv - av;
	}
	const cmp = compareValues(a, b, col);
	return dir === 'asc' ? cmp : -cmp;
}

// Stable module-level stores — created once, updated in place on round changes
const _allRows = writable<ParameterSet[]>([]);
let _store: Writable<FilterState> | null = null;
let _defaults: FilterState | null = null;
// _filteredRows derives from both _store and _allRows, so it stays valid across round changes
let _filteredRows: Readable<ParameterSet[]> | null = null;

export function createFilterStore(
	allRows: ParameterSet[],
	ranges: DataRanges,
	urlParams: URLSearchParams
) {
	const allSchemes = new Set(allRows.map((r) => r.scheme));
	const defaults = defaultFilter(ranges, allSchemes);
	const initial = applyUrlParams(defaults, urlParams);

	_defaults = defaults;
	_allRows.set(allRows);

	if (!_store || !_filteredRows) {
		_store = writable(initial);
		_filteredRows = derived([_store, _allRows], ([$state, $rows]) => {
			return $rows
				.filter((row) => {
					if (!$state.schemes.has(row.scheme)) return false;
					if (!$state.levels.has(row.level)) return false;
					if (row.pk < $state.minPk || row.pk > $state.maxPk) return false;
					if (row.sig < $state.minSig || row.sig > $state.maxSig) return false;
					if (row.pkPlusSig < $state.minPkPlusSig || row.pkPlusSig > $state.maxPkPlusSig)
						return false;
					if (
						row.signingCycles < $state.minSigningCycles ||
						row.signingCycles > $state.maxSigningCycles
					)
						return false;
					if (
						row.verificationCycles < $state.minVerificationCycles ||
						row.verificationCycles > $state.maxVerificationCycles
					)
						return false;
					if (
						row.signingUs != null &&
						(row.signingUs < $state.minSigningUs || row.signingUs > $state.maxSigningUs)
					)
						return false;
					if (
						row.verificationUs != null &&
						(row.verificationUs < $state.minVerificationUs || row.verificationUs > $state.maxVerificationUs)
					)
						return false;
					return true;
				})
				.sort((a, b) => sortRows(a, b, $state.sortCol, $state.sortDir));
		});
	} else {
		// Round change: reset filter state to new defaults, data already updated via _allRows
		_store.set(initial);
	}

	return { store: _store, defaults, filteredRows: _filteredRows };
}

export function getFilterStore() {
	if (!_store || !_defaults || !_filteredRows) throw new Error('Filter store not initialized');
	const store = _store as Writable<FilterState>;
	const defaults = _defaults as FilterState;
	return {
		store,
		defaults,
		filteredRows: _filteredRows as Readable<ParameterSet[]>,
		applyUrl(params: URLSearchParams) {
			store.set(applyUrlParams(defaults, params));
		},
	};
}
