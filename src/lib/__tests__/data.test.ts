import { describe, it, expect } from 'vitest';
import { processYamlSchemes } from '$lib/data';
import { CPUSPEED } from '$lib/constants';
import type { SchemeYaml } from '$lib/types';

function scheme(overrides: Partial<SchemeYaml> = {}): SchemeYaml {
	return {
		name: 'TestScheme',
		website: 'https://example.com',
		category: 'Lattices',
		assumption: 'LWE',
		versions: [],
		...overrides,
	};
}

const BASE_PS = {
	name: 'Test-128',
	level: 2 as const,
	pk: 1000,
	sig: 2000,
	signing_cycles: 500_000,
	verification_cycles: 100_000,
};

describe('processYamlSchemes', () => {
	it('computes pkPlusSig', () => {
		const data = [scheme({ versions: [{ version: 'v1', date: '2024-01-01', status: 'On-ramp', parametersets: [BASE_PS] }] })];
		const { parameterSets } = processYamlSchemes(data);
		expect(parameterSets[0].pkPlusSig).toBe(3000);
	});

	it('uses signing_cycles directly when present', () => {
		const data = [scheme({ versions: [{ version: 'v1', date: '2024-01-01', status: 'On-ramp', parametersets: [BASE_PS] }] })];
		const { parameterSets } = processYamlSchemes(data);
		expect(parameterSets[0].signingCycles).toBe(500_000);
		expect(parameterSets[0].extrapolated).toBe(false);
	});

	it('extrapolates cycles from signing_us when no signing_cycles', () => {
		const ps = { name: 'Test-128', level: 2 as const, pk: 1000, sig: 2000, signing_us: 400, verification_us: 100 };
		const data = [scheme({ versions: [{ version: 'v1', date: '2024-01-01', status: 'On-ramp', parametersets: [ps] }] })];
		const { parameterSets } = processYamlSchemes(data);
		expect(parameterSets[0].signingCycles).toBe(Math.round((CPUSPEED * 400) / 1_000_000));
		expect(parameterSets[0].extrapolated).toBe(true);
	});

	it('picks latest version by date when no tagFilter', () => {
		const data = [scheme({
			versions: [
				{ version: 'v1', date: '2023-01-01', status: 'On-ramp', parametersets: [{ ...BASE_PS, pk: 900 }] },
				{ version: 'v2', date: '2024-06-01', status: 'On-ramp', parametersets: [{ ...BASE_PS, pk: 1200 }] },
			],
		})];
		const { parameterSets } = processYamlSchemes(data);
		expect(parameterSets[0].pk).toBe(1200);
	});

	it('tagFilter includes only matching versions', () => {
		const data = [
			scheme({ name: 'R2Scheme', versions: [{ version: 'v1', date: '2024-01-01', status: 'On-ramp', tags: ['round-2'], parametersets: [BASE_PS] }] }),
			scheme({ name: 'R3Only', versions: [{ version: 'v1', date: '2024-01-01', status: 'On-ramp', tags: ['round-3'], parametersets: [BASE_PS] }] }),
		];
		const { schemes } = processYamlSchemes(data, 'round-2');
		expect(schemes.map(s => s.scheme)).toContain('R2Scheme');
		expect(schemes.map(s => s.scheme)).not.toContain('R3Only');
	});

	it('always includes untagged reference schemes regardless of tagFilter', () => {
		const data = [
			scheme({ name: 'Untagged', versions: [{ version: 'v1', date: '2024-01-01', status: 'Standardized', parametersets: [BASE_PS] }] }),
		];
		const { schemes } = processYamlSchemes(data, 'round-2');
		expect(schemes.map(s => s.scheme)).toContain('Untagged');
	});

	it('useLatestVersion shows newest version even when older version has the tag', () => {
		const data = [scheme({
			versions: [
				{ version: 'new', date: '2025-01-01', status: 'On-ramp', parametersets: [{ ...BASE_PS, pk: 999 }] },
				{ version: 'old', date: '2023-01-01', status: 'On-ramp', tags: ['round-2'], parametersets: [{ ...BASE_PS, pk: 1111 }] },
			],
		})];
		const { parameterSets } = processYamlSchemes(data, 'round-2', { useLatestVersion: true });
		expect(parameterSets[0].pk).toBe(999);
	});

	it('broken flag at version level propagates to parameterset', () => {
		const data = [scheme({ versions: [{ version: 'v1', date: '2024-01-01', status: 'On-ramp', broken: 'key recovery attack', parametersets: [BASE_PS] }] })];
		const { parameterSets } = processYamlSchemes(data);
		expect(parameterSets[0].broken).toBe('key recovery attack');
	});

	it('broken flag at parameterset level overrides version level', () => {
		const data = [scheme({ versions: [{
			version: 'v1', date: '2024-01-01', status: 'On-ramp', broken: 'version-level',
			parametersets: [{ ...BASE_PS, broken: 'ps-level' }],
		}] })];
		const { parameterSets } = processYamlSchemes(data);
		expect(parameterSets[0].broken).toBe('ps-level');
	});

	it('computes correct ranges', () => {
		const data = [scheme({
			versions: [{
				version: 'v1', date: '2024-01-01', status: 'On-ramp',
				parametersets: [
					{ ...BASE_PS, name: 'A', pk: 500, sig: 1000, signing_cycles: 200_000, verification_cycles: 50_000 },
					{ ...BASE_PS, name: 'B', pk: 2000, sig: 4000, signing_cycles: 800_000, verification_cycles: 200_000 },
				],
			}],
		})];
		const { ranges } = processYamlSchemes(data);
		expect(ranges.pk).toEqual([500, 2000]);
		expect(ranges.sig).toEqual([1000, 4000]);
		expect(ranges.pkPlusSig).toEqual([1500, 6000]);
		expect(ranges.signingCycles).toEqual([200_000, 800_000]);
	});

	it('skips schemes with no versions', () => {
		const data = [scheme({ versions: [] })];
		const { schemes } = processYamlSchemes(data);
		expect(schemes).toHaveLength(0);
	});

	it('Pre-Quantum level parsed correctly', () => {
		const ps = { ...BASE_PS, level: 'Pre-Quantum' as const };
		const data = [scheme({ versions: [{ version: 'v1', date: '2024-01-01', status: 'On-ramp', parametersets: [ps] }] })];
		const { parameterSets } = processYamlSchemes(data);
		expect(parameterSets[0].level).toBe('Pre-Quantum');
	});
});
