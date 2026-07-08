import type {
	KemDataRanges,
	KemFilterState,
	KemParameterSet,
	KemScheme,
	KemSchemeYaml,
	NistLevel
} from './types';
import { CPUSPEED } from './constants';

/**
 * Flatten KEM YAML into schemes + parameter-set rows.
 *
 * A one-off size + performance comparison: no versions, no round tags. Security
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
		});

		for (const ps of yaml.parametersets) {
			const level: NistLevel =
				ps.level === 'Pre-Quantum' ? 'Pre-Quantum' : (ps.level as 1 | 2 | 3 | 4 | 5);

			// Parameter-set flags fall back to scheme-level flags.
			const broken = ps.broken ?? yaml.broken ?? false;
			const warning = ps.warning ?? yaml.warning ?? false;
			const info = ps.info ?? yaml.info ?? false;

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
			});
		}
	}

	return { schemes, parameterSets: rows };
}

/** Min/max for each numeric size axis across the given rows. */
export function computeKemRanges(rows: KemParameterSet[]): KemDataRanges {
	if (rows.length === 0) {
		return { pk: [0, 0], ct: [0, 0], pkPlusCt: [0, 0] };
	}
	return {
		pk: [Math.min(...rows.map((r) => r.pk)), Math.max(...rows.map((r) => r.pk))],
		ct: [Math.min(...rows.map((r) => r.ct)), Math.max(...rows.map((r) => r.ct))],
		pkPlusCt: [
			Math.min(...rows.map((r) => r.pkPlusCt)),
			Math.max(...rows.map((r) => r.pkPlusCt))
		]
	};
}

/** Default filter: everything selected, size bounds at the data extremes. */
export function defaultKemFilter(
	ranges: KemDataRanges,
	schemeNames: Iterable<string>
): KemFilterState {
	return {
		schemes: new Set(schemeNames),
		levels: new Set(['Pre-Quantum', 1, 2, 3, 4, 5] as NistLevel[]),
		minPk: ranges.pk[0],
		maxPk: ranges.pk[1],
		minCt: ranges.ct[0],
		maxCt: ranges.ct[1],
		minPkPlusCt: ranges.pkPlusCt[0],
		maxPkPlusCt: ranges.pkPlusCt[1]
	};
}

/** Encode non-default filter state as URL query params (mirrors the signatures codec). */
export function buildKemUrlParams(
	state: KemFilterState,
	defaults: KemFilterState
): URLSearchParams {
	const p = new URLSearchParams();

	if (
		state.schemes.size !== defaults.schemes.size ||
		![...state.schemes].every((s) => defaults.schemes.has(s))
	) {
		p.set('s', [...state.schemes].map(encodeURIComponent).join(','));
	}

	if (
		state.levels.size !== defaults.levels.size ||
		![...state.levels].every((l) => defaults.levels.has(l))
	) {
		p.set('l', [...state.levels].map((l) => (l === 'Pre-Quantum' ? 'PQ' : String(l))).join(','));
	}

	if (state.minPk !== defaults.minPk) p.set('pkMin', String(state.minPk));
	if (state.maxPk !== defaults.maxPk) p.set('pkMax', String(state.maxPk));
	if (state.minCt !== defaults.minCt) p.set('ctMin', String(state.minCt));
	if (state.maxCt !== defaults.maxCt) p.set('ctMax', String(state.maxCt));
	if (state.minPkPlusCt !== defaults.minPkPlusCt) p.set('pcMin', String(state.minPkPlusCt));
	if (state.maxPkPlusCt !== defaults.maxPkPlusCt) p.set('pcMax', String(state.maxPkPlusCt));

	return p;
}

/** Decode URL query params onto a copy of the default filter state. */
export function applyKemUrlParams(
	defaults: KemFilterState,
	params: URLSearchParams
): KemFilterState {
	const s: KemFilterState = {
		...defaults,
		schemes: new Set(defaults.schemes),
		levels: new Set(defaults.levels)
	};

	const schemesParam = params.get('s');
	if (schemesParam) s.schemes = new Set(schemesParam.split(',').map(decodeURIComponent));

	const levelsParam = params.get('l');
	if (levelsParam) {
		s.levels = new Set(
			levelsParam.split(',').map((v) => (v === 'PQ' ? 'Pre-Quantum' : Number(v))) as NistLevel[]
		);
	}

	if (params.has('pkMin')) s.minPk = Number(params.get('pkMin'));
	if (params.has('pkMax')) s.maxPk = Number(params.get('pkMax'));
	if (params.has('ctMin')) s.minCt = Number(params.get('ctMin'));
	if (params.has('ctMax')) s.maxCt = Number(params.get('ctMax'));
	if (params.has('pcMin')) s.minPkPlusCt = Number(params.get('pcMin'));
	if (params.has('pcMax')) s.maxPkPlusCt = Number(params.get('pcMax'));

	return s;
}

/** Apply scheme/level/size filters to the rows. */
export function filterKemRows(
	rows: KemParameterSet[],
	filter: KemFilterState
): KemParameterSet[] {
	return rows.filter((row) => {
		if (!filter.schemes.has(row.scheme)) return false;
		if (!filter.levels.has(row.level)) return false;
		if (row.pk < filter.minPk || row.pk > filter.maxPk) return false;
		if (row.ct < filter.minCt || row.ct > filter.maxCt) return false;
		if (row.pkPlusCt < filter.minPkPlusCt || row.pkPlusCt > filter.maxPkPlusCt) return false;
		return true;
	});
}
