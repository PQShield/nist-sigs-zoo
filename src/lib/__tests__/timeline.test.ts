import { describe, expect, it } from 'vitest';
import {
	decodeTimelineFilter,
	defaultTimelineFilter,
	encodeTimelineFilter,
	entryChips,
	entryProgrammes,
	filterTimeline,
	HISTORY_TYPE_LABELS,
	isHistoryType,
	presentTypes,
	programmeLabel,
	programmeOptions,
	schemeProgrammeIndex,
	SITE_PROGRAMME,
	timelineItems,
	timelineSummary
} from '$lib/timeline';
import type { HistoryEntry, HistoryType, KemSchemeYaml, ProgrammeYaml, SchemeYaml } from '$lib/types';

const programme = (id: string, short: string, nav_label: string): ProgrammeYaml => ({
	id,
	name: id,
	short,
	nav_label,
	organiser: short,
	region: 'Somewhere',
	primitives: ['signature', 'kem'],
	stages: [{ id: 'round-1', label: 'round 1' }]
});

// Registry order, as in data/programmes.yaml.
const REGISTRY: ProgrammeYaml[] = [
	programme('nist-pqc', 'NIST', 'NIST PQC'),
	programme('nist-onramp', 'On-ramp', 'NIST on-ramp'),
	programme('china-ngcc', 'NGCC', 'China NGCC'),
	programme('kpqc', 'KpqC', 'KpqC'),
	programme('iso', 'ISO', 'ISO/IEC 18033-2')
];

function sig(name: string, ...versions: Record<string, string>[]): SchemeYaml {
	return {
		name,
		website: '',
		category: 'Lattice',
		assumption: '',
		versions: versions.map((programmes, i) => ({
			version: `v${i}`,
			date: `2026-0${i + 1}-01`,
			status: 'On-ramp',
			programmes,
			parametersets: []
		}))
	};
}

function kem(name: string, programmes?: Record<string, string>): KemSchemeYaml {
	return { name, website: '', category: 'Lattice', assumption: '', status: 'Research', programmes, parametersets: [] } as KemSchemeYaml;
}

// MAYO: the newest version has left the on-ramp map (post-round update) but MAYO still counts as on-ramp.
// Dual: a signature in KpqC and a KEM of the same name in NGCC. Real data forbids shared names
// (buildSchemeIndex throws); this is a defensive check of the union behaviour.
const INDEX = schemeProgrammeIndex(
	[sig('MAYO', { 'nist-onramp': 'round-1' }, {}), sig('ML-DSA', { 'nist-pqc': 'standardised' }), sig('Dual', { kpqc: 'round-1' })],
	[kem('Dual', { 'china-ngcc': 'round-1' }), kem('ML-KEM', { 'nist-pqc': 'standardised' }), kem('Plain')]
);

const entry = (o: Partial<HistoryEntry> = {}): HistoryEntry => ({ date: '2026-01-01', type: 'update', description: 'x', ...o });

describe('schemeProgrammeIndex', () => {
	it('collects programmes from every version', () => {
		expect([...INDEX.get('MAYO')!]).toEqual(['nist-onramp']);
	});
	it('merges a signature and a KEM that share a name', () => {
		expect([...INDEX.get('Dual')!].sort()).toEqual(['china-ngcc', 'kpqc']);
	});
	it('keeps schemes with no programmes as an empty set', () => {
		expect(INDEX.get('Plain')!.size).toBe(0);
	});
});

describe('entryProgrammes', () => {
	it('derives programmes from the named schemes', () => {
		expect(entryProgrammes(entry({ schemes: ['MAYO'] }), INDEX, REGISTRY)).toEqual(['nist-onramp']);
	});
	it('returns registry order without duplicates', () => {
		expect(entryProgrammes(entry({ schemes: ['Dual', 'ML-KEM'], programmes: ['kpqc'] }), INDEX, REGISTRY)).toEqual([
			'nist-pqc',
			'china-ngcc',
			'kpqc'
		]);
	});
	it('adds explicit programmes to the derived ones', () => {
		expect(entryProgrammes(entry({ schemes: ['MAYO'], programmes: ['nist-pqc'] }), INDEX, REGISTRY)).toEqual([
			'nist-pqc',
			'nist-onramp'
		]);
	});
	it('uses explicit programmes alone when no scheme is named', () => {
		expect(entryProgrammes(entry({ programmes: ['china-ngcc'] }), INDEX, REGISTRY)).toEqual(['china-ngcc']);
	});
	it('falls back to site for entries tied to no programme', () => {
		expect(entryProgrammes(entry(), INDEX, REGISTRY)).toEqual([SITE_PROGRAMME]);
		expect(entryProgrammes(entry({ schemes: ['Plain'] }), INDEX, REGISTRY)).toEqual([SITE_PROGRAMME]);
	});
	it('ignores unknown schemes and programme ids (the data test reports them)', () => {
		expect(entryProgrammes(entry({ schemes: ['Nope'], programmes: ['bogus'] }), INDEX, REGISTRY)).toEqual([SITE_PROGRAMME]);
	});
});

describe('timelineItems', () => {
	const entries = [
		entry({ date: '2026-01-01', description: 'old' }),
		entry({ date: '2026-03-01', description: 'new A' }),
		entry({ date: '2026-03-01', description: 'new B' })
	];
	const items = timelineItems(entries, INDEX, REGISTRY);
	it('sorts newest first and keeps file order within a date', () => {
		expect(items.map((i) => i.entry.description)).toEqual(['new A', 'new B', 'old']);
	});
	it('gives every item a unique key', () => {
		expect(new Set(items.map((i) => i.key)).size).toBe(3);
	});
	it('leaves the input unsorted', () => {
		expect(entries[0].description).toBe('old');
	});
});

describe('options and labels', () => {
	const items = timelineItems(
		[entry({ schemes: ['Dual'] }), entry({ type: 'attack', schemes: ['MAYO'] }), entry({ type: 'milestone' })],
		INDEX,
		REGISTRY
	);
	it('offers only programmes that occur, in registry order, then site', () => {
		expect(programmeOptions(items, REGISTRY).map((o) => o.id)).toEqual(['nist-onramp', 'china-ngcc', 'kpqc', SITE_PROGRAMME]);
	});
	it('offers only event types that occur, in filter order', () => {
		expect(presentTypes(items)).toEqual(['milestone', 'attack', 'update']);
	});
	it('labels programmes with their nav_label', () => {
		expect(programmeLabel('nist-pqc', REGISTRY)).toBe('NIST PQC');
		expect(programmeLabel('nist-onramp', REGISTRY)).toBe('NIST on-ramp');
		expect(programmeLabel('china-ngcc', REGISTRY)).toBe('China NGCC');
		expect(programmeLabel(SITE_PROGRAMME, REGISTRY)).toBe('Site updates');
	});
	it('labels types in British spelling', () => {
		expect(HISTORY_TYPE_LABELS.standardization).toBe('Standardisation');
	});
	it('recognises event types', () => {
		expect(isHistoryType('attack')).toBe(true);
		expect(isHistoryType('standardisation')).toBe(false);
	});
	it('builds chips with the option labels, all in the plain tone', () => {
		const options = programmeOptions(items, REGISTRY);
		expect(entryChips(['china-ngcc', 'kpqc'], options)).toEqual([
			{ label: 'China NGCC', tone: 'plain', programme: 'china-ngcc' },
			{ label: 'KpqC', tone: 'plain', programme: 'kpqc' }
		]);
		expect(entryChips([SITE_PROGRAMME], options)).toEqual([{ label: 'Site updates', tone: 'plain' }]);
	});
});

describe('filterTimeline', () => {
	const items = timelineItems(
		[entry({ schemes: ['MAYO'] }), entry({ type: 'attack', schemes: ['Dual'] }), entry({ description: 'site news' })],
		INDEX,
		REGISTRY
	);
	const programmes = programmeOptions(items, REGISTRY).map((o) => o.id);
	const types = presentTypes(items);
	const all = defaultTimelineFilter(programmes, types);

	it('shows everything by default, including site entries', () => {
		expect(filterTimeline(items, all)).toHaveLength(3);
	});
	it('hides site entries only when site is unticked', () => {
		const noOnramp = { ...all, programmes: new Set(programmes.filter((p) => p !== 'nist-onramp')) };
		expect(filterTimeline(items, noOnramp).map((i) => i.entry.description)).toContain('site news');
		const noSite = { ...all, programmes: new Set(programmes.filter((p) => p !== SITE_PROGRAMME)) };
		expect(filterTimeline(items, noSite).map((i) => i.entry.description)).not.toContain('site news');
	});
	it('keeps an entry when any of its programmes is ticked', () => {
		const kpqcOnly = { ...all, programmes: new Set(['kpqc']) };
		expect(filterTimeline(items, kpqcOnly).map((i) => i.entry.type)).toEqual(['attack']);
	});
	it('filters by type', () => {
		const attacks = { ...all, types: new Set<HistoryType>(['attack']) };
		expect(filterTimeline(items, attacks)).toHaveLength(1);
	});
});

describe('timeline URL codec', () => {
	const programmes = ['nist-onramp', 'kpqc', SITE_PROGRAMME];
	const types: HistoryType[] = ['milestone', 'attack', 'update'];
	const all = defaultTimelineFilter(programmes, types);

	it('leaves the defaults out of the URL', () => {
		expect(encodeTimelineFilter(all, programmes, types).toString()).toBe('');
	});
	it('round-trips a narrowed filter in option order', () => {
		const f = { programmes: new Set(['kpqc', 'nist-onramp']), types: new Set<HistoryType>(['update', 'attack']) };
		const p = encodeTimelineFilter(f, programmes, types);
		expect(p.get('p')).toBe('nist-onramp,kpqc');
		expect(p.get('t')).toBe('attack,update');
		expect(decodeTimelineFilter(p, programmes, types)).toEqual(f);
	});
	it('reads a missing param as all', () => {
		expect(decodeTimelineFilter(new URLSearchParams('t=attack'), programmes, types).programmes).toEqual(new Set(programmes));
	});
	it('reads an empty param as none, and encodes none as an empty param', () => {
		const f = decodeTimelineFilter(new URLSearchParams('p='), programmes, types);
		expect(f.programmes.size).toBe(0);
		expect(encodeTimelineFilter(f, programmes, types).toString()).toBe('p=');
	});
	it('ignores unknown values', () => {
		const f = decodeTimelineFilter(new URLSearchParams('p=bogus,kpqc&t=attack,foo,standardization'), programmes, types);
		expect([...f.programmes]).toEqual(['kpqc']);
		// standardization is a real type but not offered (absent from the data), so it is dropped too.
		expect([...f.types]).toEqual(['attack']);
	});
	it('reads only-unknown values as none', () => {
		expect(decodeTimelineFilter(new URLSearchParams('p=bogus'), programmes, types).programmes.size).toBe(0);
	});
});

describe('timelineSummary', () => {
	it('says "all" when nothing is filtered out', () => {
		expect(timelineSummary(28, 28)).toBe('Showing all 28 events');
		expect(timelineSummary(1, 1)).toBe('Showing all 1 event');
	});
	it('gives shown and total otherwise', () => {
		expect(timelineSummary(2, 28)).toBe('Showing 2 of 28 events');
		expect(timelineSummary(0, 28)).toBe('Showing 0 of 28 events');
	});
});
