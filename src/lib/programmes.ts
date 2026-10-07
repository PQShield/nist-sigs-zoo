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

import type { ProgrammeStageYaml } from './types';

export interface StageGroup {
	stage: ProgrammeStageYaml;
	/** Scheme names, sorted. */
	schemes: string[];
}

export interface ProgrammeSummary {
	programme: ProgrammeYaml;
	signatures: StageGroup[];
	kems: StageGroup[];
}

/** Group scheme names by stage; most advanced (last registry stage) first, empty groups omitted. */
function groupByStage(
	programme: ProgrammeYaml,
	members: { name: string; stage: string | undefined }[]
): StageGroup[] {
	return [...programme.stages]
		.reverse()
		.map((stage) => ({
			stage,
			schemes: members
				.filter((m) => m.stage === stage.id)
				.map((m) => m.name)
				.sort((a, b) => a.localeCompare(b))
		}))
		.filter((g) => g.schemes.length > 0);
}

export function programmeSummaries(
	programmes: ProgrammeYaml[],
	sigs: SchemeYaml[],
	kems: KemSchemeYaml[]
): ProgrammeSummary[] {
	return programmes.map((programme) => ({
		programme,
		signatures: groupByStage(
			programme,
			sigs.map((s) => ({
				name: s.name,
				stage: schemeProgrammes(s.versions).find((m) => m.programme === programme.id)?.stage
			}))
		),
		kems: groupByStage(
			programme,
			kems.map((k) => {
				const stages = k.programmes?.[programme.id];
				return { name: k.name, stage: stages === undefined ? undefined : lastStage(stages) };
			})
		)
	}));
}

const ROUND = /^round-(\d+)$/;

/**
 * Collapse a cell's groups (most advanced first): non-round stages and the highest round
 * are listed in full; lower rounds are reduced to a count of schemes that ended there.
 */
export function summariseGroups(groups: StageGroup[]): {
	full: StageGroup[];
	ended: { stage: ProgrammeStageYaml; count: number }[];
} {
	const roundNo = (g: StageGroup) => Number(ROUND.exec(g.stage.id)?.[1] ?? NaN);
	const top = Math.max(...groups.map(roundNo).filter((n) => !Number.isNaN(n)));
	const full: StageGroup[] = [];
	const ended: { stage: ProgrammeStageYaml; count: number }[] = [];
	for (const g of groups) {
		const n = roundNo(g);
		if (Number.isNaN(n) || n === top) full.push(g);
		else ended.push({ stage: g.stage, count: g.schemes.length });
	}
	return { full, ended };
}
