// Thom's decision (2026-10-15): schemes still in the on-ramp carry our benchmark of each
// round's own code in every round view, so round views and the version comparison show real
// round-to-round changes on one machine; eliminated schemes keep their reported numbers.
// bench/update_scheme_data.py writes our timings into the version a row targets
// (bench-common/tests/test_bench_importers.py checks the import end to end); these tests check
// what the site makes of the imported data.
import { describe, it, expect } from 'vitest';
import { programmeSignatureRows } from '$lib/programmePage';
import { compareVersions } from '$lib/versionCompare';
import type { SchemeYaml, VersionYaml } from '$lib/types';

const ENV = '2026-10-20-c8i.4xlarge';

function ours(version: string, date: string, stage: string | string[], sign: number): VersionYaml {
	return {
		version,
		date,
		status: 'On-ramp',
		programmes: { 'nist-onramp': stage },
		perf_source: 'our benchmark',
		parametersets: [
			{
				name: 'one',
				level: 1,
				pk: 1420,
				sig: 454,
				signing_cycles: sign,
				verification_cycles: sign / 4,
				signing_us: sign / 3700,
				verification_us: sign / 14800,
				bench_env: ENV
			}
		]
	};
}

// An active scheme after the import: every round's version has our timings.
const MAYO: SchemeYaml = {
	name: 'MAYO',
	website: 'https://pqmayo.org',
	category: 'Multivariate',
	assumption: 'Multivariate Quadratic',
	versions: [
		ours('Round 3', '2026-08-31', 'round-3', 171468),
		// A post-round update without a programme: not shown in any round view.
		{
			version: 'Round 2',
			date: '2025-03-26',
			status: 'On-ramp',
			perf_source: 'submission document',
			parametersets: [{ name: 'one', level: 1, pk: 1420, sig: 454, signing_cycles: 668264 }]
		},
		ours('Round 2', '2025-02-05', 'round-2', 440000),
		ours('Round 1', '2023-06-01', 'round-1', 420000)
	]
};

// An eliminated scheme: its round-2 version keeps the submission document's numbers.
const CROSS: SchemeYaml = {
	name: 'CROSS',
	website: 'https://www.cross-crypto.com',
	category: 'Code-based',
	assumption: 'Restricted syndrome decoding',
	versions: [
		{
			version: 'v2',
			date: '2025-01-31',
			status: 'On-ramp',
			programmes: { 'nist-onramp': 'round-2' },
			perf_source: 'submission document',
			parametersets: [{ name: 'R-SDP 1 fast', level: 1, pk: 77, sig: 18432, signing_cycles: 1_200_000 }]
		}
	]
};

describe('Round 2 view', () => {
	const rows = programmeSignatureRows('nist-onramp', [MAYO, CROSS], 'round-2').parameterSets;
	const row = (scheme: string) => rows.find((r) => r.scheme === scheme)!;

	it('shows our timings of the round-2 code for a scheme still in the competition', () => {
		expect(row('MAYO')).toMatchObject({
			version: 'Round 2',
			signingCycles: 440000,
			perfSource: 'our benchmark',
			benchEnv: ENV,
			extrapolated: false
		});
	});

	it('keeps the reported numbers of an eliminated scheme', () => {
		expect(row('CROSS')).toMatchObject({
			version: 'v2',
			signingCycles: 1_200_000,
			perfSource: 'submission document',
			benchEnv: null
		});
	});

	it('round 1 pins the round-1 version, also with our timings', () => {
		const r1 = programmeSignatureRows('nist-onramp', [MAYO, CROSS], 'round-1').parameterSets;
		expect(r1.map((r) => [r.scheme, r.version, r.signingCycles, r.perfSource])).toEqual([
			['MAYO', 'Round 1', 420000, 'our benchmark']
		]);
	});
});

describe('version comparison MAYO Round 2 -> Round 3', () => {
	const [r3, , r2] = MAYO.versions;
	const cmp = compareVersions(r2, r3);

	it('shows our measured numbers on both sides', () => {
		const one = cmp.rows.find((r) => r.name === 'one')!;
		expect(one.metrics.signingCycles.from).toEqual({ value: 440000, estimated: false });
		expect(one.metrics.signingCycles.to).toEqual({ value: 171468, estimated: false });
		expect(one.metrics.signingCycles.pct).toBeCloseTo(-61.03, 1);
	});

	it('notes that both sides cite the same source', () => {
		expect(cmp.perf).toEqual({ from: 'our benchmark', to: 'our benchmark', same: true, anyEstimated: false });
	});

	it('still flags differing sources against the post-round update', () => {
		expect(compareVersions(MAYO.versions[1], r3).perf.same).toBe(false);
	});
});
