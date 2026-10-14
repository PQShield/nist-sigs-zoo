import { describe, expect, it } from 'vitest';
import {
	chartTooltip,
	compareData,
	compareRows,
	decodeAxes,
	decodeCompareFilter,
	defaultCompareFilter,
	encodeAxes,
	encodeCompareFilter,
	filterCompareRows,
	sortCompareRows,
	type FilterSpec
} from '$lib/compare';
import { processYamlSchemes } from '$lib/data';
import { processKemSchemes } from '$lib/kemData';
import { KEMS, SIGNATURES } from '$lib/primitives';
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

	it('encodes scheme names once and round-trips odd names', () => {
		const odd = defaultCompareFilter(spec(true), RANGES, ['a', 'Aigis-Sig+', '100%', 'MAYO%', 'B C'], ['research']);
		const state = { ...odd, schemes: new Set(['Aigis-Sig+', '100%', 'B C']) };
		const p = encodeCompareFilter(spec(true), state, odd);
		expect(p.toString()).toBe('s=Aigis-Sig%2B%2C100%25%2CB+C');
		expect([...decodeCompareFilter(spec(true), odd, new URLSearchParams(p.toString())).schemes]).toEqual([
			'Aigis-Sig+',
			'100%',
			'B C'
		]);
		expect([...decodeCompareFilter(spec(true), odd, new URLSearchParams('s=100%')).schemes]).toEqual(['100%']);
	});

	it('still reads links from the old double-encoding encoder', () => {
		// The old encoder ran encodeURIComponent on each name before URLSearchParams encoded the list.
		const odd = defaultCompareFilter(spec(true), RANGES, ['a', 'Aigis-Sig+', '100%', 'B C'], ['research']);
		const old = new URLSearchParams();
		old.set('s', ['Aigis-Sig+', '100%', 'B C'].map(encodeURIComponent).join(','));
		expect(old.toString()).toBe('s=Aigis-Sig%252B%2C100%2525%2CB%2520C');
		expect([...decodeCompareFilter(spec(true), odd, new URLSearchParams(old.toString())).schemes]).toEqual([
			'Aigis-Sig+',
			'100%',
			'B C'
		]);
	});

	it('ignores a malformed scheme escape instead of throwing', () => {
		const f = decodeCompareFilter(spec(true), defaults, new URLSearchParams('?s=MAYO%&szMax=25'));
		expect([...f.schemes]).toEqual(['MAYO%']);
		expect(f.maxSize).toBe(25);
		const p = new URLSearchParams();
		p.set('s', 'a,%E0%A4%A');
		expect([...decodeCompareFilter(spec(true), defaults, p).schemes]).toEqual(['a', '%E0%A4%A']);
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

describe('chartTooltip', () => {
	it('lists a signature row with its version, cycles and times but not its notes', () => {
		const [row] = processYamlSchemes([
			{
				name: 'S',
				website: '',
				category: 'Lattices',
				assumption: '',
				versions: [
					{
						version: 'v2',
						date: '2025-01-01',
						status: 'On-ramp',
						warning: 'side <b>channel</b>',
						parametersets: [
							{ name: 'S-1', level: 1, pk: 1000, sig: 2000, signing_cycles: 2_500_000, verification_us: 3, notes: 'n' }
						]
					}
				]
			}
		]).parameterSets;
		const t = chartTooltip(SIGNATURES.chart, row);
		expect(Object.keys(t)).toEqual([
			'Scheme',
			'Parameter set',
			'Version',
			'Family',
			'Security level',
			'pk (bytes)',
			'sig (bytes)',
			'pk+sig (bytes)',
			'Signing (cycles)',
			'Signing time',
			'Verification time',
			'Notes',
			'Scheme page'
		]);
		expect(t['pk+sig (bytes)']).toBe((3000).toLocaleString());
		expect(t['Signing time']).toBe('1.00 ms (estimated from cycles)');
		expect(t.Notes).toBe('⚠ side channel');
	});

	it('lists a KEM row with all three operations and its notes', () => {
		const [row] = processKemSchemes([
			{
				name: 'K',
				website: '',
				category: 'Lattices',
				assumption: '',
				status: 'x',
				broken: 'attack',
				parametersets: [{ name: 'K-1', ngcc_level: 128, pk: 10, ct: 20, encaps_cycles: 2500, notes: 'reference code' }]
			}
		]).parameterSets;
		const t = chartTooltip(KEMS.chart, row);
		expect(t).toEqual({
			Scheme: 'K',
			'Parameter set': 'K-1',
			Family: 'Lattices',
			'Security level': 'NGCC target: 128-bit classical',
			'pk (bytes)': '10',
			'ct (bytes)': '20',
			'pk+ct (bytes)': '30',
			'Encapsulation (cycles)': (2500).toLocaleString(),
			'Encapsulation time': '1.0 µs (estimated from cycles)',
			Notes: '⚠ attack | reference code',
			'Scheme page': 'click the point'
		});
	});
});

describe('advanced page axes', () => {
	it('decode keeps offered fields and known scales, defaulting to the Sizes view on log scales', () => {
		expect(decodeAxes(SIGNATURES.chart, new URLSearchParams(''))).toEqual({ x: 'pk', y: 'sig', xs: 'log', ys: 'log' });
		expect(decodeAxes(KEMS.chart, new URLSearchParams('x=keygenUs&ys=linear'))).toEqual({
			x: 'keygenUs',
			y: 'ct',
			xs: 'log',
			ys: 'linear'
		});
		// time axes belong to the main page's Speed view and are not offered here
		expect(decodeAxes(SIGNATURES.chart, new URLSearchParams('x=signingTime&y=nope&xs=cubic')).x).toBe('pk');
	});

	it('encode omits defaults', () => {
		const p = new URLSearchParams();
		encodeAxes(SIGNATURES.chart, { x: 'pk', y: 'signingUs', xs: 'linear', ys: 'log' }, p);
		expect(p.toString()).toBe('y=signingUs&xs=linear');
	});
});

describe('compareData', () => {
	it('lists sorted categories and the programme groups present', () => {
		const { schemes, parameterSets } = processKemSchemes([
			{
				name: 'B',
				website: '',
				category: 'Lattices',
				assumption: '',
				status: 'x',
				parametersets: [{ name: 'B-1', level: 1, pk: 1, ct: 1 }]
			},
			{
				name: 'A',
				website: '',
				category: 'Code-based',
				assumption: '',
				status: 'x',
				programmes: { kpqc: 'round-1' },
				parametersets: [{ name: 'A-1', level: 1, pk: 1, ct: 1 }]
			}
		]);
		const d = compareData(schemes, parameterSets, null);
		expect(d.categories).toEqual(['Code-based', 'Lattices']);
		expect(d.groups).toEqual(['kpqc', 'research']);
	});
});
