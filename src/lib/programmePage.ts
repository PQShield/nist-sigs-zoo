import { NGCC_LEVELS } from './constants';
import { fmtDate } from './dates';
import { processYamlSchemes } from './data';
import { processKemSchemes } from './kemData';
import { STANDARDS_PROGRAMME, versionProgrammes } from './programmes';
import type {
	CommonFilter,
	DataRanges,
	KemParameterSet,
	KemSchemeYaml,
	NgccLevel,
	ParameterSet,
	ProgrammeTimelineEntry,
	ProgrammeYaml,
	SchemeYaml
} from './types';

/** Stage-view id meaning "every member at its newest version". */
export const CURRENT_VIEW = 'current';

export type Primitive = 'signature' | 'kem';

export interface StageView {
	id: string;
	label: string;
}

const ROUND = /^round-(\d+)$/;

const capitalise = (t: string) => t.charAt(0).toUpperCase() + t.slice(1);

function inProgramme(v: { programmes?: Record<string, string | string[]> }, programmeId: string): boolean {
	return programmeId in versionProgrammes(v);
}

function atStage(v: { programmes?: Record<string, string | string[]> }, programmeId: string, stage: string): boolean {
	return versionProgrammes(v)[programmeId]?.includes(stage) ?? false;
}

/** Finite placeholder ranges for no rows (processYamlSchemes([]) yields +-Infinity). */
const EMPTY_RANGES: DataRanges = {
	pk: [0, 0],
	sig: [0, 0],
	pkPlusSig: [0, 0],
	signingCycles: [0, 0],
	verificationCycles: [0, 0],
	signingUs: null,
	verificationUs: null
};

export function signatureMembers(programmeId: string, sigs: SchemeYaml[]): SchemeYaml[] {
	return sigs.filter((s) => s.versions.some((v) => inProgramme(v, programmeId)));
}

export function kemMembers(programmeId: string, kems: KemSchemeYaml[]): KemSchemeYaml[] {
	return kems.filter((k) => inProgramme(k, programmeId));
}

/**
 * Stage toggle for the signature candidates: "All, latest specs" plus every round at which a
 * member has a recorded version (most advanced first). Empty when fewer than two rounds have
 * versions, so there is nothing to toggle.
 */
export function stageViews(programme: ProgrammeYaml, sigs: SchemeYaml[]): StageView[] {
	const members = signatureMembers(programme.id, sigs);
	const rounds = [...programme.stages]
		.reverse()
		.filter(
			(stage) =>
				ROUND.test(stage.id) &&
				members.some((s) => s.versions.some((v) => atStage(v, programme.id, stage.id)))
		);
	if (rounds.length < 2) return [];
	return [
		{ id: CURRENT_VIEW, label: 'All, latest specs' },
		...rounds.map((s) => ({ id: s.id, label: capitalise(s.label) }))
	];
}

/**
 * Signature rows of a programme's members only (no reference schemes). The current view shows
 * each member's newest version; a stage view shows the entrants at that stage, pinned to the
 * version recorded there.
 */
export function programmeSignatureRows(
	programmeId: string,
	sigs: SchemeYaml[],
	view: string
): { parameterSets: ParameterSet[]; ranges: DataRanges } {
	const members = signatureMembers(programmeId, sigs);
	if (members.length === 0) return { parameterSets: [], ranges: EMPTY_RANGES };
	if (view === CURRENT_VIEW) {
		const { parameterSets, ranges } = processYamlSchemes(members);
		return { parameterSets, ranges };
	}
	const entrants = members.filter((s) => s.versions.some((v) => atStage(v, programmeId, view)));
	if (entrants.length === 0) return { parameterSets: [], ranges: EMPTY_RANGES };
	const { parameterSets, ranges } = processYamlSchemes(entrants, { programme: programmeId, stage: view });
	return { parameterSets, ranges };
}

export function programmeKemRows(programmeId: string, kems: KemSchemeYaml[]): KemParameterSet[] {
	return processKemSchemes(kemMembers(programmeId, kems)).parameterSets;
}

/** NGCC targets present in the rows, in scale order. */
export function ngccTargets(rows: { ngccLevel: NgccLevel | null }[]): NgccLevel[] {
	return NGCC_LEVELS.filter((l) => rows.some((r) => r.ngccLevel === l));
}

/** Schemes with no recommended parameter set at `target`, sorted. */
export function unmappedAtTarget(
	rows: { scheme: string; ngccLevel: NgccLevel | null; recommended: boolean }[],
	target: NgccLevel
): string[] {
	const mapped = new Set(rows.filter((r) => r.ngccLevel === target && r.recommended).map((r) => r.scheme));
	return [...new Set(rows.map((r) => r.scheme))].filter((s) => !mapped.has(s)).sort((a, b) => a.localeCompare(b));
}

/** Show one NGCC target only. NIST levels are left alone: the scales are never converted. */
export function focusTarget<T extends CommonFilter>(filter: T, target: NgccLevel | null): T {
	return target === null ? filter : { ...filter, ngccLevels: new Set<NgccLevel>([target]) };
}

export type FindingClass = 'design' | 'implementation' | 'warning' | 'none';

export const FINDING_ORDER: readonly FindingClass[] = ['design', 'implementation', 'warning', 'none'];

export const FINDING_LABELS: Record<FindingClass, string> = {
	design: 'broken scheme',
	implementation: 'implementation findings only',
	warning: 'security warnings only',
	none: 'nothing recorded'
};

/** The row fields the findings summary reads. ParameterSet and KemParameterSet both satisfy it. */
export interface FindingRow {
	scheme: string;
	classical: boolean;
	broken: false | string;
	implementation: false | string;
	warning: false | string;
	findingsUrl: string | null;
}

export interface FindingsCount {
	candidates: number;
	design: number;
	implementation: number;
	warning: number;
	none: number;
	/** Candidates with a link to a published findings report. */
	reports: number;
}

const RANK: Record<FindingClass, number> = { design: 3, implementation: 2, warning: 1, none: 0 };

function rowClass(r: FindingRow): FindingClass {
	if (r.broken !== false && !r.classical) return 'design';
	if (r.implementation !== false) return 'implementation';
	if (r.warning !== false) return 'warning';
	return 'none';
}

/** Worst recorded finding per candidate (scheme), counted once each. */
export function findingsSummary(rows: FindingRow[]): FindingsCount {
	const worst = new Map<string, FindingClass>();
	const reports = new Set<string>();
	for (const r of rows) {
		const c = rowClass(r);
		const prev = worst.get(r.scheme);
		if (prev === undefined || RANK[c] > RANK[prev]) worst.set(r.scheme, c);
		if (r.findingsUrl) reports.add(r.scheme);
	}
	const count: FindingsCount = { candidates: worst.size, design: 0, implementation: 0, warning: 0, none: 0, reports: reports.size };
	for (const c of worst.values()) count[c]++;
	return count;
}

/** Link to /signatures/ or /kems/ with the programme filter (and NGCC target / on-ramp round) set. */
export function compareHref(
	base: string,
	programmeId: string,
	kind: Primitive,
	opts: { target?: NgccLevel | null; stage?: string } = {}
): string {
	const params = new URLSearchParams();
	params.set('p', programmeId === STANDARDS_PROGRAMME ? 'nist-standards,nist-pqc-rounds' : programmeId);
	if (opts.target != null) params.set('nl', String(opts.target));
	const round = kind === 'signature' && programmeId === 'nist-onramp' ? ROUND.exec(opts.stage ?? '') : null;
	if (round) params.set('r', round[1]);
	return `${base}/${kind === 'signature' ? 'signatures' : 'kems'}/?${params}`;
}

export function currentStage(programme: ProgrammeYaml): ProgrammeTimelineEntry | null {
	return programme.timeline?.find((e) => e.current) ?? null;
}

/** "Round 1, since 20 Sep 2026", or the bare label when undated; null without a current entry. */
export function stageText(programme: ProgrammeYaml): string | null {
	const cur = currentStage(programme);
	if (!cur) return null;
	return cur.date ? `${cur.label}, since ${fmtDate(cur.date)}` : cur.label;
}

export interface Fact {
	term: string;
	detail: string;
}

function countText(tracked: number, official: number | undefined): string {
	if (official === undefined) return `${tracked} ${tracked === 1 ? 'scheme' : 'schemes'}`;
	const head = `${official} ${official === 1 ? 'candidate' : 'candidates'}`;
	if (tracked === official) return head;
	// More tracked than official: our data spans several rounds, so say so.
	if (tracked > official) return `${head} (official) · ${tracked} entrants across all rounds in our data`;
	return `${head} · ${tracked} in our data`;
}

export function programmeFacts(programme: ProgrammeYaml, counts: { signature: number; kem: number }): Fact[] {
	const facts: Fact[] = [
		{ term: 'Organiser', detail: programme.organiser_detail ?? programme.organiser },
		{ term: 'Region', detail: programme.region }
	];
	const stage = stageText(programme);
	if (stage) facts.push({ term: 'Stage', detail: stage });
	const kinds = [['signature', 'Signatures'], ['kem', 'KEMs']] as const;
	for (const [kind, term] of kinds) {
		if (!programme.primitives.includes(kind)) continue;
		facts.push({ term, detail: countText(counts[kind], programme.official_counts?.[kind]) });
	}
	if (programme.out_of_scope) facts.push({ term: 'Not tracked', detail: programme.out_of_scope });
	return facts;
}

export interface ProgrammeCard {
	programme: ProgrammeYaml;
	stage: string | null;
	/** Members in our data; null when the primitive is out of the programme's scope. */
	signatures: number | null;
	kems: number | null;
}

export function programmeCards(registry: ProgrammeYaml[], sigs: SchemeYaml[], kems: KemSchemeYaml[]): ProgrammeCard[] {
	return registry.map((programme) => ({
		programme,
		stage: stageText(programme),
		signatures: programme.primitives.includes('signature') ? signatureMembers(programme.id, sigs).length : null,
		kems: programme.primitives.includes('kem') ? kemMembers(programme.id, kems).length : null
	}));
}
