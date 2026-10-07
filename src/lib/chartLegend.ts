import type { NgccLevel, NistLevel } from './types';

/** SVG paths in [-1, 1]²: Vega-Lite shape values and HTML legend symbols use the same strings. */
export const SHAPES = {
	star: 'M0,-1L0.224,-0.309L0.951,-0.309L0.363,0.118L0.588,0.809L0,0.382L-0.588,0.809L-0.363,0.118L-0.951,-0.309L-0.224,-0.309Z',
	square: 'M-0.75,-0.75H0.75V0.75H-0.75Z',
	plus: 'M-0.3,-1H0.3V-0.3H1V0.3H0.3V1H-0.3V0.3H-1V-0.3H-0.3Z',
	diamond: 'M0,-1L0.9,0L0,1L-0.9,0Z',
	triangleUp: 'M0,-0.9L0.9,0.7L-0.9,0.7Z',
	triangleDown: 'M0,0.9L0.9,-0.7L-0.9,-0.7Z',
	circle: 'M0.8,0A0.8,0.8 0 1,1 -0.8,0A0.8,0.8 0 1,1 0.8,0Z',
	triangleLeft: 'M-0.9,0L0.7,-0.9L0.7,0.9Z',
	triangleRight: 'M0.9,0L-0.7,-0.9L-0.7,0.9Z'
} as const;

export const STANDARDISED = 'Standardised';

/** Family → shape, in legend order. Standardised schemes get a star whatever their family. */
export const FAMILY_SHAPES: Record<string, string> = {
	[STANDARDISED]: SHAPES.star,
	Lattices: SHAPES.square,
	'Code-based': SHAPES.plus,
	MPCitH: SHAPES.diamond,
	Multivariate: SHAPES.triangleUp,
	Symmetric: SHAPES.triangleDown,
	Isogenies: SHAPES.circle,
	'Pre-Quantum': SHAPES.triangleLeft,
	Other: SHAPES.triangleRight
};

export function familyKey(row: { status: string; category: string }): string {
	if (row.status === 'FIPS' || row.status === 'To be standardized') return STANDARDISED;
	return row.category in FAMILY_SHAPES ? row.category : 'Other';
}

const NIST_KEYS = ['Pre-quantum', 'Level 1', 'Level 2', 'Level 3', 'Level 4', 'Level 5'] as const;
const NIST_COLOURS = {
	light: ['#6B7280', '#1D4ED8', '#059669', '#D97706', '#EA580C', '#DC2626'],
	dark: ['#9CA3AF', '#60A5FA', '#34D399', '#FBBF24', '#F97316', '#F87171']
};
const NGCC_KEYS = ['128-bit', '256-bit', '384-bit', '512-bit'] as const;
const NGCC_COLOURS = {
	light: ['#A855F7', '#7E22CE', '#6B21A8', '#4C1D95'],
	dark: ['#D8B4FE', '#C084FC', '#A855F7', '#9333EA']
};

/** Colour key of a row: its level on its own scale. NGCC targets never become NIST levels. */
export function levelKey(level: NistLevel | null, ngccLevel: NgccLevel | null): string {
	if (level === 'Pre-Quantum') return 'Pre-quantum';
	if (level !== null) return `Level ${level}`;
	if (ngccLevel !== null) return `${ngccLevel}-bit`;
	return 'No level';
}

export function levelDescription(level: NistLevel | null, ngccLevel: NgccLevel | null): string {
	if (level === 'Pre-Quantum') return 'Pre-quantum';
	if (level !== null) return `NIST level ${level}`;
	if (ngccLevel !== null) return `NGCC target: ${ngccLevel}-bit classical`;
	return 'Not stated';
}

export interface ColourScale {
	domain: string[];
	range: string[];
}

export function levelColourScale(keys: Iterable<string>, dark: boolean): ColourScale {
	const present = new Set(keys);
	const scale: ColourScale = { domain: [], range: [] };
	const add = (ks: readonly string[], colours: string[]) =>
		ks.forEach((k, i) => {
			if (present.has(k)) {
				scale.domain.push(k);
				scale.range.push(colours[i]);
			}
		});
	add(NIST_KEYS, dark ? NIST_COLOURS.dark : NIST_COLOURS.light);
	add(NGCC_KEYS, dark ? NGCC_COLOURS.dark : NGCC_COLOURS.light);
	return scale;
}

export type Security = 'broken' | 'warning' | 'ok';

export function securityStatus(row: { broken: false | string; classical: boolean; warning: false | string }): Security {
	if (row.broken && !row.classical) return 'broken';
	if (row.warning) return 'warning';
	return 'ok';
}

/** Ring colours of the broken/warning overlay layers. */
export const RINGS = {
	broken: { light: '#B91C1C', dark: '#FCA5A5' },
	warning: { light: '#B45309', dark: '#FDE68A' }
} as const;

export interface LegendItem {
	label: string;
	colour?: string;
	shape?: string;
	ring?: string;
}

export interface LegendGroup {
	title: string;
	items: LegendItem[];
	note?: string;
}

export interface ChartPoint {
	family: string;
	level: string;
	security: Security;
}

/** Legend groups for the points actually drawn: Family, NIST category, NGCC target, Security. */
export function chartLegend(points: ChartPoint[], dark: boolean): LegendGroup[] {
	const families = new Set(points.map((p) => p.family));
	const levels = new Set(points.map((p) => p.level));
	const scale = levelColourScale(levels, dark);
	const colourOf = (k: string) => scale.range[scale.domain.indexOf(k)];
	const groups: LegendGroup[] = [];

	const family = Object.keys(FAMILY_SHAPES)
		.filter((f) => families.has(f))
		.map((f) => ({ label: f === 'Pre-Quantum' ? 'Pre-quantum' : f, shape: FAMILY_SHAPES[f] }));
	if (family.length) groups.push({ title: 'Family', items: family });

	const nist = NIST_KEYS.filter((k) => levels.has(k)).map((k) => ({ label: k, colour: colourOf(k) }));
	if (nist.length) groups.push({ title: 'NIST category', items: nist });

	const ngcc = NGCC_KEYS.filter((k) => levels.has(k)).map((k) => ({ label: `${k} classical`, colour: colourOf(k) }));
	if (ngcc.length) groups.push({ title: 'NGCC target', items: ngcc, note: 'Not comparable to NIST levels.' });

	const tone = dark ? 'dark' : 'light';
	const security: LegendItem[] = [];
	if (points.some((p) => p.security === 'broken')) security.push({ label: 'Broken by design', ring: RINGS.broken[tone] });
	if (points.some((p) => p.security === 'warning')) security.push({ label: 'Warning', ring: RINGS.warning[tone] });
	if (security.length) groups.push({ title: 'Security', items: security });

	return groups;
}
