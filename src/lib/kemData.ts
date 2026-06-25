import { CPUSPEED } from './constants';
import type {
	Kem,
	KemDataRanges,
	KemParameterSet,
	KemYaml,
	NistLevel,
} from './types';

const modules = import.meta.glob('../../data/kems/*.yaml', {
	eager: true,
}) as Record<string, { default: KemYaml }>;

export const allKemData: KemYaml[] = Object.values(modules).map((m) => m.default);

export function processKems(kemData: KemYaml[]): {
	kems: Kem[];
	parameterSets: KemParameterSet[];
	ranges: KemDataRanges;
} {
	const kems: Kem[] = [];
	const rows: KemParameterSet[] = [];

	for (const yaml of kemData) {
		if (!yaml.parametersets || yaml.parametersets.length === 0) continue;

		const classical = yaml.classical === true || yaml.broken === 'classical';

		kems.push({ scheme: yaml.name, category: yaml.category });

		for (const ps of yaml.parametersets) {
			const level: NistLevel =
				ps.level === 'Pre-Quantum' ? 'Pre-Quantum' : (ps.level as 1 | 2 | 3 | 4 | 5);

			const broken = ps.broken ?? yaml.broken ?? false;
			const warning = ps.warning ?? yaml.warning ?? false;
			const info = ps.info ?? yaml.info ?? false;

			// Performance: prefer measured cycles; otherwise extrapolate from µs
			// using CPUSPEED, matching the signature data model.
			const keygenUs = ps.keygen_us ?? null;
			const encapsUs = ps.encaps_us ?? null;
			const decapsUs = ps.decaps_us ?? null;

			let keygenCycles: number;
			let encapsCycles: number;
			let decapsCycles: number;
			let extrapolated: boolean;

			if (ps.keygen_cycles != null && ps.keygen_cycles > 0) {
				extrapolated = false;
				keygenCycles = ps.keygen_cycles;
				encapsCycles = ps.encaps_cycles ?? 0;
				decapsCycles = ps.decaps_cycles ?? 0;
			} else {
				extrapolated = true;
				keygenCycles = keygenUs != null ? Math.round((CPUSPEED * keygenUs) / 1_000_000) : 0;
				encapsCycles = encapsUs != null ? Math.round((CPUSPEED * encapsUs) / 1_000_000) : 0;
				decapsCycles = decapsUs != null ? Math.round((CPUSPEED * decapsUs) / 1_000_000) : 0;
			}

			rows.push({
				scheme: yaml.name,
				parameterset: ps.name,
				category: yaml.category,
				status: yaml.status,
				level,
				pk: ps.pk,
				ct: ps.ct,
				sk: ps.sk ?? null,
				pkPlusCt: ps.pk + ps.ct,
				keygenCycles,
				encapsCycles,
				decapsCycles,
				keygenUs,
				encapsUs,
				decapsUs,
				extrapolated,
				broken: broken === 'classical' ? false : broken || false,
				warning: warning || false,
				info: info || false,
				classical,
				website: yaml.website,
				assumption: yaml.assumption,
				notes: ps.notes ?? null,
				perfSource: yaml.perf_source ?? null,
			});
		}
	}

	const skRows = rows.filter((r) => r.sk != null);
	const range = (vals: number[]): [number, number] =>
		vals.length > 0 ? [Math.min(...vals), Math.max(...vals)] : [0, 0];
	const usRange = (vals: number[]): [number, number] | null =>
		vals.length > 0 ? [Math.min(...vals), Math.max(...vals)] : null;

	const ranges: KemDataRanges = {
		pk: range(rows.map((r) => r.pk)),
		ct: range(rows.map((r) => r.ct)),
		pkPlusCt: range(rows.map((r) => r.pkPlusCt)),
		sk: range(skRows.map((r) => r.sk!)),
		keygenCycles: range(rows.filter((r) => r.keygenCycles > 0).map((r) => r.keygenCycles)),
		encapsCycles: range(rows.filter((r) => r.encapsCycles > 0).map((r) => r.encapsCycles)),
		decapsCycles: range(rows.filter((r) => r.decapsCycles > 0).map((r) => r.decapsCycles)),
		keygenUs: usRange(rows.filter((r) => r.keygenUs != null).map((r) => r.keygenUs!)),
		encapsUs: usRange(rows.filter((r) => r.encapsUs != null).map((r) => r.encapsUs!)),
		decapsUs: usRange(rows.filter((r) => r.decapsUs != null).map((r) => r.decapsUs!)),
	};

	return { kems, parameterSets: rows, ranges };
}
