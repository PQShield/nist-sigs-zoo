import { describe, expect, it } from 'vitest';
import {
	compareRows,
	decodeCompareFilter,
	defaultCompareFilter,
	encodeCompareFilter,
	filterCompareRows,
	sortCompareRows,
	type FilterSpec
} from '$lib/compare';
import type { CompareFilter, NistLevel, NgccLevel, ProgrammeMembership } from '$lib/types';

interface Row {
	scheme: string;
	parameterset: string;
	size: number;
	time: number | null;
	programmes: ProgrammeMembership[];
	classical: boolean;
	broken: false | string;
	warning: false | string;
	implementation: false | string;
	recommended: boolean;
	level: NistLevel | null;
	ngccLevel: NgccLevel | null;
}
interface Ranges {
	size: [number, number];
	time: [number, number] | null;
}
interface Filter extends CompareFilter {
	minSize: number;
	maxSize: number;
	minTime: number;
	maxTime: number;
}

const row = (scheme: string, size: number, time: number | null, level: NistLevel | null = 1): Row => ({
	scheme,
	parameterset: `${scheme}-p`,
	size,
	time,
	programmes: [],
	classical: false,
	broken: false,
	warning: false,
	implementation: false,
	recommended: true,
	level,
	ngccLevel: level === null ? 128 : null
});

const spec = (sortInUrl: boolean): FilterSpec<Row, Ranges, Filter> => ({
	bounds: [
		{ field: 'Size', url: 'sz', label: 'size', section: 'Sizes', value: (r) => r.size, range: (g) => g.size },
		{ field: 'Time', url: 'tm', label: 'time', section: 'Speed', value: (r) => r.time, range: (g) => g.time }
	],
	defaultSort: 'size',
	sortInUrl,
	numericSort: { size: (r) => r.size },
	timeSort: { time: (r) => r.time }
});

const ROWS = [row('B', 20, 5), row('a', 10, null, 'Pre-Quantum'), row('C', 30, 1, null)];
const RANGES: Ranges = { size: [10, 30], time: null };

describe('defaultCompareFilter', () => {
	it('sets bounds from the ranges, 0 to Infinity where there is no data, and the default sort', () => {
		const f = defaultCompareFilter(spec(true), RANGES, ['a', 'B'], ['research']);
		expect([f.minSize, f.maxSize, f.minTime, f.maxTime]).toEqual([10, 30, 0, Infinity]);
		expect([f.sortCol, f.sortDir]).toEqual(['size', 'asc']);
		expect([...f.schemes]).toEqual(['a', 'B']);
	});
});

describe('URL codec', () => {
	const defaults = defaultCompareFilter(spec(true), RANGES, ['a', 'B', 'C'], ['research']);

	it('encodes bounds under <url>Min / <url>Max and round-trips them', () => {
		const p = encodeCompareFilter(spec(true), { ...defaults, maxSize: 25, minTime: 2 }, defaults);
		expect(p.toString()).toBe('szMax=25&tmMin=2');
		const back = decodeCompareFilter(spec(true), defaults, p);
		expect([back.maxSize, back.minTime, back.minSize]).toEqual([25, 2, 10]);
	});

	it('carries the sort only when the spec says so', () => {
		const state = { ...defaults, sortCol: 'scheme', sortDir: 'desc' as const };
		expect(encodeCompareFilter(spec(true), state, defaults).toString()).toBe('sort=scheme&dir=desc');
		expect(encodeCompareFilter(spec(false), state, defaults).toString()).toBe('');
		expect(decodeCompareFilter(spec(false), defaults, new URLSearchParams('sort=scheme')).sortCol).toBe('size');
	});
});

describe('filterCompareRows', () => {
	it('applies bounds, letting rows without a value pass', () => {
		const f = defaultCompareFilter(spec(true), { size: [10, 30], time: [1, 5] }, ['a', 'B', 'C'], ['research']);
		const out = filterCompareRows(spec(true), ROWS, { ...f, maxTime: 3 });
		expect(out.map((r) => r.scheme)).toEqual(['a', 'C']);
	});
});

describe('sortCompareRows', () => {
	const names = (col: string, dir: 'asc' | 'desc') => sortCompareRows(spec(true), ROWS, col, dir).map((r) => r.scheme);

	it('sorts numeric keys, text case-insensitively and levels in order with NGCC rows last', () => {
		expect(names('size', 'desc')).toEqual(['C', 'B', 'a']);
		expect(names('scheme', 'asc')).toEqual(['a', 'B', 'C']);
		expect(names('level', 'asc')).toEqual(['a', 'B', 'C']);
	});

	it('keeps rows without a time last in both directions', () => {
		expect(names('time', 'asc')).toEqual(['C', 'B', 'a']);
		expect(names('time', 'desc')).toEqual(['B', 'C', 'a']);
	});

	it('treats an unknown column from the URL as text', () => {
		expect(compareRows(spec(true), ROWS[0], ROWS[1], 'constructor', 'asc')).toBe(0);
	});
});
