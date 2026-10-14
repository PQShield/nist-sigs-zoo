import { membershipChipLabel, STANDARDS_PROGRAMME } from './programmes';
import type { ProgrammeMembership, ProgrammeYaml } from './types';

export type BadgeKind = 'pre-quantum' | 'broken' | 'implementation' | 'warning' | 'note';

export interface Badge {
	kind: BadgeKind;
	/** Button name prefix and detail-row heading, e.g. "Broken". */
	title: string;
	/** Detail text: trusted HTML from our own YAML files. */
	html: string;
	/** Full findings report (implementation findings only). */
	href: string | null;
}

/** The URL if it is http(s), else null: the one sanitiser for any href built from YAML data. */
export function httpUrl(url: string | null | undefined): string | null {
	return url && /^https?:\/\//i.test(url) ? url : null;
}

/** Legend wording per badge kind. */
export const BADGE_LEGEND: Record<BadgeKind, string> = {
	'pre-quantum': 'pre-quantum',
	broken: 'broken by design',
	implementation: 'implementation findings',
	warning: 'security warning',
	note: 'note'
};

const PRE_QUANTUM_TEXT =
	'Not quantum-resistant: security rests on classical assumptions only. Listed as a reference point.';

export function securityBadges(row: {
	classical: boolean;
	broken: false | string;
	implementation: false | string;
	warning: false | string;
	info: false | string;
	findingsUrl: string | null;
}): Badge[] {
	const badges: Badge[] = [];
	if (row.classical) {
		badges.push({ kind: 'pre-quantum', title: 'Pre-quantum', html: PRE_QUANTUM_TEXT, href: null });
	} else if (row.broken) {
		badges.push({ kind: 'broken', title: 'Broken', html: row.broken, href: null });
	}
	if (row.implementation) {
		badges.push({ kind: 'implementation', title: 'Implementation findings', html: row.implementation, href: httpUrl(row.findingsUrl) });
	}
	if (row.warning) badges.push({ kind: 'warning', title: 'Warning', html: row.warning, href: null });
	if (row.info) badges.push({ kind: 'note', title: 'Note', html: row.info, href: null });
	return badges;
}

export type ChipTone = 'standard' | 'new' | 'plain';

export interface Chip {
	label: string;
	tone: ChipTone;
	/** Programme id for membership chips (not the standard chip); lets pages link to /programmes/<id>/. */
	programme?: string;
}

/**
 * Programmes-column chips. A standard's status chip (FIPS number / Std pending) replaces its
 * NIST PQC membership; every other membership follows as "<short> <stage>".
 */
export function programmeChips(
	row: { status: string; version?: string; programmes: ProgrammeMembership[] },
	registry: ProgrammeYaml[]
): Chip[] {
	const chips: Chip[] = [];
	let standardShown = false;
	if (row.status === 'FIPS') {
		chips.push({
			label: row.version?.startsWith('FIPS') ? row.version : 'FIPS',
			tone: 'standard',
			programme: STANDARDS_PROGRAMME
		});
		standardShown = true;
	} else if (row.status === 'To be standardized') {
		chips.push({ label: 'Std pending', tone: 'standard', programme: STANDARDS_PROGRAMME });
		standardShown = true;
	}
	for (const m of row.programmes) {
		if (standardShown && m.programme === STANDARDS_PROGRAMME) continue;
		chips.push({
			label: membershipChipLabel(m, registry),
			tone: m.programme === 'china-ngcc' ? 'new' : 'plain',
			programme: m.programme
		});
	}
	if (chips.length === 0) {
		chips.push({ label: row.status === 'Classic cryptography' ? 'Classic' : 'Research', tone: 'plain' });
	}
	return chips;
}
