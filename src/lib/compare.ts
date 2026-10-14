import type { ChartMode, CompareFilter, ProgrammeGroup, SortDir } from './types';
import { levelDescription } from './chartLegend';
import { timeTooltip } from './chartData';
import { stripHtml } from './html';
import {
	decodeCommonFilters,
	defaultCommonFilter,
	encodeCommonFilters,
	passesCommonFilters,
	type FilterableRow
} from './compareFilters';

/** Row fields the shared sort and table read. ParameterSet and KemParameterSet both satisfy it. */
export interface CompareRow extends FilterableRow {
	scheme: string;
	parameterset: string;
	category: string;
	status: string;
	assumption: string;
	info: false | string;
	findingsUrl: string | null;
	notes: string | null;
	version?: string;
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

/** A table column after Scheme / Category / Programmes / Parameter Set / Level. */
export interface TableColumn<R> {
	key: string;
	label: string;
	/** Size in bytes. */
	size?: (row: R) => number;
	/** Measured µs and cycle count; the time is extrapolated from cycles when not measured. */
	time?: (row: R) => [number | null, number];
}

export interface TableSpec<R> {
	columns: TableColumn<R>[];
	/** Show the parameter set without the "<scheme>-" prefix (full name as title). */
	shortParamSet: boolean;
	emptyText: string;
}

/** A timed operation, for the chart tooltip. */
export interface ChartOp<R> {
	label: string;
	us: (row: R) => number | null;
	cycles: (row: R) => number;
}

export interface ChartSpec<R, A extends string> {
	/** Every axis field (the point carries all of them) and its axis title. */
	axisTitles: Record<A, string>;
	/** Short names for the advanced page heading. */
	axisShort: Record<A, string>;
	/** Fields offered by the advanced page's axis selects; the rest are the main page's time axes. */
	axisOptions: A[];
	/** Axis value of a row; null when it has none (left out of the chart). */
	value: (row: R, field: A) => number | null;
	/** Main page views: heading and axes per chart mode. */
	views: Record<ChartMode, { title: string; x: A; y: A }>;
	/** Tooltip size entries. */
	sizes: [label: string, value: (row: R) => number][];
	ops: ChartOp<R>[];
	/** Add the parameter set's notes to the tooltip notes. */
	rowNotes: boolean;
	/** Point sizes: default, per family (Standardised and others), and the broken/warning rings. */
	marks: { size: number; families: Record<string, number>; ring: number };
}

/** Everything the shared compare components need to know about one primitive. */
export interface CompareConfig<R extends CompareRow, G, F extends CompareFilter, A extends string = string> {
	filter: FilterSpec<R, G, F>;
	table: TableSpec<R>;
	chart: ChartSpec<R, A>;
}

/** Chart tooltip of a row, in display order. */
export function chartTooltip<R extends CompareRow, A extends string>(
	spec: ChartSpec<R, A>,
	d: R
): Record<string, string | number> {
	const notes = [
		d.broken && !d.classical ? `⚠ ${stripHtml(d.broken)}` : null,
		d.warning ? `⚠ ${stripHtml(d.warning)}` : null,
		spec.rowNotes ? (d.notes ?? null) : null
	]
		.filter(Boolean)
		.join(' | ');
	const cycles: Record<string, string> = {};
	for (const op of spec.ops) {
		const c = op.cycles(d);
		if (c > 0) cycles[`${op.label} (cycles)`] = c.toLocaleString();
	}
	return {
		Scheme: d.scheme,
		'Parameter set': d.parameterset,
		...(d.version ? { Version: d.version } : {}),
		Family: d.category,
		'Security level': levelDescription(d.level, d.ngccLevel),
		...Object.fromEntries(spec.sizes.map(([label, value]) => [label, value(d).toLocaleString()])),
		...cycles,
		...timeTooltip(spec.ops.map((op) => ({ label: op.label, us: op.us(d), cycles: op.cycles(d) }))),
		...(notes ? { Notes: notes } : {}),
		'Scheme page': 'click the point'
	};
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
