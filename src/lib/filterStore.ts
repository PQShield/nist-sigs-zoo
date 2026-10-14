import { derived, writable } from 'svelte/store';
import type { Readable, Writable } from 'svelte/store';
import type { DataRanges, FilterState, ParameterSet, SortDir } from './types';
import { presentProgrammeGroups } from './compareFilters';
import {
	compareRows,
	decodeCompareFilter,
	defaultCompareFilter,
	encodeCompareFilter,
	filterCompareRows
} from './compare';
import { SIGNATURE_FILTER } from './primitives';

export function buildUrlParams(state: FilterState, defaults: FilterState): URLSearchParams {
	return encodeCompareFilter(SIGNATURE_FILTER, state, defaults);
}

/** Sort comparator: time columns keep rows without data last regardless of direction. */
export function sortRows(a: ParameterSet, b: ParameterSet, col: string, dir: SortDir): number {
	return compareRows(SIGNATURE_FILTER, a, b, col, dir);
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
	urlParams: URLSearchParams,
	adjust: (f: FilterState) => FilterState = (f) => f
) {
	const allSchemes = new Set(allRows.map((r) => r.scheme));
	const defaults = defaultCompareFilter(
		SIGNATURE_FILTER,
		ranges,
		allSchemes,
		presentProgrammeGroups(allRows)
	);
	const initial = adjust(decodeCompareFilter(SIGNATURE_FILTER, defaults, urlParams));

	_defaults = defaults;
	_allRows.set(allRows);

	if (!_store || !_filteredRows) {
		_store = writable(initial);
		_filteredRows = derived([_store, _allRows], ([$state, $rows]) =>
			filterCompareRows(SIGNATURE_FILTER, $rows, $state).sort((a, b) =>
				sortRows(a, b, $state.sortCol, $state.sortDir)
			)
		);
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
			// read _defaults at call time: createFilterStore() may have replaced them since
			store.set(decodeCompareFilter(SIGNATURE_FILTER, _defaults ?? defaults, params));
		},
	};
}
