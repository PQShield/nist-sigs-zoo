import { derived, writable } from 'svelte/store';
import type { Readable, Writable } from 'svelte/store';
import type {
	KemFilterState,
	KemParameterSet,
	KemSortableColumn,
	NistLevel,
} from './types';

const DEFAULT_SORT_COL: KemSortableColumn = 'scheme';
const DEFAULT_SORT_DIR = 'asc' as const;

function defaultFilter(allSchemes: Set<string>): KemFilterState {
	return {
		schemes: new Set(allSchemes),
		levels: new Set(['Pre-Quantum', 1, 2, 3, 4, 5] as NistLevel[]),
		sortCol: DEFAULT_SORT_COL,
		sortDir: DEFAULT_SORT_DIR,
	};
}

function applyUrlParams(state: KemFilterState, params: URLSearchParams): KemFilterState {
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

	const sortParam = params.get('sort');
	if (sortParam) s.sortCol = sortParam as KemSortableColumn;
	const dirParam = params.get('dir');
	if (dirParam === 'asc' || dirParam === 'desc') s.sortDir = dirParam;

	return s;
}

export function buildUrlParams(
	state: KemFilterState,
	defaults: KemFilterState
): URLSearchParams {
	const params = new URLSearchParams();

	const allSchemes = defaults.schemes;
	if (
		state.schemes.size !== allSchemes.size ||
		![...state.schemes].every((s) => allSchemes.has(s))
	) {
		params.set('s', [...state.schemes].map(encodeURIComponent).join(','));
	}

	const allLevels = defaults.levels;
	if (state.levels.size !== allLevels.size || ![...state.levels].every((l) => allLevels.has(l))) {
		params.set('l', [...state.levels].map((l) => (l === 'Pre-Quantum' ? 'PQ' : String(l))).join(','));
	}

	if (state.sortCol !== DEFAULT_SORT_COL) params.set('sort', state.sortCol);
	if (state.sortDir !== DEFAULT_SORT_DIR) params.set('dir', state.sortDir);

	return params;
}

function compareValues(a: KemParameterSet, b: KemParameterSet, col: KemSortableColumn): number {
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
		case 'ct': return a.ct - b.ct;
		case 'pkPlusCt': return a.pkPlusCt - b.pkPlusCt;
		case 'sk': return (a.sk ?? 0) - (b.sk ?? 0);
		case 'keygenUs': return (a.keygenUs ?? a.keygenCycles / 2500) - (b.keygenUs ?? b.keygenCycles / 2500);
		case 'encapsUs': return (a.encapsUs ?? a.encapsCycles / 2500) - (b.encapsUs ?? b.encapsCycles / 2500);
		case 'decapsUs': return (a.decapsUs ?? a.decapsCycles / 2500) - (b.decapsUs ?? b.decapsCycles / 2500);
		default: {
			const av = String(a[col] ?? '').toLowerCase();
			const bv = String(b[col] ?? '').toLowerCase();
			return av < bv ? -1 : av > bv ? 1 : 0;
		}
	}
}

let _store: Writable<KemFilterState> | null = null;
let _defaults: KemFilterState | null = null;
let _filteredRows: Readable<KemParameterSet[]> | null = null;

export function createKemStore(allRows: KemParameterSet[], urlParams: URLSearchParams) {
	const allSchemes = new Set(allRows.map((r) => r.scheme));
	const defaults = defaultFilter(allSchemes);
	const initial = applyUrlParams(defaults, urlParams);

	_defaults = defaults;
	_store = writable(initial);
	_filteredRows = derived(_store, ($state) =>
		allRows
			.filter((row) => {
				if (!$state.schemes.has(row.scheme)) return false;
				if (!$state.levels.has(row.level)) return false;
				return true;
			})
			.sort((a, b) => {
				const cmp = compareValues(a, b, $state.sortCol);
				return $state.sortDir === 'asc' ? cmp : -cmp;
			})
	);

	return { store: _store, defaults, filteredRows: _filteredRows };
}

export function getKemStore() {
	if (!_store || !_defaults || !_filteredRows) throw new Error('KEM store not initialized');
	const store = _store as Writable<KemFilterState>;
	const defaults = _defaults as KemFilterState;
	return {
		store,
		defaults,
		filteredRows: _filteredRows as Readable<KemParameterSet[]>,
		applyUrl(params: URLSearchParams) {
			store.set(applyUrlParams(defaults, params));
		},
	};
}
