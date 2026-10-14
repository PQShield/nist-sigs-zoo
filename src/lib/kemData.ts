import type { KemDataRanges, KemParameterSet, KemScheme, KemSchemeYaml, NistLevel } from './types';
import { CPUSPEED } from './constants';
import { lastStage } from './programmes';
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
			programmes
		});

		const anyRecommended = yaml.parametersets.some((p) => p.recommended);
		for (const ps of yaml.parametersets) {
			const level: NistLevel | null =
				ps.level === undefined ? null : ps.level === 'Pre-Quantum' ? 'Pre-Quantum' : (ps.level as 1 | 2 | 3 | 4 | 5);

			// Parameter-set flags fall back to scheme-level flags.
			const broken = ps.broken ?? yaml.broken ?? false;
			const warning = ps.warning ?? yaml.warning ?? false;
			const info = ps.info ?? yaml.info ?? false;
			const implementation = ps.implementation ?? yaml.implementation ?? false;

			const keygenUs = ps.keygen_us ?? null;
			const encapsUs = ps.encaps_us ?? null;
			const decapsUs = ps.decaps_us ?? null;
			const cyc = (cycles?: number, us?: number | null) =>
				cycles != null && cycles > 0 ? cycles : us != null ? Math.round((CPUSPEED * us) / 1_000_000) : 0;

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
				dnf: ps.dnf ?? null,
				programmes,
				ngccLevel: ps.ngcc_level ?? null,
				recommended: anyRecommended ? ps.recommended === true : true,
				implementation,
				findingsUrl: yaml.findings_url ?? null
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
