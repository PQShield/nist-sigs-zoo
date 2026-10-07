import { describe, it, expect } from 'vitest';
import {
	versionProgrammes,
	schemeProgrammes,
	isReferenceScheme,
	membershipLabel,
	programmeSummaries
} from '$lib/programmes';
import type { KemSchemeYaml, ProgrammeYaml, SchemeYaml, VersionYaml } from '$lib/types';

const PROGRAMMES: ProgrammeYaml[] = [
	{ id: 'nist-pqc', name: 'NIST PQC', short: 'NIST', organiser: 'NIST', region: 'US', primitives: ['signature', 'kem'],
	  stages: [{ id: 'standardised', label: 'standard' }, { id: 'round-3', label: 'round 3' }] },
	{ id: 'nist-onramp', name: 'On-ramp', short: 'On-ramp', organiser: 'NIST', region: 'US', primitives: ['signature'],
	  stages: [{ id: 'round-1', label: 'round 1' }, { id: 'round-3', label: 'round 3' }] },
	{ id: 'kpqc', name: 'KpqC', short: 'KpqC', organiser: 'KpqC', region: 'KR', primitives: ['signature', 'kem'],
	  stages: [{ id: 'selected', label: 'selected' }] }
];

function v(date: string, programmes?: Record<string, string | string[]>): VersionYaml {
	return { version: date, date, status: 'On-ramp', programmes, parametersets: [] };
}

describe('versionProgrammes', () => {
	it('normalises scalar stages to lists', () => {
		expect(versionProgrammes(v('2024-01-01', { 'nist-onramp': 'round-1' }))).toEqual({ 'nist-onramp': ['round-1'] });
	});
	it('keeps list stages', () => {
		expect(versionProgrammes(v('2025-02-05', { 'nist-onramp': ['round-2', 'round-3'] }))).toEqual({ 'nist-onramp': ['round-2', 'round-3'] });
	});
	it('returns an empty map when absent', () => {
		expect(versionProgrammes(v('2024-01-01'))).toEqual({});
	});
});

describe('schemeProgrammes', () => {
	it('takes the stage from the newest version in each programme', () => {
		const versions = [
			v('2023-01-01', { 'nist-onramp': 'round-1' }),
			v('2026-05-01', { 'nist-onramp': 'round-3' }),
			v('2026-09-01')
		];
		expect(schemeProgrammes(versions)).toEqual([{ programme: 'nist-onramp', stage: 'round-3' }]);
	});
	it('reports the last listed stage when a version has several', () => {
		expect(schemeProgrammes([v('2025-02-05', { 'nist-onramp': ['round-2', 'round-3'] })])).toEqual([
			{ programme: 'nist-onramp', stage: 'round-3' }
		]);
	});
	it('lists every programme a scheme was in', () => {
		const versions = [v('2023-05-31', { 'nist-onramp': 'round-1', kpqc: 'selected' })];
		expect(schemeProgrammes(versions)).toEqual([
			{ programme: 'nist-onramp', stage: 'round-1' },
			{ programme: 'kpqc', stage: 'selected' }
		]);
	});
	it('is empty for a scheme in no programme', () => {
		expect(schemeProgrammes([v('2020-01-01')])).toEqual([]);
	});
	it('kems without programmes resolve to an empty list', () => {
		const kems: KemSchemeYaml[] = [{ name: 'BAT', website: '', category: 'Lattices', assumption: '', status: '', parametersets: [] }];
		expect(programmeSummaries(PROGRAMMES, [], kems).every((s) => s.kems.length === 0)).toBe(true);
	});
});

describe('isReferenceScheme', () => {
	it('is true for a scheme in no programme (pre-quantum)', () => {
		expect(isReferenceScheme([v('2020-01-01')])).toBe(true);
	});
	it('is true for a NIST standard', () => {
		expect(isReferenceScheme([v('2024-08-13', { 'nist-pqc': 'standardised' })])).toBe(true);
	});
	it('is false for an on-ramp candidate', () => {
		expect(isReferenceScheme([v('2024-01-01', { 'nist-onramp': 'round-1' })])).toBe(false);
	});
	it('is false for a KpqC-only scheme', () => {
		expect(isReferenceScheme([v('2024-01-01', { kpqc: 'selected' })])).toBe(false);
	});
});

describe('membershipLabel', () => {
	it('combines short name and stage label', () => {
		expect(membershipLabel({ programme: 'nist-onramp', stage: 'round-3' }, PROGRAMMES)).toBe('On-ramp round 3');
	});
	it('throws on an unknown programme', () => {
		expect(() => membershipLabel({ programme: 'nope', stage: 'x' }, PROGRAMMES)).toThrow(/unknown programme "nope"/);
	});
	it('throws on an unknown stage', () => {
		expect(() => membershipLabel({ programme: 'kpqc', stage: 'round-9' }, PROGRAMMES)).toThrow(/unknown stage "round-9"/);
	});
});

describe('programmeSummaries', () => {
	it('lists signature and KEM members per programme, sorted', () => {
		const sigs: SchemeYaml[] = [
			{ name: 'MAYO', website: '', category: 'Multivariate', assumption: '', versions: [v('2026-08-31', { 'nist-onramp': 'round-3' })] },
			{ name: 'HAETAE', website: '', category: 'Lattices', assumption: '', versions: [v('2023-05-31', { 'nist-onramp': 'round-1', kpqc: 'selected' })] },
			{ name: 'AIMer', website: '', category: 'Symmetric', assumption: '', versions: [v('2023-05-31', { 'nist-onramp': 'round-1', kpqc: 'selected' })] }
		];
		const kems: KemSchemeYaml[] = [
			{ name: 'SMAUG-T', website: '', category: 'Lattices', assumption: '', status: '', programmes: { kpqc: 'selected' }, parametersets: [] }
		];
		const kpqc = programmeSummaries(PROGRAMMES, sigs, kems).find((s) => s.programme.id === 'kpqc')!;
		expect(kpqc.signatures).toEqual(['AIMer', 'HAETAE']);
		expect(kpqc.kems).toEqual(['SMAUG-T']);
	});
});
