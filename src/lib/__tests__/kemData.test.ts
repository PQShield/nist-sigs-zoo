import { describe, it, expect } from 'vitest';
import { processKems } from '$lib/kemData';
import { CPUSPEED } from '$lib/constants';
import type { KemYaml } from '$lib/types';

function kem(overrides: Partial<KemYaml> = {}): KemYaml {
	return {
		name: 'TestKem',
		website: 'https://example.com',
		category: 'Lattices',
		assumption: 'MLWE',
		status: 'On-ramp',
		parametersets: [],
		...overrides,
	};
}

const BASE_PS = {
	name: 'Test-128',
	level: 1 as const,
	pk: 800,
	ct: 768,
	sk: 1632,
};

describe('processKems', () => {
	it('computes pkPlusCt', () => {
		const data = [kem({ parametersets: [BASE_PS] })];
		const { parameterSets } = processKems(data);
		expect(parameterSets[0].pkPlusCt).toBe(1568);
	});

	it('keeps pk, ct and sk values', () => {
		const data = [kem({ parametersets: [BASE_PS] })];
		const { parameterSets } = processKems(data);
		expect(parameterSets[0].pk).toBe(800);
		expect(parameterSets[0].ct).toBe(768);
		expect(parameterSets[0].sk).toBe(1632);
	});

	it('sk is null when omitted', () => {
		const ps = { name: 'NoSk', level: 1 as const, pk: 100, ct: 200 };
		const data = [kem({ parametersets: [ps] })];
		const { parameterSets } = processKems(data);
		expect(parameterSets[0].sk).toBeNull();
	});

	it('emits one kem entry and a row per parameter set', () => {
		const data = [
			kem({
				name: 'Multi',
				parametersets: [BASE_PS, { ...BASE_PS, name: 'Test-256', pk: 1568, ct: 1568 }],
			}),
		];
		const { kems, parameterSets } = processKems(data);
		expect(kems).toHaveLength(1);
		expect(kems[0]).toEqual({ scheme: 'Multi', category: 'Lattices' });
		expect(parameterSets).toHaveLength(2);
	});

	it('marks classical schemes via the classical flag', () => {
		const data = [kem({ category: 'Pre-Quantum', classical: true, parametersets: [BASE_PS] })];
		const { parameterSets } = processKems(data);
		expect(parameterSets[0].classical).toBe(true);
	});

	it('treats broken: classical as classical, not broken', () => {
		const data = [kem({ broken: 'classical', parametersets: [BASE_PS] })];
		const { parameterSets } = processKems(data);
		expect(parameterSets[0].classical).toBe(true);
		expect(parameterSets[0].broken).toBe(false);
	});

	it('propagates scheme-level broken to parameter sets', () => {
		const data = [kem({ broken: 'key recovery attack', parametersets: [BASE_PS] })];
		const { parameterSets } = processKems(data);
		expect(parameterSets[0].broken).toBe('key recovery attack');
	});

	it('parameterset-level warning overrides scheme level', () => {
		const data = [kem({ warning: 'scheme-level', parametersets: [{ ...BASE_PS, warning: 'ps-level' }] })];
		const { parameterSets } = processKems(data);
		expect(parameterSets[0].warning).toBe('ps-level');
	});

	it('parses Pre-Quantum level', () => {
		const ps = { ...BASE_PS, level: 'Pre-Quantum' as const };
		const data = [kem({ parametersets: [ps] })];
		const { parameterSets } = processKems(data);
		expect(parameterSets[0].level).toBe('Pre-Quantum');
	});

	it('skips schemes with no parameter sets', () => {
		const data = [kem({ parametersets: [] })];
		const { kems, parameterSets } = processKems(data);
		expect(kems).toHaveLength(0);
		expect(parameterSets).toHaveLength(0);
	});

	it('computes ranges across parameter sets', () => {
		const data = [
			kem({
				parametersets: [
					{ name: 'A', level: 1, pk: 800, ct: 768, sk: 1632 },
					{ name: 'B', level: 5, pk: 1568, ct: 1568, sk: 3168 },
				],
			}),
		];
		const { ranges } = processKems(data);
		expect(ranges.pk).toEqual([800, 1568]);
		expect(ranges.ct).toEqual([768, 1568]);
		expect(ranges.pkPlusCt).toEqual([1568, 3136]);
		expect(ranges.sk).toEqual([1632, 3168]);
	});

	it('sk range falls back to [0, 0] when no secret keys present', () => {
		const data = [
			kem({ parametersets: [{ name: 'NoSk', level: 1, pk: 100, ct: 200 }] }),
		];
		const { ranges } = processKems(data);
		expect(ranges.sk).toEqual([0, 0]);
	});

	it('uses measured cycles directly when present', () => {
		const ps = {
			...BASE_PS,
			keygen_cycles: 45_000,
			encaps_cycles: 52_000,
			decaps_cycles: 61_000,
		};
		const data = [kem({ parametersets: [ps] })];
		const { parameterSets } = processKems(data);
		expect(parameterSets[0].keygenCycles).toBe(45_000);
		expect(parameterSets[0].encapsCycles).toBe(52_000);
		expect(parameterSets[0].decapsCycles).toBe(61_000);
		expect(parameterSets[0].extrapolated).toBe(false);
	});

	it('extrapolates cycles from µs when no cycles given', () => {
		const ps = {
			...BASE_PS,
			keygen_us: 18,
			encaps_us: 21,
			decaps_us: 24,
		};
		const data = [kem({ parametersets: [ps] })];
		const { parameterSets } = processKems(data);
		expect(parameterSets[0].keygenCycles).toBe(Math.round((CPUSPEED * 18) / 1_000_000));
		expect(parameterSets[0].encapsCycles).toBe(Math.round((CPUSPEED * 21) / 1_000_000));
		expect(parameterSets[0].decapsCycles).toBe(Math.round((CPUSPEED * 24) / 1_000_000));
		expect(parameterSets[0].extrapolated).toBe(true);
		expect(parameterSets[0].keygenUs).toBe(18);
	});

	it('timing fields are null/zero when no perf data', () => {
		const data = [kem({ parametersets: [BASE_PS] })];
		const { parameterSets } = processKems(data);
		expect(parameterSets[0].keygenUs).toBeNull();
		expect(parameterSets[0].encapsUs).toBeNull();
		expect(parameterSets[0].decapsUs).toBeNull();
		expect(parameterSets[0].keygenCycles).toBe(0);
	});

	it('exposes perf_source from the scheme', () => {
		const data = [kem({ perf_source: 'kembench', parametersets: [BASE_PS] })];
		const { parameterSets } = processKems(data);
		expect(parameterSets[0].perfSource).toBe('kembench');
	});

	it('computes cycle ranges ignoring zero (unmeasured) entries', () => {
		const data = [
			kem({
				parametersets: [
					{ ...BASE_PS, name: 'A', keygen_cycles: 100, encaps_cycles: 200, decaps_cycles: 300 },
					{ ...BASE_PS, name: 'B', keygen_cycles: 500, encaps_cycles: 600, decaps_cycles: 700 },
					{ ...BASE_PS, name: 'C' }, // no perf → zeros, excluded from range
				],
			}),
		];
		const { ranges } = processKems(data);
		expect(ranges.keygenCycles).toEqual([100, 500]);
		expect(ranges.decapsCycles).toEqual([300, 700]);
	});
});
