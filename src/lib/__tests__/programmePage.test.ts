import { describe, it, expect } from 'vitest';
import {
	anyKemTimings,
	anySigTimings,
	compareHref,
	CURRENT_VIEW,
	currentStage,
	findingsSummary,
	focusTarget,
	formatDate,
	kemMembers,
	ngccTargets,
	programmeCards,
	programmeFacts,
	programmeKemRows,
	programmeSignatureRows,
	signatureMembers,
	stageViews,
	unmappedAtTarget,
	type FindingRow
} from '$lib/programmePage';
import { defaultCommonFilter } from '$lib/compareFilters';
import type {
	KemSchemeYaml,
	ParameterSetYaml,
	ProgrammeYaml,
	SchemeYaml,
	VersionYaml
} from '$lib/types';

function ver(
	date: string,
	programmes: Record<string, string | string[]> | undefined,
	sets: Partial<ParameterSetYaml>[] = [{}],
	extra: Partial<VersionYaml> = {}
): VersionYaml {
	return {
		version: date,
		date,
		status: 'On-ramp',
		programmes,
		parametersets: sets.map((p, i) => ({ name: `${date}-${i}`, pk: 10, sig: 20, ...p })),
		...extra
	};
}

function sig(name: string, versions: VersionYaml[]): SchemeYaml {
	return { name, website: 'https://example.org', category: 'Lattices', assumption: 'x', versions };
}

function kem(name: string, programmes?: Record<string, string | string[]>): KemSchemeYaml {
	return {
		name,
		website: 'https://example.org',
		category: 'Lattices',
		assumption: 'x',
		status: 'Candidate',
		programmes,
		parametersets: [{ name: `${name}-1`, pk: 800, ct: 700, ngcc_level: 128 }]
	};
}

const ONRAMP: ProgrammeYaml = {
	id: 'nist-onramp', name: 'Additional signatures', short: 'On-ramp', organiser: 'NIST', region: 'US',
	primitives: ['signature'],
	stages: [{ id: 'round-1', label: 'round 1' }, { id: 'round-2', label: 'round 2' }, { id: 'round-3', label: 'round 3' }]
};

const NGCC: ProgrammeYaml = {
	id: 'china-ngcc', name: 'Next-generation commercial cryptographic algorithms', short: 'NGCC',
	organiser: 'ICCS', region: 'China', primitives: ['signature', 'kem'],
	stages: [{ id: 'round-1', label: 'round 1' }],
	organiser_detail: 'ICCS, Beijing',
	out_of_scope: '9 key exchange, 35 hash functions',
	official_counts: { signature: 34, kem: 41 },
	timeline: [
		{ date: '2026-06-30', label: 'Submissions closed' },
		{ date: '2026-09-20', label: 'Round 1', current: true },
		{ label: 'Round 2', note: 'not announced' }
	]
};

const KPQC: ProgrammeYaml = {
	id: 'kpqc', name: 'KpqC', short: 'KpqC', organiser: 'KpqC', region: 'South Korea',
	primitives: ['signature', 'kem'], stages: [{ id: 'selected', label: 'selected' }]
};

const ISO: ProgrammeYaml = {
	id: 'iso', name: 'ISO/IEC 18033-2 amendment', short: 'ISO', organiser: 'ISO/IEC', region: 'International',
	primitives: ['kem'], stages: [{ id: 'in-progress', label: 'in progress' }],
	timeline: [{ label: 'Amendment in preparation', current: true }]
};

const MAYO = sig('MAYO', [
	ver('2023-06-01', { 'nist-onramp': 'round-1' }),
	ver('2025-02-05', { 'nist-onramp': 'round-2' }),
	ver('2026-08-01', undefined)
]);
const THREEWISE = sig('3WISE', [ver('2023-06-01', { 'nist-onramp': 'round-1' })]);
const MLDSA = sig('ML-DSA', [ver('2024-08-13', { 'nist-pqc': 'standardised' })]);
const AIMER = sig('AIMer', [ver('2023-06-01', { 'nist-onramp': 'round-1', kpqc: 'selected' })]);
const SIGS = [MAYO, THREEWISE, MLDSA, AIMER];

describe('members', () => {
	it('selects signature schemes with a version in the programme', () => {
		expect(signatureMembers('nist-onramp', SIGS).map((s) => s.name)).toEqual(['MAYO', '3WISE', 'AIMer']);
		expect(signatureMembers('kpqc', SIGS).map((s) => s.name)).toEqual(['AIMer']);
		expect(signatureMembers('china-ngcc', SIGS)).toEqual([]);
	});
	it('selects KEMs by their scheme-level memberships', () => {
		const kems = [kem('A', { kpqc: 'selected' }), kem('B', { 'nist-pqc': 'round-3' }), kem('C')];
		expect(kemMembers('kpqc', kems).map((k) => k.name)).toEqual(['A']);
		expect(kemMembers('iso', kems)).toEqual([]);
	});
});

describe('stageViews', () => {
	it('offers latest specs plus each round with recorded versions, most advanced first', () => {
		expect(stageViews(ONRAMP, SIGS)).toEqual([
			{ id: CURRENT_VIEW, label: 'All, latest specs' },
			{ id: 'round-2', label: 'Round 2' },
			{ id: 'round-1', label: 'Round 1' }
		]);
	});
	it('offers nothing when fewer than two rounds have versions', () => {
		const ngccSig = sig('BiT', [ver('2026-09-20', { 'china-ngcc': 'round-1' })]);
		expect(stageViews(NGCC, [ngccSig])).toEqual([]);
		expect(stageViews(NGCC, [])).toEqual([]);
	});
	it('ignores non-round stages such as "selected"', () => {
		expect(stageViews(KPQC, SIGS)).toEqual([]);
	});
});

describe('programmeSignatureRows', () => {
	it('shows every member at its newest version in the current view', () => {
		const rows = programmeSignatureRows('nist-onramp', SIGS, CURRENT_VIEW).parameterSets;
		expect(rows.map((r) => [r.scheme, r.version])).toEqual([
			['MAYO', '2026-08-01'],
			['3WISE', '2023-06-01'],
			['AIMer', '2023-06-01']
		]);
	});
	it('shows only that round’s entrants, pinned to the version recorded at the round', () => {
		const r1 = programmeSignatureRows('nist-onramp', SIGS, 'round-1').parameterSets;
		expect(r1.map((r) => [r.scheme, r.version])).toEqual([
			['MAYO', '2023-06-01'],
			['3WISE', '2023-06-01'],
			['AIMer', '2023-06-01']
		]);
		const r2 = programmeSignatureRows('nist-onramp', SIGS, 'round-2').parameterSets;
		expect(r2.map((r) => [r.scheme, r.version])).toEqual([['MAYO', '2025-02-05']]);
	});
	it('never includes reference schemes', () => {
		const rows = programmeSignatureRows('nist-onramp', SIGS, 'round-1').parameterSets;
		expect(rows.some((r) => r.scheme === 'ML-DSA')).toBe(false);
	});
	it('returns no rows for a programme without members', () => {
		expect(programmeSignatureRows('china-ngcc', SIGS, CURRENT_VIEW).parameterSets).toEqual([]);
		expect(programmeSignatureRows('china-ngcc', [], CURRENT_VIEW).parameterSets).toEqual([]);
		for (const view of [CURRENT_VIEW, 'round-1']) {
			const { ranges } = programmeSignatureRows('china-ngcc', SIGS, view);
			for (const r of Object.values(ranges)) {
				if (r !== null) expect(r.every(Number.isFinite)).toBe(true);
			}
		}
	});
});

describe('programmeKemRows', () => {
	it('flattens the members’ parameter sets', () => {
		const rows = programmeKemRows('kpqc', [kem('A', { kpqc: 'selected' }), kem('B')]);
		expect(rows.map((r) => r.parameterset)).toEqual(['A-1']);
	});
	it('is empty without members', () => {
		expect(programmeKemRows('china-ngcc', [])).toEqual([]);
	});
});

describe('NGCC targets', () => {
	it('lists the targets present, in scale order', () => {
		expect(ngccTargets([{ ngccLevel: 512 }, { ngccLevel: null }, { ngccLevel: 128 }, { ngccLevel: 512 }])).toEqual([128, 512]);
		expect(ngccTargets([])).toEqual([]);
	});
	it('names schemes without a recommended set at a target', () => {
		const rows = [
			{ scheme: 'X', ngccLevel: 128 as const, recommended: true },
			{ scheme: 'Y', ngccLevel: 256 as const, recommended: true },
			{ scheme: 'Z', ngccLevel: 128 as const, recommended: false },
			{ scheme: 'Z', ngccLevel: 256 as const, recommended: true }
		];
		expect(unmappedAtTarget(rows, 128)).toEqual(['Y', 'Z']);
		expect(unmappedAtTarget(rows, 256)).toEqual(['X']);
	});
	it('focuses a filter on one target without touching NIST levels or mutating the input', () => {
		const f = defaultCommonFilter(['china-ngcc']);
		const focused = focusTarget(f, 256);
		expect([...focused.ngccLevels]).toEqual([256]);
		expect(focused.levels).toEqual(f.levels);
		expect(f.ngccLevels.size).toBe(4);
		expect(focusTarget(f, null)).toBe(f);
	});
});

describe('timings presence', () => {
	const base = { signingUs: null, verificationUs: null, signingCycles: 0, verificationCycles: 0 };
	it('detects signature timings, measured or from cycles', () => {
		expect(anySigTimings([])).toBe(false);
		expect(anySigTimings(programmeSignatureRows('nist-onramp', SIGS, CURRENT_VIEW).parameterSets)).toBe(false);
		const rows = programmeSignatureRows('nist-onramp', SIGS, CURRENT_VIEW).parameterSets;
		expect(anySigTimings([{ ...rows[0], ...base, signingCycles: 1000 }])).toBe(true);
	});
	it('detects KEM timings', () => {
		const rows = programmeKemRows('kpqc', [kem('A', { kpqc: 'selected' })]);
		expect(anyKemTimings(rows)).toBe(false);
		expect(anyKemTimings([{ ...rows[0], encapsUs: 12 }])).toBe(true);
	});
});

describe('findingsSummary', () => {
	const row = (scheme: string, f: Partial<FindingRow> = {}): FindingRow => ({
		scheme, classical: false, broken: false, implementation: false, warning: false, findingsUrl: null, ...f
	});
	it('counts each candidate once, under its worst finding', () => {
		const s = findingsSummary([
			row('A', { broken: 'design break at 512-bit' }),
			row('A', { implementation: 'bug', findingsUrl: 'https://ngcc.dev/reports/sign-01.html' }),
			row('A', { findingsUrl: 'https://ngcc.dev/reports/sign-01.html' }),
			row('B', { implementation: 'bug', findingsUrl: 'https://ngcc.dev/reports/sign-02.html' }),
			row('B', { warning: 'heads-up' }),
			row('C', { warning: 'heads-up' }),
			row('D')
		]);
		expect(s).toEqual({ candidates: 4, design: 1, implementation: 1, warning: 1, none: 1, reports: 2 });
	});
	it('does not count a pre-quantum scheme as broken by design', () => {
		expect(findingsSummary([row('RSA', { classical: true, broken: 'classical' })])).toEqual({
			candidates: 1, design: 0, implementation: 0, warning: 0, none: 1, reports: 0
		});
		expect(
			findingsSummary([row('RSA', { classical: true, broken: 'classical', implementation: 'bug' })])
		).toEqual({ candidates: 1, design: 0, implementation: 1, warning: 0, none: 0, reports: 0 });
	});
	it('is all zeros without candidates', () => {
		expect(findingsSummary([])).toEqual({ candidates: 0, design: 0, implementation: 0, warning: 0, none: 0, reports: 0 });
	});
});

describe('compareHref', () => {
	it('links to the compare page filtered to the programme, keeping the base', () => {
		expect(compareHref('/nist-sigs-zoo', 'china-ngcc', 'signature', { target: 128 })).toBe(
			'/nist-sigs-zoo/signatures/?p=china-ngcc&nl=128'
		);
		expect(compareHref('', 'kpqc', 'kem')).toBe('/kems/?p=kpqc');
	});
	it('maps NIST PQC onto both of its filter groups', () => {
		expect(compareHref('', 'nist-pqc', 'kem')).toBe('/kems/?p=nist-standards%2Cnist-pqc-rounds');
	});
	it('carries the on-ramp round for signatures only', () => {
		expect(compareHref('', 'nist-onramp', 'signature', { stage: 'round-2' })).toBe('/signatures/?p=nist-onramp&r=2');
		expect(compareHref('', 'nist-onramp', 'signature', { stage: CURRENT_VIEW })).toBe('/signatures/?p=nist-onramp');
		expect(compareHref('', 'china-ngcc', 'signature', { stage: 'round-1', target: null })).toBe('/signatures/?p=china-ngcc');
	});
});

describe('dates and facts', () => {
	it('formats day, month and year precision without a locale', () => {
		expect(formatDate('2026-09-20')).toBe('20 Sep 2026');
		expect(formatDate('2025-01')).toBe('Jan 2025');
		expect(formatDate('2021')).toBe('2021');
		expect(formatDate('soon')).toBe('soon');
		expect(formatDate('2025-13')).toBe('2025-13');
	});
	it('finds the current timeline entry', () => {
		expect(currentStage(NGCC)?.label).toBe('Round 1');
		expect(currentStage(KPQC)).toBeNull();
	});
	it('builds the facts box with official and tracked counts', () => {
		expect(programmeFacts(NGCC, { signature: 0, kem: 41 })).toEqual([
			{ term: 'Organiser', detail: 'ICCS, Beijing' },
			{ term: 'Region', detail: 'China' },
			{ term: 'Stage', detail: 'Round 1, since 20 Sep 2026' },
			{ term: 'Signatures', detail: '34 candidates · 0 in our data' },
			{ term: 'KEMs', detail: '41 candidates' },
			{ term: 'Not tracked', detail: '9 key exchange, 35 hash functions' }
		]);
	});
	it('labels the official count when our data spans more rounds', () => {
		const onramp = { ...NGCC, official_counts: { signature: 9 } };
		const f = programmeFacts(onramp, { signature: 41, kem: 0 }).find((x) => x.term === 'Signatures');
		expect(f?.detail).toBe('9 candidates (official) · 41 entrants across all rounds in our data');
	});
	it('falls back to the organiser, an undated stage and plain counts', () => {
		expect(programmeFacts(ISO, { signature: 0, kem: 1 })).toEqual([
			{ term: 'Organiser', detail: 'ISO/IEC' },
			{ term: 'Region', detail: 'International' },
			{ term: 'Stage', detail: 'Amendment in preparation' },
			{ term: 'KEMs', detail: '1 scheme' }
		]);
	});
	it('builds index cards, with null counts for primitives out of scope', () => {
		const cards = programmeCards([NGCC, ISO], SIGS, [kem('F', { iso: 'in-progress' })]);
		expect(cards.map((c) => [c.programme.id, c.stage, c.signatures, c.kems])).toEqual([
			['china-ngcc', 'Round 1, since 20 Sep 2026', 0, 0],
			['iso', 'Amendment in preparation', null, 1]
		]);
	});
});
