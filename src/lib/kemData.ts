import type {
	KemDataRanges,
	KemFilterState,
	KemParameterSet,
	KemScheme,
	KemSchemeYaml,
	NistLevel,
	ProgrammeGroup
} from './types';
import { CPUSPEED } from './constants';
import { lastStage } from './programmes';
import { decodeCommonFilters, defaultCommonFilter, encodeCommonFilters, passesCommonFilters } from './compareFilters';
import { kemTimes, timeRange } from './timing';

/**
 * Flatten KEM YAML into schemes + parameter-set rows.
 *
 * A one-off size + performance comparison: no versions (programme memberships live at scheme level). Security
 * flags (broken/warning/info) may be set at scheme level and overridden per
 * parameter set. Benchmark fields (keygen/encaps/decaps) are optional.
 */
export function processKemSchemes(schemeData: KemSchemeYaml[]): {
	schemes: KemScheme[];
	parameterSets: KemParameterSet[];
} {
	const schemes: KemScheme[] = [];
	const rows: KemParameterSet[] = [];

	for (const yaml of schemeData) {
		if (!yaml.parametersets || yaml.parametersets.length === 0) continue;

		const programmes = Object.entries(yaml.programmes ?? {}).map(([programme, stages]) => ({
			programme,
			stage: lastStage(stages)
		}));

		schemes.push({
			scheme: yaml.name,
			status: yaml.status,
			website: yaml.website,
			category: yaml.category,
			assumption: yaml.assumption,
			broken: yaml.broken ?? false,
			warning: yaml.warning ?? false,
			info: yaml.info ?? false,
			classical: yaml.broken === 'classical',
			programmes,
		});

		const anyRecommended = yaml.parametersets.some((p) => p.recommended);
		for (const ps of yaml.parametersets) {
			const level: NistLevel | null =
				ps.level === undefined
					? null
					: ps.level === 'Pre-Quantum'
						? 'Pre-Quantum'
						: (ps.level as 1 | 2 | 3 | 4 | 5);

			// Parameter-set flags fall back to scheme-level flags.
			const broken = ps.broken ?? yaml.broken ?? false;
			const warning = ps.warning ?? yaml.warning ?? false;
			const info = ps.info ?? yaml.info ?? false;
			const implementation = ps.implementation ?? yaml.implementation ?? false;

			const keygenUs = ps.keygen_us ?? null;
			const encapsUs = ps.encaps_us ?? null;
			const decapsUs = ps.decaps_us ?? null;
			const cyc = (cycles?: number, us?: number | null) =>
				cycles != null && cycles > 0
					? cycles
					: us != null
						? Math.round((CPUSPEED * us) / 1_000_000)
						: 0;

			rows.push({
				scheme: yaml.name,
				parameterset: ps.name,
				category: yaml.category,
				status: yaml.status,
				level,
				pk: ps.pk,
				ct: ps.ct,
				pkPlusCt: ps.pk + ps.ct,
				keygenCycles: cyc(ps.keygen_cycles, keygenUs),
				encapsCycles: cyc(ps.encaps_cycles, encapsUs),
				decapsCycles: cyc(ps.decaps_cycles, decapsUs),
				keygenUs,
				encapsUs,
				decapsUs,
				broken,
				warning,
				info,
				classical: broken === 'classical',
				website: yaml.website,
				assumption: yaml.assumption,
				notes: ps.notes ?? null,
				programmes,
				ngccLevel: ps.ngcc_level ?? null,
				recommended: anyRecommended ? ps.recommended === true : true,
				implementation,
				findingsUrl: yaml.findings_url ?? null,
			});
		}
	}

	return { schemes, parameterSets: rows };
}

/** Min/max for each size axis, and the displayed-µs range of each operation, across the given rows. */
export function computeKemRanges(rows: KemParameterSet[]): KemDataRanges {
	const span = (values: number[]): [number, number] =>
		values.length === 0 ? [0, 0] : [Math.min(...values), Math.max(...values)];
	const times = rows.map(kemTimes);
	return {
		pk: span(rows.map((r) => r.pk)),
		ct: span(rows.map((r) => r.ct)),
		pkPlusCt: span(rows.map((r) => r.pkPlusCt)),
		keygenUs: timeRange(times.map((t) => t.keygen)),
		encapsUs: timeRange(times.map((t) => t.encaps)),
		decapsUs: timeRange(times.map((t) => t.decaps))
	};
}

/** Default filter: every scheme and programme group, all levels, bounds at the data extremes. */
export function defaultKemFilter(
	ranges: KemDataRanges,
	schemeNames: Iterable<string>,
	groups: Iterable<ProgrammeGroup>
): KemFilterState {
	return {
		...defaultCommonFilter(groups),
		schemes: new Set(schemeNames),
		minPk: ranges.pk[0],
		maxPk: ranges.pk[1],
		minCt: ranges.ct[0],
		maxCt: ranges.ct[1],
		minPkPlusCt: ranges.pkPlusCt[0],
		maxPkPlusCt: ranges.pkPlusCt[1],
		minKeygenUs: ranges.keygenUs?.[0] ?? 0,
		maxKeygenUs: ranges.keygenUs?.[1] ?? Infinity,
		minEncapsUs: ranges.encapsUs?.[0] ?? 0,
		maxEncapsUs: ranges.encapsUs?.[1] ?? Infinity,
		minDecapsUs: ranges.decapsUs?.[0] ?? 0,
		maxDecapsUs: ranges.decapsUs?.[1] ?? Infinity
	};
}

/** Numeric bound fields and their URL keys. */
const BOUNDS = [
	['pkMin', 'minPk'], ['pkMax', 'maxPk'],
	['ctMin', 'minCt'], ['ctMax', 'maxCt'],
	['pcMin', 'minPkPlusCt'], ['pcMax', 'maxPkPlusCt'],
	['kuMin', 'minKeygenUs'], ['kuMax', 'maxKeygenUs'],
	['euMin', 'minEncapsUs'], ['euMax', 'maxEncapsUs'],
	['duMin', 'minDecapsUs'], ['duMax', 'maxDecapsUs']
] as const;

/** Encode non-default filter state as URL query params (shares the common codec with the signatures page). */
export function buildKemUrlParams(state: KemFilterState, defaults: KemFilterState): URLSearchParams {
	const p = new URLSearchParams();

	if (
		state.schemes.size !== defaults.schemes.size ||
		![...state.schemes].every((s) => defaults.schemes.has(s))
	) {
		p.set('s', [...state.schemes].map(encodeURIComponent).join(','));
	}

	encodeCommonFilters(state, defaults, p);

	for (const [key, field] of BOUNDS) {
		if (state[field] !== defaults[field]) p.set(key, String(state[field]));
	}

	return p;
}

/** Decode URL query params onto a copy of the default filter state. */
export function applyKemUrlParams(defaults: KemFilterState, params: URLSearchParams): KemFilterState {
	const s: KemFilterState = { ...defaults, schemes: new Set(defaults.schemes) };

	const schemesParam = params.get('s');
	if (schemesParam) s.schemes = new Set(schemesParam.split(',').map(decodeURIComponent));

	for (const [key, field] of BOUNDS) {
		if (params.has(key)) s[field] = Number(params.get(key));
	}

	return { ...s, ...decodeCommonFilters(s, params) };
}

function outside(v: number | null, min: number, max: number): boolean {
	return v != null && (v < min || v > max);
}

/** Apply scheme, common (programme/level/parameter-set/status), size and µs filters. */
export function filterKemRows(rows: KemParameterSet[], filter: KemFilterState): KemParameterSet[] {
	return rows.filter((row) => {
		if (!filter.schemes.has(row.scheme)) return false;
		if (!passesCommonFilters(row, filter)) return false;
		if (row.pk < filter.minPk || row.pk > filter.maxPk) return false;
		if (row.ct < filter.minCt || row.ct > filter.maxCt) return false;
		if (row.pkPlusCt < filter.minPkPlusCt || row.pkPlusCt > filter.maxPkPlusCt) return false;
		const t = kemTimes(row);
		if (outside(t.keygen, filter.minKeygenUs, filter.maxKeygenUs)) return false;
		if (outside(t.encaps, filter.minEncapsUs, filter.maxEncapsUs)) return false;
		if (outside(t.decaps, filter.minDecapsUs, filter.maxDecapsUs)) return false;
		return true;
	});
}
