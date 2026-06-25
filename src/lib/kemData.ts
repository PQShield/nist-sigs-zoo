import type {
	KemDataRanges,
	KemFilterState,
	KemParameterSet,
	KemScheme,
	KemSchemeYaml,
	NistLevel
} from './types';

/** Extrapolate cycles from µs at the reference clock when only µs is present. */
const KEM_CPUSPEED = 2_500_000_000;

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
						? Math.round((KEM_CPUSPEED * us) / 1_000_000)
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
