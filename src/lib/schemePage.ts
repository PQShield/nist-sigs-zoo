import { httpUrl, programmeChips, securityBadges, type Badge, type BadgeKind, type Chip } from './badges';
import { CPU_GHZ_LABEL, CYCLES_PER_US } from './constants';
import { processYamlSchemes } from './data';
import { fmt, fmtCycles, fmtTime } from './format';
import { processKemSchemes } from './kemData';
import { schemeProgrammes } from './programmes';
import { schemeEvents, type SchemeEvent } from './schemeHistory';
import { schemeSlug } from './slugs';
import { levelText, versionOptions, type VersionOption } from './versionCompare';
import { dnfCell } from './timing';
import type {
	HistoryEntry,
	KemParameterSet,
	KemSchemeYaml,
	ParameterSet,
	ProgrammeYaml,
	SchemeYaml,
	VersionYaml
} from './types';

export interface TableCell {
	text: string;
	kind: 'value' | 'estimated' | 'missing' | 'dnf';
	title?: string;
}

export interface ParamSetRow {
	key: string;
	name: string;
	level: string;
	/** Shown only when the designers name recommended sets and this is one. */
	recommendedTag: boolean;
	badges: Badge[];
	cells: TableCell[];
}

export interface ParamSetTableModel {
	headers: string[];
	rows: ParamSetRow[];
}

export interface FlagNote extends Badge {
	sets: string[];
	allSets: boolean;
}

interface PageCommon {
	name: string;
	fullName: string | null;
	slug: string;
	website: string;
	websiteHost: string | null;
	category: string;
	assumption: string;
	chips: Chip[];
	flagSummary: string;
	findings: { url: string; host: string } | null;
	table: ParamSetTableModel;
	flagNotes: FlagNote[];
	events: SchemeEvent[];
}

export interface SignaturePage extends PageCommon {
	primitive: 'signature';
	latest: VersionOption;
	/** Newest first, aligned with `options`. */
	versions: VersionYaml[];
	options: VersionOption[];
}

export interface KemPage extends PageCommon {
	primitive: 'kem';
	status: string;
}

export type SchemePageData = SignaturePage | KemPage;

export function hostOf(url: string): string | null {
	try {
		const u = new URL(url);
		if (u.protocol !== 'https:' && u.protocol !== 'http:') return null;
		return u.hostname.replace(/^www\./, '');
	} catch {
		return null;
	}
}

function findingsLink(url: string | null | undefined): { url: string; host: string } | null {
	if (!url) return null;
	const host = hostOf(url);
	return host ? { url, host } : null;
}

const num = (n: number): TableCell => ({ text: fmt(n), kind: 'value' });

/** Measured µs, else estimated from measured cycles, else "timeout"/"DNF" when the benchmark did
 * not finish, else "not yet benchmarked". */
export function timeCell(us: number | null, cycles: number, dnf: string | null = null): TableCell {
	if (us != null) return { text: fmtTime(us), kind: 'value' };
	if (cycles > 0) {
		return {
			text: fmtTime(cycles / CYCLES_PER_US),
			kind: 'estimated',
			title: `Estimated from ${fmtCycles(cycles)} cycles @ ${CPU_GHZ_LABEL}`
		};
	}
	if (dnf) return { ...dnfCell(dnf), kind: 'dnf' };
	return { text: '—', kind: 'missing', title: 'Not yet benchmarked' };
}

export function signatureTable(rows: ParameterSet[]): ParamSetTableModel {
	const tagged = rows.some((r) => !r.recommended);
	return {
		headers: ['pk (B)', 'sig (B)', 'pk+sig (B)', 'Sign', 'Verify'],
		rows: rows.map((r) => ({
			key: r.parameterset,
			name: r.parameterset,
			level: levelText(r.level, r.ngccLevel),
			recommendedTag: tagged && r.recommended,
			badges: securityBadges(r),
			cells: [
				num(r.pk),
				num(r.sig),
				num(r.pkPlusSig),
				timeCell(r.signingUs, r.signingCycles, r.dnf),
				timeCell(r.verificationUs, r.verificationCycles, r.dnf)
			]
		}))
	};
}

export function kemTable(rows: KemParameterSet[]): ParamSetTableModel {
	const tagged = rows.some((r) => !r.recommended);
	return {
		headers: ['pk (B)', 'ct (B)', 'pk+ct (B)', 'Keygen', 'Encaps', 'Decaps'],
		rows: rows.map((r) => ({
			key: r.parameterset,
			name: r.parameterset,
			level: levelText(r.level, r.ngccLevel),
			recommendedTag: tagged && r.recommended,
			badges: securityBadges(r),
			cells: [
				num(r.pk),
				num(r.ct),
				num(r.pkPlusCt),
				timeCell(r.keygenUs, r.keygenCycles, r.dnf),
				timeCell(r.encapsUs, r.encapsCycles, r.dnf),
				timeCell(r.decapsUs, r.decapsCycles, r.dnf)
			]
		}))
	};
}

/** One note per distinct (kind, text), listing the parameter sets it applies to. */
export function flagNotes(rows: (Parameters<typeof securityBadges>[0] & { parameterset: string })[]): FlagNote[] {
	const notes: FlagNote[] = [];
	for (const r of rows) {
		for (const b of securityBadges(r)) {
			const hit = notes.find((n) => n.kind === b.kind && n.html === b.html);
			if (hit) {
				hit.sets.push(r.parameterset);
			} else {
				notes.push({ ...b, href: httpUrl(b.href), sets: [r.parameterset], allSets: false });
			}
		}
	}
	for (const n of notes) n.allSets = n.sets.length === rows.length;
	return notes;
}

const SUMMARY_KINDS: BadgeKind[] = ['pre-quantum', 'broken', 'implementation', 'warning'];

export function flagSummary(notes: FlagNote[], total: number): string {
	const parts = SUMMARY_KINDS.flatMap((kind) => {
		const ofKind = notes.filter((n) => n.kind === kind);
		if (ofKind.length === 0) return [];
		const sets = new Set(ofKind.flatMap((n) => n.sets));
		const count = sets.size === total ? 'all sets' : `${sets.size} of ${total} sets`;
		return [`${ofKind[0].title} (${count})`];
	});
	return parts.length > 0 ? parts.join(' · ') : 'None on current sets';
}

export function signaturePage(yaml: SchemeYaml, history: HistoryEntry[], registry: ProgrammeYaml[]): SignaturePage {
	if (yaml.versions.length === 0) throw new Error(`scheme "${yaml.name}" has no versions`);
	const versions = [...yaml.versions].sort((a, b) => b.date.localeCompare(a.date));
	const latest = versions[0];
	const options = versionOptions(versions);
	const { parameterSets } = processYamlSchemes([yaml]);
	const notes = flagNotes(parameterSets);
	return {
		primitive: 'signature',
		name: yaml.name,
		fullName: yaml.full_name ?? null,
		slug: schemeSlug(yaml),
		website: yaml.website,
		websiteHost: hostOf(yaml.website),
		category: yaml.category,
		assumption: yaml.assumption,
		chips: programmeChips(
			{ status: latest.status, version: latest.version, programmes: schemeProgrammes(yaml.versions) },
			registry
		),
		flagSummary: flagSummary(notes, parameterSets.length),
		findings: findingsLink(latest.findings_url),
		table: signatureTable(parameterSets),
		flagNotes: notes,
		events: schemeEvents(history, yaml.name, versions),
		latest: options[0],
		versions,
		options
	};
}

export function kemPage(yaml: KemSchemeYaml, history: HistoryEntry[], registry: ProgrammeYaml[]): KemPage {
	const { schemes, parameterSets } = processKemSchemes([yaml]);
	const notes = flagNotes(parameterSets);
	return {
		primitive: 'kem',
		name: yaml.name,
		fullName: yaml.full_name ?? null,
		slug: schemeSlug(yaml),
		website: yaml.website,
		websiteHost: hostOf(yaml.website),
		category: yaml.category,
		assumption: yaml.assumption,
		chips: programmeChips({ status: yaml.status, programmes: schemes[0]?.programmes ?? [] }, registry),
		flagSummary: flagSummary(notes, parameterSets.length),
		findings: findingsLink(yaml.findings_url),
		table: kemTable(parameterSets),
		flagNotes: notes,
		events: schemeEvents(history, yaml.name, []),
		status: yaml.status
	};
}
