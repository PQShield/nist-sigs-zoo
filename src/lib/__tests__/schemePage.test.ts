import { describe, expect, it } from 'vitest';
import { flagSummary, hostOf, kemPage, signaturePage, timeCell } from '$lib/schemePage';
import type { HistoryEntry, KemSchemeYaml, ProgrammeYaml, SchemeYaml } from '$lib/types';

const REGISTRY: ProgrammeYaml[] = [
	{ id: 'nist-pqc', name: 'NIST PQC', short: 'NIST', organiser: 'NIST', region: 'US', primitives: ['signature', 'kem'], stages: [{ id: 'standardised', label: 'standard' }] },
	{ id: 'nist-onramp', name: 'On-ramp', short: 'On-ramp', organiser: 'NIST', region: 'US', primitives: ['signature'], stages: [{ id: 'round-1', label: 'round 1' }, { id: 'round-2', label: 'round 2' }] },
	{ id: 'china-ngcc', name: 'NGCC', short: 'NGCC', organiser: 'ICCS', region: 'China', primitives: ['signature', 'kem'], stages: [{ id: 'round-1', label: 'round 1' }] }
];

const SIG: SchemeYaml = {
	name: 'Toy',
	website: 'https://www.toy.example/',
	category: 'Multivariate',
	assumption: 'MQ',
	versions: [
		{
			version: 'v1', date: '2024-01-01', status: 'On-ramp', programmes: { 'nist-onramp': 'round-1' },
			parametersets: [{ name: 'a', level: 1, pk: 100, sig: 50, signing_cycles: 1000, verification_cycles: 500 }]
		},
		{
			version: 'v2', date: '2025-01-01', status: 'On-ramp', programmes: { 'nist-onramp': 'round-2' },
			findings_url: 'https://www.ngcc.dev/r.html',
			parametersets: [
				{ name: 'a', level: 1, pk: 90, sig: 60, signing_us: 2.5, verification_us: 1, warning: 'MQ estimate lowered' },
				{ name: 'b', level: 3, pk: 200, sig: 120, signing_cycles: 25000, verification_cycles: 10000, warning: 'MQ estimate lowered' }
			]
		}
	]
};

const KEM: KemSchemeYaml = {
	name: 'ToyKEM',
	website: 'not a url',
	category: 'Lattices',
	assumption: 'MLWE',
	status: 'Research proposal',
	programmes: { 'china-ngcc': 'round-1' },
	implementation: 'Decapsulation bug (Credit, ngcc.dev).',
	findings_url: 'javascript:alert(1)',
	parametersets: [{ name: 'k1', ngcc_level: 128, pk: 800, ct: 100 }]
};

const HISTORY: HistoryEntry[] = [
	{ date: '2025-01-01', type: 'update', description: 'v2 out', schemes: ['Toy'] },
	{ date: '2025-06-01', type: 'attack', description: 'KEM attack', schemes: ['ToyKEM'] }
];

describe('signaturePage', () => {
	const page = signaturePage(SIG, HISTORY, REGISTRY);

	it('describes the scheme from its newest version', () => {
		expect(page).toMatchObject({
			primitive: 'signature',
			name: 'Toy',
			slug: 'toy',
			websiteHost: 'toy.example',
			latest: { id: '2025-01-01', label: 'v2', date: '2025-01-01' },
			chips: [{ label: 'On-ramp round 2', tone: 'plain', programme: 'nist-onramp' }],
			findings: { url: 'https://www.ngcc.dev/r.html', host: 'ngcc.dev' }
		});
		expect(page.options.map((o) => o.id)).toEqual(['2025-01-01', '2024-01-01']);
		expect(page.versions.map((v) => v.version)).toEqual(['v2', 'v1']);
	});

	it('builds the parameter-set table of the newest version', () => {
		expect(page.table.headers).toEqual(['pk (B)', 'sig (B)', 'pk+sig (B)', 'Sign', 'Verify']);
		expect(page.table.rows.map((r) => [r.name, r.level])).toEqual([['a', '1'], ['b', '3']]);
		expect(page.table.rows[0].cells.map((c) => c.text)).toEqual(['90', '60', '150', '2.5 µs', '1.0 µs']);
		expect(page.table.rows[1].cells[3]).toEqual({ text: '10.0 µs', kind: 'estimated', title: 'Estimated from 25K cycles @ 2.5 GHz' });
		expect(page.table.rows[0].badges.map((b) => b.kind)).toEqual(['warning']);
	});

	it('groups identical flags into one note and summarises them', () => {
		expect(page.flagNotes).toEqual([
			expect.objectContaining({ kind: 'warning', html: 'MQ estimate lowered', sets: ['a', 'b'], allSets: true })
		]);
		expect(page.flagSummary).toBe('Warning (all sets)');
	});

	it('merges the same-day update into the version event', () => {
		expect(page.events.map((e) => [e.date, e.kind, e.title, e.html])).toEqual([
			['2025-01-01', 'version', 'v2', 'v2 out'],
			['2024-01-01', 'version', 'v1', null]
		]);
	});
});

describe('signaturePage flags and versions', () => {
	const set = (extra: object) => ({ name: 'a', level: 1, pk: 1, sig: 1, signing_cycles: 1, verification_cycles: 1, ...extra });
	const mk = (versions: object[]): SchemeYaml =>
		({ name: 'Flagged', website: 'https://x.example/', category: 'c', assumption: 'a', versions }) as SchemeYaml;
	const ver = (date: string, extra: object, sets: object[]) => ({ version: date, date, status: 'On-ramp', ...extra, parametersets: sets });

	it('never yields a broken note for a classical scheme', () => {
		const page = signaturePage(mk([ver('2025-01-01', { broken: 'classical' }, [set({})])]), [], REGISTRY);
		expect(page.flagNotes.map((n) => n.kind)).toEqual(['pre-quantum']);
	});

	it('lets a set-level false override a version-level flag', () => {
		const page = signaturePage(
			mk([ver('2025-01-01', { warning: 'v-level' }, [set({ name: 'a', warning: false }), set({ name: 'b' })])]),
			[],
			REGISTRY
		);
		expect(page.flagNotes).toEqual([expect.objectContaining({ kind: 'warning', sets: ['b'], allSets: false })]);
	});

	it('takes findings from the latest version only', () => {
		const page = signaturePage(
			mk([
				ver('2024-01-01', { findings_url: 'https://old.example/r' }, [set({})]),
				ver('2025-01-01', {}, [set({})])
			]),
			[],
			REGISTRY
		);
		expect(page.findings).toBeNull();
	});

	it('rejects a scheme without versions', () => {
		expect(() => signaturePage(mk([]), [], REGISTRY)).toThrow(/no versions/);
	});

	it('drops non-http badge links', () => {
		const page = signaturePage(
			mk([ver('2025-01-01', { implementation: 'bug', findings_url: 'javascript:alert(1)' }, [set({})])]),
			[],
			REGISTRY
		);
		expect(page.flagNotes[0].href).toBeNull();
		expect(page.table.rows[0].badges[0].href).toBeNull();
	});
});

describe('kemPage', () => {
	const page = kemPage(KEM, HISTORY, REGISTRY);

	it('has programmes, status, an NGCC-scale table and no versions', () => {
		expect(page).toMatchObject({ primitive: 'kem', slug: 'toykem', status: 'Research proposal', websiteHost: null, findings: null });
		expect(page).not.toHaveProperty('versions');
		expect(page.chips).toEqual([{ label: 'NGCC round 1', tone: 'new', programme: 'china-ngcc' }]);
		expect(page.table.headers).toEqual(['pk (B)', 'ct (B)', 'pk+ct (B)', 'Keygen', 'Encaps', 'Decaps']);
		expect(page.table.rows[0].level).toBe('128-bit');
		expect(page.table.rows[0].cells.slice(3).map((c) => c.kind)).toEqual(['missing', 'missing', 'missing']);
	});

	it('never links a non-http findings URL', () => {
		expect(page.flagNotes).toEqual([expect.objectContaining({ kind: 'implementation', href: null, allSets: true })]);
		expect(page.flagSummary).toBe('Implementation findings (all sets)');
	});

	it('shows only the history entries naming the KEM', () => {
		expect(page.events.map((e) => e.html)).toEqual(['KEM attack']);
	});
});

describe('flagSummary', () => {
	it('says there are no flags, counts partial flags, and ignores notes', () => {
		expect(flagSummary([], 4)).toBe('None on current sets');
		expect(flagSummary([{ kind: 'note', title: 'Note', html: 'n', href: null, sets: ['a'], allSets: false }], 4)).toBe('None on current sets');
		expect(
			flagSummary(
				[
					{ kind: 'broken', title: 'Broken', html: 'x', href: null, sets: ['two'], allSets: false },
					{ kind: 'warning', title: 'Warning', html: 'y', href: null, sets: ['one'], allSets: false },
					{ kind: 'warning', title: 'Warning', html: 'z', href: null, sets: ['one', 'three'], allSets: false }
				],
				4
			)
		).toBe('Broken (1 of 4 sets) · Warning (2 of 4 sets)');
	});
});

describe('hostOf / timeCell', () => {
	it('strips www and rejects non-http URLs', () => {
		expect(hostOf('https://www.pqmayo.org/x')).toBe('pqmayo.org');
		expect(hostOf('ftp://a.example')).toBeNull();
		expect(hostOf('nonsense')).toBeNull();
	});

	it('marks unbenchmarked cells', () => {
		expect(timeCell(null, 0)).toEqual({ text: '—', kind: 'missing', title: 'Not yet benchmarked' });
	});
});
