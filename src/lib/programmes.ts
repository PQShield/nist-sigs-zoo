import type {
	KemSchemeYaml,
	ProgrammeMembership,
	ProgrammeYaml,
	SchemeYaml,
	VersionYaml
} from './types';

/** Programme id of the NIST standards track; its members are shown in every view. */
export const STANDARDS_PROGRAMME = 'nist-pqc';

/** Programme id -> stage ids. A scalar in the YAML is normalised to a one-element list. */
export function versionProgrammes(v: {
	programmes?: Record<string, string | string[]>;
}): Record<string, string[]> {
	return Object.fromEntries(
		Object.entries(v.programmes ?? {}).map(([programme, stages]) => [
			programme,
			Array.isArray(stages) ? [...stages] : [stages]
		])
	);
}

/** The stage reported for a membership: the last listed one (the furthest reached). */
export function lastStage(stages: string | string[]): string {
	return Array.isArray(stages) ? stages[stages.length - 1] : stages;
}

export function schemeProgrammes(versions: VersionYaml[]): ProgrammeMembership[] {
	const newestFirst = [...versions].sort((a, b) => b.date.localeCompare(a.date));
	const seen = new Map<string, string>();
	for (const version of newestFirst) {
		for (const [programme, stages] of Object.entries(versionProgrammes(version))) {
			if (!seen.has(programme)) seen.set(programme, lastStage(stages));
		}
	}
	return [...seen].map(([programme, stage]) => ({ programme, stage }));
}

/** Standards and pre-quantum schemes: shown as reference rows in every competition view. */
export function isReferenceScheme(versions: VersionYaml[]): boolean {
	return versions.every((v) =>
		Object.keys(versionProgrammes(v)).every((p) => p === STANDARDS_PROGRAMME)
	);
}

export function membershipLabel(m: ProgrammeMembership, programmes: ProgrammeYaml[]): string {
	const programme = programmes.find((p) => p.id === m.programme);
	if (!programme) throw new Error(`unknown programme "${m.programme}"`);
	const stage = programme.stages.find((s) => s.id === m.stage);
	if (!stage) throw new Error(`unknown stage "${m.stage}" in programme "${m.programme}"`);
	return `${programme.short} ${stage.label}`;
}

export interface ProgrammeSummary {
	programme: ProgrammeYaml;
	signatures: string[];
	kems: string[];
}

export function programmeSummaries(
	programmes: ProgrammeYaml[],
	sigs: SchemeYaml[],
	kems: KemSchemeYaml[]
): ProgrammeSummary[] {
	return programmes.map((programme) => ({
		programme,
		signatures: sigs
			.filter((s) => schemeProgrammes(s.versions).some((m) => m.programme === programme.id))
			.map((s) => s.name)
			.sort((a, b) => a.localeCompare(b)),
		kems: kems
			.filter((k) => programme.id in (k.programmes ?? {}))
			.map((k) => k.name)
			.sort((a, b) => a.localeCompare(b))
	}));
}
