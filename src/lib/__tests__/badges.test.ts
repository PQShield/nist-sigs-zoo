import { describe, expect, it } from 'vitest';
import { programmeChips, securityBadges } from '$lib/badges';
import type { ProgrammeYaml } from '$lib/types';

const CLEAN = {
	classical: false,
	broken: false as const,
	implementation: false as const,
	warning: false as const,
	info: false as const,
	findingsUrl: null
};

describe('securityBadges', () => {
	it('returns nothing for a row without flags', () => {
		expect(securityBadges(CLEAN)).toEqual([]);
	});

	it('keeps a design break and implementation findings as separate badges, in order', () => {
		const badges = securityBadges({
			...CLEAN,
			broken: 'forgery',
			implementation: 'stack write',
			findingsUrl: 'https://example.org/r.html'
		});
		expect(badges.map((b) => b.kind)).toEqual(['broken', 'implementation']);
		expect(badges[0]).toMatchObject({ title: 'Broken', html: 'forgery', href: null });
		expect(badges[1]).toMatchObject({
			title: 'Implementation findings',
			html: 'stack write',
			href: 'https://example.org/r.html'
		});
	});

	it('links implementation findings only when a report URL exists', () => {
		expect(securityBadges({ ...CLEAN, implementation: 'bug' })[0].href).toBeNull();
	});

	it('shows pre-quantum schemes as pre-quantum, not broken', () => {
		const badges = securityBadges({ ...CLEAN, classical: true, broken: 'classical' });
		expect(badges.map((b) => b.kind)).toEqual(['pre-quantum']);
		expect(badges[0].html).not.toContain('classical<');
	});

	it('lists warning and note after the others', () => {
		expect(securityBadges({ ...CLEAN, warning: 'w', info: 'n' }).map((b) => b.kind)).toEqual(['warning', 'note']);
	});
});

const REGISTRY: ProgrammeYaml[] = [
	{
		id: 'nist-pqc',
		name: 'NIST PQC',
		short: 'NIST',
		organiser: 'NIST',
		region: 'US',
		primitives: ['signature', 'kem'],
		stages: [
			{ id: 'round-3', label: 'round 3' },
			{ id: 'selected', label: 'selected' },
			{ id: 'standardised', label: 'standard' }
		]
	},
	{
		id: 'nist-onramp',
		name: 'On-ramp',
		short: 'On-ramp',
		organiser: 'NIST',
		region: 'US',
		primitives: ['signature'],
		stages: [
			{ id: 'round-1', label: 'round 1' },
			{ id: 'round-3', label: 'round 3' }
		]
	},
	{
		id: 'china-ngcc',
		name: 'NGCC',
		short: 'NGCC',
		organiser: 'ICCS',
		region: 'CN',
		primitives: ['signature', 'kem'],
		stages: [{ id: 'round-1', label: 'round 1' }]
	},
	{
		id: 'kpqc',
		name: 'KpqC',
		short: 'KpqC',
		organiser: 'KpqC',
		region: 'KR',
		primitives: ['signature', 'kem'],
		stages: [{ id: 'selected', label: 'selected' }]
	},
	{
		id: 'iso',
		name: 'ISO',
		short: 'ISO',
		organiser: 'ISO/IEC',
		region: 'Intl',
		primitives: ['kem'],
		stages: [{ id: 'in-progress', label: 'in progress' }]
	}
];

describe('programmeChips', () => {
	it('shows a FIPS standard by its FIPS number instead of the NIST membership', () => {
		expect(
			programmeChips(
				{ status: 'FIPS', version: 'FIPS 204', programmes: [{ programme: 'nist-pqc', stage: 'standardised' }] },
				REGISTRY
			)
		).toEqual([{ label: 'FIPS 204', tone: 'standard', programme: 'nist-pqc' }]);
	});

	it('falls back to "FIPS" without a FIPS version (KEMs)', () => {
		expect(
			programmeChips({ status: 'FIPS', programmes: [{ programme: 'nist-pqc', stage: 'standardised' }] }, REGISTRY)
		).toEqual([{ label: 'FIPS', tone: 'standard', programme: 'nist-pqc' }]);
	});

	it('marks a selected scheme as Std pending', () => {
		expect(
			programmeChips(
				{ status: 'To be standardized', programmes: [{ programme: 'nist-pqc', stage: 'selected' }] },
				REGISTRY
			)
		).toEqual([{ label: 'Std pending', tone: 'standard', programme: 'nist-pqc' }]);
	});

	it('lists every other programme membership', () => {
		expect(
			programmeChips(
				{
					status: 'On-ramp',
					programmes: [
						{ programme: 'nist-onramp', stage: 'round-1' },
						{ programme: 'kpqc', stage: 'selected' }
					]
				},
				REGISTRY
			)
		).toEqual([
			{ label: 'On-ramp R1', tone: 'plain', programme: 'nist-onramp' },
			{ label: 'KpqC selected', tone: 'plain', programme: 'kpqc' }
		]);
		expect(
			programmeChips(
				{
					status: 'ISO standardisation',
					programmes: [
						{ programme: 'nist-pqc', stage: 'round-3' },
						{ programme: 'iso', stage: 'in-progress' }
					]
				},
				REGISTRY
			)
		).toEqual([
			{ label: 'NIST R3', tone: 'plain', programme: 'nist-pqc' },
			{ label: 'ISO in progress', tone: 'plain', programme: 'iso' }
		]);
	});

	it('shows only the programme name for a stage with an empty chip', () => {
		const reg = [
			...REGISTRY.filter((p) => p.id !== 'iso'),
			{ ...REGISTRY.find((p) => p.id === 'iso')!, stages: [{ id: 'published', label: 'published', chip: '' }] }
		];
		expect(
			programmeChips({ status: 'ISO standard', programmes: [{ programme: 'iso', stage: 'published' }] }, reg)
		).toEqual([{ label: 'ISO', tone: 'plain', programme: 'iso' }]);
	});

	it('shows NGCC memberships like any other programme', () => {
		expect(
			programmeChips({ status: 'NGCC', programmes: [{ programme: 'china-ngcc', stage: 'round-1' }] }, REGISTRY)
		).toEqual([{ label: 'NGCC R1', tone: 'plain', programme: 'china-ngcc' }]);
	});

	it('labels schemes outside every programme', () => {
		expect(programmeChips({ status: 'Classic cryptography', programmes: [] }, REGISTRY)).toEqual([
			{ label: 'Classic', tone: 'plain' }
		]);
		expect(programmeChips({ status: 'Academic', programmes: [] }, REGISTRY)).toEqual([
			{ label: 'Research', tone: 'plain' }
		]);
	});
});
