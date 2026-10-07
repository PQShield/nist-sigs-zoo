import { CYCLES_PER_US } from './constants';
import type { NgccLevel, ParameterSetYaml, VersionYaml } from './types';

export type CompareMode = 'sizes' | 'cycles';
export type MetricKey = 'pk' | 'sig' | 'signingCycles' | 'verificationCycles';

const ALL_METRICS: MetricKey[] = ['pk', 'sig', 'signingCycles', 'verificationCycles'];

export const COMPARE_METRICS: Record<CompareMode, readonly [MetricKey, MetricKey]> = {
	sizes: ['pk', 'sig'],
	cycles: ['signingCycles', 'verificationCycles']
};

export const METRIC_LABELS: Record<MetricKey, string> = {
	pk: 'Public key (B)',
	sig: 'Signature (B)',
	signingCycles: 'Sign (cycles)',
	verificationCycles: 'Verify (cycles)'
};

export type FlagKind = 'broken' | 'implementation' | 'warning';

export const FLAG_TITLES: Record<FlagKind, string> = {
	broken: 'Broken',
	implementation: 'Implementation findings',
	warning: 'Warning'
};

export interface VersionFlag {
	kind: FlagKind;
	/** Trusted HTML from our own YAML. */
	html: string;
}

export interface VersionOption {
	/** Unique within the scheme: the date, suffixed "-2", "-3" … for same-date versions. */
	id: string;
	label: string;
	date: string;
	perfSource: string | null;
}

export interface MetricValue {
	value: number;
	/** Cycles estimated from µs at the reference clock. */
	estimated: boolean;
}

export interface MetricChange {
	from: MetricValue | null;
	to: MetricValue | null;
	pct: number | null;
}

export type FlagChange = 'none' | 'unchanged' | 'raised' | 'cleared' | 'changed' | 'n/a';

export interface ParamSetComparison {
	name: string;
	presence: 'both' | 'added' | 'removed';
	fromLevel: string | null;
	toLevel: string | null;
	metrics: Record<MetricKey, MetricChange>;
	fromFlag: VersionFlag | null;
	toFlag: VersionFlag | null;
	/** 'n/a' unless the set exists in both versions. */
	flagChange: FlagChange;
}

export interface PerfNote {
	from: string | null;
	to: string | null;
	/** Both versions name the same, non-empty source. Even then the machines may differ. */
	same: boolean;
	anyEstimated: boolean;
}

export interface VersionComparison {
	rows: ParamSetComparison[];
	perf: PerfNote;
}

export function levelText(
	level: number | 'Pre-Quantum' | null | undefined,
	ngccLevel: NgccLevel | null | undefined
): string {
	if (level === 'Pre-Quantum') return 'Pre-quantum';
	if (level != null) return String(level);
	if (ngccLevel != null) return `${ngccLevel}-bit`;
	return '—';
}

/** Select options in the given order (callers pass newest first). */
export function versionOptions(versions: VersionYaml[]): VersionOption[] {
	const seen = new Map<string, number>();
	return versions.map((v) => {
		const n = (seen.get(v.date) ?? 0) + 1;
		seen.set(v.date, n);
		return {
			id: n === 1 ? v.date : `${v.date}-${n}`,
			label: v.version,
			date: v.date,
			perfSource: v.perf_source ?? null
		};
	});
}

/** The flag a parameter set carries in a version: set-level overrides version-level; broken > implementation > warning. */
export function parameterSetFlag(v: VersionYaml, ps: ParameterSetYaml): VersionFlag | null {
	const broken = ps.broken ?? v.broken;
	if (broken && broken !== 'classical') return { kind: 'broken', html: broken };
	const implementation = ps.implementation ?? v.implementation;
	if (implementation) return { kind: 'implementation', html: implementation };
	const warning = ps.warning ?? v.warning;
	if (warning) return { kind: 'warning', html: warning };
	return null;
}

export function metricValue(ps: ParameterSetYaml, key: MetricKey): MetricValue | null {
	if (key === 'pk' || key === 'sig') return { value: ps[key], estimated: false };
	const [cycles, us] =
		key === 'signingCycles'
			? [ps.signing_cycles, ps.signing_us]
			: [ps.verification_cycles, ps.verification_us];
	if (cycles != null && cycles > 0) return { value: cycles, estimated: false };
	if (us != null && us > 0) return { value: Math.round(us * CYCLES_PER_US), estimated: true };
	return null;
}

export function percentChange(from: number | null | undefined, to: number | null | undefined): number | null {
	if (from == null || to == null || from === 0) return null;
	return ((to - from) / from) * 100;
}

function flagChange(from: VersionFlag | null, to: VersionFlag | null): FlagChange {
	if (!from && !to) return 'none';
	if (!from) return 'raised';
	if (!to) return 'cleared';
	return from.kind === to.kind ? 'unchanged' : 'changed';
}

export function compareVersions(from: VersionYaml, to: VersionYaml): VersionComparison {
	const fromByName = new Map(from.parametersets.map((p) => [p.name, p]));
	const toNames = new Set(to.parametersets.map((p) => p.name));
	const pairs: [ParameterSetYaml | null, ParameterSetYaml | null][] = [
		...to.parametersets.map((b): [ParameterSetYaml | null, ParameterSetYaml] => [fromByName.get(b.name) ?? null, b]),
		...from.parametersets
			.filter((a) => !toNames.has(a.name))
			.map((a): [ParameterSetYaml, null] => [a, null])
	];

	const rows = pairs.map(([a, b]): ParamSetComparison => {
		const metrics = Object.fromEntries(
			ALL_METRICS.map((k) => {
				const x = a ? metricValue(a, k) : null;
				const y = b ? metricValue(b, k) : null;
				return [k, { from: x, to: y, pct: percentChange(x?.value, y?.value) }];
			})
		) as Record<MetricKey, MetricChange>;
		const fromFlag = a ? parameterSetFlag(from, a) : null;
		const toFlag = b ? parameterSetFlag(to, b) : null;
		return {
			name: (b ?? a)!.name,
			presence: a && b ? 'both' : b ? 'added' : 'removed',
			fromLevel: a ? levelText(a.level, a.ngcc_level) : null,
			toLevel: b ? levelText(b.level, b.ngcc_level) : null,
			metrics,
			fromFlag,
			toFlag,
			flagChange: a && b ? flagChange(fromFlag, toFlag) : 'n/a'
		};
	});

	const fromSource = from.perf_source ?? null;
	const toSource = to.perf_source ?? null;
	const anyEstimated = rows.some((r) =>
		COMPARE_METRICS.cycles.some((k) => r.metrics[k].from?.estimated || r.metrics[k].to?.estimated)
	);
	return {
		rows,
		perf: { from: fromSource, to: toSource, same: fromSource !== null && fromSource === toSource, anyEstimated }
	};
}

export interface ChangeText {
	symbol: '▼' | '▲' | '';
	text: string;
	direction: 'down' | 'up' | 'same' | null;
}

/** "▼ 40%", "▲ 0.5%", "▲ <0.1%", "same" or "—" (no comparable values). */
export function formatChange(pct: number | null): ChangeText {
	if (pct === null || !Number.isFinite(pct)) return { symbol: '', text: '—', direction: null };
	if (pct === 0) return { symbol: '', text: 'same', direction: 'same' };
	const abs = Math.abs(pct);
	const shown = abs < 1 ? abs.toFixed(1) : String(Math.round(abs));
	const text = shown === '0.0' ? '<0.1%' : `${shown}%`;
	return pct < 0 ? { symbol: '▼', text, direction: 'down' } : { symbol: '▲', text, direction: 'up' };
}
