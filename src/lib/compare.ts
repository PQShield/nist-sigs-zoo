import type { CommonFilter, ProgrammeGroup } from './types';
import {
	decodeCommonFilters,
	defaultCommonFilter,
	encodeCommonFilters,
	passesCommonFilters,
	type FilterableRow
} from './compareFilters';

export type SortDir = 'asc' | 'desc';

/** Filter state of a compare page; each bound adds numeric `min<Field>` / `max<Field>` fields. */
export interface CompareFilter extends CommonFilter {
	schemes: Set<string>;
	sortCol: string;
	sortDir: SortDir;
}

/** Row fields the shared sort and table read. ParameterSet and KemParameterSet both satisfy it. */
export interface CompareRow extends FilterableRow {
	scheme: string;
	parameterset: string;
	category: string;
	status: string;
	assumption: string;
}

/** A min/max filter on one row value. */
export interface BoundSpec<R, G> {
	/** Filter fields `min<field>` / `max<field>`. */
	field: string;
	/** URL keys `<url>Min` / `<url>Max`. */
	url: string;
	/** Filter panel labels "Min <label>" / "Max <label>". */
	label: string;
	/** Filter panel heading; bounds with the same section are grouped. */
	section: string;
	/** null: the row has no value here (not yet benchmarked) and passes. */
	value: (row: R) => number | null;
	/** Data extremes; null when no row has a value (field hidden, bounds 0 to Infinity). */
	range: (ranges: G) => [number, number] | null;
}

/** Per-primitive filter fields, URL keys and sort keys. `F` is the page's filter state type. */
export interface FilterSpec<R, G, F extends CompareFilter = CompareFilter> {
	bounds: BoundSpec<R, G>[];
	defaultSort: F['sortCol'];
	/** Whether `sort` / `dir` travel in the URL. */
	sortInUrl: boolean;
	/** Numeric sort keys; other columns sort as text, `level` by level. */
	numericSort: Record<string, (row: R) => number>;
	/** Time sort keys: rows without a time sort last in both directions. */
	timeSort: Record<string, (row: R) => number | null>;
}

type Bounds = Record<string, number>;

const minKey = (b: { field: string }) => `min${b.field}`;
const maxKey = (b: { field: string }) => `max${b.field}`;

/** Default filter: every scheme and programme group, all levels, bounds at the data extremes. */
export function defaultCompareFilter<R, G, F extends CompareFilter>(
	spec: FilterSpec<R, G, F>,
	ranges: G,
	schemeNames: Iterable<string>,
	groups: Iterable<ProgrammeGroup>
): F {
	const bounds: Bounds = {};
	for (const b of spec.bounds) {
		const r = b.range(ranges);
		bounds[minKey(b)] = r?.[0] ?? 0;
		bounds[maxKey(b)] = r?.[1] ?? Infinity;
	}
	return {
		...defaultCommonFilter(groups),
		schemes: new Set(schemeNames),
		...bounds,
		sortCol: spec.defaultSort,
		sortDir: 'asc'
	} as unknown as F;
}

/** Encode non-default filter state as URL query params. */
export function encodeCompareFilter<R, G, F extends CompareFilter>(
	spec: FilterSpec<R, G, F>,
	state: F,
	defaults: F
): URLSearchParams {
	const p = new URLSearchParams();
	if (state.schemes.size !== defaults.schemes.size || ![...state.schemes].every((s) => defaults.schemes.has(s))) {
		p.set('s', [...state.schemes].map(encodeURIComponent).join(','));
	}
	encodeCommonFilters(state, defaults, p);
	const s = state as unknown as Bounds;
	const d = defaults as unknown as Bounds;
	for (const b of spec.bounds) {
		if (s[minKey(b)] !== d[minKey(b)]) p.set(`${b.url}Min`, String(s[minKey(b)]));
		if (s[maxKey(b)] !== d[maxKey(b)]) p.set(`${b.url}Max`, String(s[maxKey(b)]));
	}
	if (spec.sortInUrl) {
		if (state.sortCol !== defaults.sortCol) p.set('sort', state.sortCol);
		if (state.sortDir !== defaults.sortDir) p.set('dir', state.sortDir);
	}
	return p;
}

/** Decode URL query params onto a copy of `defaults`. */
export function decodeCompareFilter<R, G, F extends CompareFilter>(
	spec: FilterSpec<R, G, F>,
	defaults: F,
	params: URLSearchParams
): F {
	const s: F = { ...defaults, schemes: new Set(defaults.schemes) };
	const schemesParam = params.get('s');
	if (schemesParam) s.schemes = new Set(schemesParam.split(',').map(decodeURIComponent));
	const bounds = s as unknown as Bounds;
	for (const b of spec.bounds) {
		if (params.has(`${b.url}Min`)) bounds[minKey(b)] = Number(params.get(`${b.url}Min`));
		if (params.has(`${b.url}Max`)) bounds[maxKey(b)] = Number(params.get(`${b.url}Max`));
	}
	if (spec.sortInUrl) {
		const sort = params.get('sort');
		if (sort) s.sortCol = sort as F['sortCol'];
		const dir = params.get('dir');
		if (dir === 'asc' || dir === 'desc') s.sortDir = dir;
	}
	return { ...s, ...decodeCommonFilters(s, params) };
}

function outside(v: number | null, min: number, max: number): boolean {
	return v != null && (v < min || v > max);
}

/** Apply scheme, common (programme/level/parameter-set/status) and bound filters. */
export function filterCompareRows<R extends FilterableRow & { scheme: string }, G, F extends CompareFilter>(
	spec: FilterSpec<R, G, F>,
	rows: R[],
	filter: F
): R[] {
	const f = filter as unknown as Bounds;
	return rows.filter(
		(row) =>
			filter.schemes.has(row.scheme) &&
			passesCommonFilters(row, filter) &&
			spec.bounds.every((b) => !outside(b.value(row), f[minKey(b)], f[maxKey(b)]))
	);
}

const LEVEL_ORDER: Record<string, number> = { 'Pre-Quantum': 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5 };

/** Sort comparator: time columns keep rows without data last regardless of direction. */
export function compareRows<R, G, F extends CompareFilter>(spec: FilterSpec<R, G, F>, a: R, b: R, col: string, dir: SortDir): number {
	const time = Object.hasOwn(spec.timeSort, col) ? spec.timeSort[col] : undefined;
	if (time) {
		const av = time(a);
		const bv = time(b);
		if (av == null && bv == null) return 0;
		if (av == null) return 1;
		if (bv == null) return -1;
		return dir === 'asc' ? av - bv : bv - av;
	}
	let cmp: number;
	const numeric = Object.hasOwn(spec.numericSort, col) ? spec.numericSort[col] : undefined;
	if (col === 'level') {
		const level = (r: R) => LEVEL_ORDER[String((r as { level: unknown }).level)] ?? 99;
		cmp = level(a) - level(b);
	} else if (numeric) {
		cmp = numeric(a) - numeric(b);
	} else {
		const av = String((a as Record<string, unknown>)[col] ?? '').toLowerCase();
		const bv = String((b as Record<string, unknown>)[col] ?? '').toLowerCase();
		cmp = av < bv ? -1 : av > bv ? 1 : 0;
	}
	return dir === 'asc' ? cmp : -cmp;
}

export function sortCompareRows<R, G, F extends CompareFilter>(spec: FilterSpec<R, G, F>, rows: R[], col: string, dir: SortDir): R[] {
	return [...rows].sort((a, b) => compareRows(spec, a, b, col, dir));
}
