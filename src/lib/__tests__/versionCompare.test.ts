import { describe, expect, it } from 'vitest';
import {
	compareVersions,
	formatChange,
	levelText,
	metricValue,
	parameterSetFlag,
	percentChange,
	versionOptions
} from '$lib/versionCompare';
import type { VersionYaml } from '$lib/types';

const R2: VersionYaml = {
	version: 'Round 2',
	date: '2025-02-05',
	status: 'On-ramp',
	perf_source: 'submission document',
	parametersets: [
		{ name: 'one', level: 1, pk: 1420, sig: 454, signing_cycles: 471028, verification_cycles: 153266, warning: 'MQ' },
		{ name: 'two', level: 1, pk: 4912, sig: 186, signing_cycles: 286028, verification_cycles: 56374, broken: 'Wedge' },
		{ name: 'old', level: 3, pk: 100, sig: 100 }
	]
};

const R3: VersionYaml = {
	version: 'Round 3',
	date: '2026-08-31',
	status: 'On-ramp',
	perf_source: 'our benchmark',
	parametersets: [
		{ name: 'two', level: 1, pk: 2928, sig: 239, signing_cycles: 95622, verification_cycles: 38545 },
		{ name: 'one', level: 1, pk: 1456, sig: 464, signing_us: 47.2, verification_us: 20.9, warning: 'MQ' },
		{ name: 'new', level: 5, pk: 5554, sig: 964 }
	]
};

const row = (c: ReturnType<typeof compareVersions>, name: string) => c.rows.find((r) => r.name === name)!;

describe('compareVersions', () => {
	const c = compareVersions(R2, R3);

	it('matches parameter sets by name: To order first, then sets removed in To', () => {
		expect(c.rows.map((r) => [r.name, r.presence])).toEqual([
			['two', 'both'],
			['one', 'both'],
			['new', 'added'],
			['old', 'removed']
		]);
	});

	it('pairs a renamed set with its previous name, in either direction', () => {
		const renamed: VersionYaml = { ...R3, parametersets: [{ name: 'old3', renamed_from: 'old', level: 3, pk: 120, sig: 100 }] };
		expect(compareVersions(R2, renamed).rows.map((r) => [r.name, r.presence])).toEqual([
			['old3', 'both'],
			['one', 'removed'],
			['two', 'removed']
		]);
		expect(compareVersions(renamed, R2).rows.find((r) => r.name === 'old')!.presence).toBe('both');
	});

	it('computes percentage changes', () => {
		expect(row(c, 'two').metrics.pk.pct).toBeCloseTo(-40.39, 2);
		expect(row(c, 'two').metrics.sig.pct).toBeCloseTo(28.49, 2);
		expect(row(c, 'two').metrics.signingCycles).toMatchObject({
			from: { value: 286028, estimated: false },
			to: { value: 95622, estimated: false }
		});
	});

	it('has no change for added and removed sets', () => {
		expect(row(c, 'new').metrics.pk).toEqual({ from: null, to: { value: 5554, estimated: false }, pct: null });
		expect(row(c, 'old').metrics.sig).toEqual({ from: { value: 100, estimated: false }, to: null, pct: null });
		expect([row(c, 'new').fromLevel, row(c, 'new').toLevel]).toEqual([null, '5']);
		expect([row(c, 'old').fromLevel, row(c, 'old').toLevel]).toEqual(['3', null]);
	});

	it('estimates cycles from µs when a version has no cycle count', () => {
		expect(row(c, 'one').metrics.signingCycles.to).toEqual({ value: 118000, estimated: true });
		expect(c.perf.anyEstimated).toBe(true);
	});

	it('reports the flag of each version and how it changed', () => {
		expect(row(c, 'two')).toMatchObject({
			fromFlag: { kind: 'broken', html: 'Wedge' },
			toFlag: null,
			flagChange: 'cleared'
		});
		expect(row(c, 'one')).toMatchObject({ flagChange: 'unchanged' });
		expect(row(c, 'new').flagChange).toBe('n/a');
		expect(row(c, 'old').flagChange).toBe('n/a');
	});

	it('notes both benchmark sources and never treats different sources as the same', () => {
		expect(c.perf).toMatchObject({ from: 'submission document', to: 'our benchmark', same: false });
		expect(compareVersions(R2, { ...R2, date: '2025-03-26' }).perf.same).toBe(true);
		expect(compareVersions({ ...R2, perf_source: undefined }, { ...R2, perf_source: undefined }).perf).toMatchObject({
			from: null,
			to: null,
			same: false
		});
	});

	it('compares a version with itself as unchanged', () => {
		const self = compareVersions(R3, R3);
		expect(self.rows.every((r) => r.presence === 'both' && r.metrics.pk.pct === 0)).toBe(true);
		expect(self.rows.map((r) => r.flagChange)).toEqual(['none', 'unchanged', 'none']);
	});

	it('raises and changes flags', () => {
		const flagged: VersionYaml = { ...R3, warning: 'new attack' };
		expect(row(compareVersions(R3, flagged), 'two').flagChange).toBe('raised');
		const worse: VersionYaml = {
			...R2,
			parametersets: R2.parametersets.map((p) => ({ ...p, warning: undefined, broken: 'x' }))
		};
		expect(row(compareVersions(R2, worse), 'one').flagChange).toBe('changed');
	});
});

describe('parameterSetFlag', () => {
	const v: VersionYaml = {
		version: 'v',
		date: '2025-01-01',
		status: 'On-ramp',
		broken: 'version-wide',
		parametersets: []
	};

	it('applies a version-level flag to every set and lets a set override it', () => {
		expect(parameterSetFlag(v, { name: 'a', pk: 1, sig: 1 })).toEqual({ kind: 'broken', html: 'version-wide' });
		expect(parameterSetFlag(v, { name: 'b', pk: 1, sig: 1, broken: 'set-specific' })).toEqual({
			kind: 'broken',
			html: 'set-specific'
		});
	});

	it('ranks broken over implementation findings over warnings', () => {
		const plain: VersionYaml = { ...v, broken: undefined };
		expect(parameterSetFlag(plain, { name: 'a', pk: 1, sig: 1, implementation: 'bug', warning: 'w' })?.kind).toBe(
			'implementation'
		);
		expect(parameterSetFlag(plain, { name: 'a', pk: 1, sig: 1, warning: 'w' })?.kind).toBe('warning');
		expect(parameterSetFlag(plain, { name: 'a', pk: 1, sig: 1, info: 'just a note' })).toBeNull();
	});

	it('does not treat a pre-quantum scheme as a security flag', () => {
		expect(parameterSetFlag({ ...v, broken: 'classical' }, { name: 'a', pk: 1, sig: 1 })).toBeNull();
	});
});

describe('metricValue', () => {
	it('returns null when neither cycles nor µs exist', () => {
		expect(metricValue({ name: 'a', pk: 1, sig: 1 }, 'verificationCycles')).toBeNull();
	});
});

describe('percentChange', () => {
	it('is null when a side is missing or the base is zero', () => {
		expect(percentChange(null, 5)).toBeNull();
		expect(percentChange(5, undefined)).toBeNull();
		expect(percentChange(0, 5)).toBeNull();
	});
});

describe('formatChange', () => {
	it.each([
		[null, '', '—', null],
		[0, '', 'same', 'same'],
		[-40.39, '▼', '40%', 'down'],
		[2.535, '▲', '3%', 'up'],
		[0.5, '▲', '0.5%', 'up'],
		[-0.96, '▼', '1.0%', 'down'],
		[0.04, '▲', '<0.1%', 'up'],
		[-0.04, '▼', '<0.1%', 'down'],
		[Number.NaN, '', '—', null],
		[Number.POSITIVE_INFINITY, '', '—', null]
	])('%s -> %s %s', (pct, symbol, text, direction) => {
		expect(formatChange(pct)).toEqual({ symbol, text, direction });
	});

	it('shows a tiny rise computed from sizes as <0.1%', () => {
		expect(formatChange(percentChange(10000, 10004))).toEqual({ symbol: '▲', text: '<0.1%', direction: 'up' });
	});
});

describe('versionOptions', () => {
	it('keeps the given order and gives same-date versions distinct ids', () => {
		const a: VersionYaml = {
			version: 'Round 2',
			date: '2025-03-26',
			status: 'On-ramp',
			perf_source: 'x',
			parametersets: []
		};
		const b: VersionYaml = { version: 'Round 2', date: '2025-03-26', status: 'On-ramp', parametersets: [] };
		expect(versionOptions([a, b, R2])).toEqual([
			{ id: '2025-03-26', label: 'Round 2', date: '2025-03-26', perfSource: 'x' },
			{ id: '2025-03-26-2', label: 'Round 2', date: '2025-03-26', perfSource: null },
			{ id: '2025-02-05', label: 'Round 2', date: '2025-02-05', perfSource: 'submission document' }
		]);
	});
});

describe('levelText', () => {
	it('shows a NIST level, pre-quantum or an NGCC target, never converting', () => {
		expect(levelText('Pre-Quantum', undefined)).toBe('Pre-quantum');
		expect(levelText(3, undefined)).toBe('3');
		expect(levelText(undefined, 128)).toBe('128-bit');
		expect(levelText(null, null)).toBe('—');
	});
});
