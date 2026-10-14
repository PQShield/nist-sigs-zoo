export type NistLevel = 'Pre-Quantum' | 1 | 2 | 3 | 4 | 5;

/** China NGCC claimed classical security, in bits. Not a NIST category; never converted to one. */
export type NgccLevel = 128 | 256 | 384 | 512;

export interface Scheme {
	scheme: string;
	status: string;
	website: string;
	category: string;
	assumption: string;
	broken: false | string;
	warning: false | string;
	info: false | string;
	classical: boolean;
	programmes: ProgrammeMembership[];
}

export interface ParameterSet {
	scheme: string;
	parameterset: string;
	category: string;
	status: string;
	level: NistLevel | null;
	pk: number;
	sig: number;
	pkPlusSig: number;
	signingCycles: number;
	verificationCycles: number;
	signingUs: number | null;
	verificationUs: number | null;
	extrapolated: boolean;
	broken: false | string;
	warning: false | string;
	info: false | string;
	classical: boolean;
	website: string;
	assumption: string;
	notes: string | null;
	version: string;
	perfSource: string | null;
	programmes: ProgrammeMembership[];
	ngccLevel: NgccLevel | null;
	recommended: boolean;
	implementation: false | string;
	findingsUrl: string | null;
}

// YAML schema types

export interface ParameterSetYaml {
	name: string;
	/** The set's name in the previous version, when it was renamed (pairs them in version comparisons). */
	renamed_from?: string;
	level?: number | 'Pre-Quantum';
	ngcc_level?: NgccLevel;
	recommended?: boolean;
	implementation?: string;
	pk: number;
	sig: number;
	signing_cycles?: number;
	verification_cycles?: number;
	signing_us?: number;
	verification_us?: number;
	broken?: string;
	warning?: string;
	info?: string;
	notes?: string;
}

export interface VersionYaml {
	version: string;
	date: string;
	status: string;
	perf_source?: string;
	implementation?: string;
	findings_url?: string;
	/** Programme id → stage id, e.g. { 'nist-onramp': 'round-3' } */
	programmes?: Record<string, string | string[]>;
	notes?: string;
	broken?: string;
	warning?: string;
	info?: string;
	parametersets: ParameterSetYaml[];
}

export interface SchemeYaml {
	name: string;
	/** Full title when `name` is an abbreviation (shown on the scheme page only). */
	full_name?: string;
	website: string;
	category: string;
	assumption: string;
	/** Optional URL slug for /schemes/<slug>/; defaults to slugify(name). Needed only for clashes or non-ASCII names. */
	slug?: string;
	versions: VersionYaml[];
}

// KEM schema types (standalone size comparison — no rounds)

export interface KemParameterSetYaml {
	name: string;
	level?: number | 'Pre-Quantum';
	ngcc_level?: NgccLevel;
	recommended?: boolean;
	implementation?: string;
	pk: number;
	ct: number;
	keygen_cycles?: number;
	encaps_cycles?: number;
	decaps_cycles?: number;
	keygen_us?: number;
	encaps_us?: number;
	decaps_us?: number;
	broken?: string;
	warning?: string;
	info?: string;
	notes?: string;
}

export interface KemSchemeYaml {
	name: string;
	/** Full title when `name` is an abbreviation (shown on the scheme page only). */
	full_name?: string;
	website: string;
	category: string;
	assumption: string;
	/** Optional URL slug for /schemes/<slug>/; defaults to slugify(name). Needed only for clashes or non-ASCII names. */
	slug?: string;
	status: string;
	implementation?: string;
	findings_url?: string;
	/** Programme id → stage id, e.g. { 'nist-pqc': 'standardised' } */
	programmes?: Record<string, string | string[]>;
	broken?: string;
	warning?: string;
	info?: string;
	parametersets: KemParameterSetYaml[];
}

export interface KemScheme {
	scheme: string;
	status: string;
	website: string;
	category: string;
	assumption: string;
	broken: false | string;
	warning: false | string;
	info: false | string;
	classical: boolean;
	programmes: ProgrammeMembership[];
}

export interface KemParameterSet {
	scheme: string;
	parameterset: string;
	category: string;
	status: string;
	level: NistLevel | null;
	pk: number;
	ct: number;
	pkPlusCt: number;
	keygenCycles: number;
	encapsCycles: number;
	decapsCycles: number;
	keygenUs: number | null;
	encapsUs: number | null;
	decapsUs: number | null;
	broken: false | string;
	warning: false | string;
	info: false | string;
	classical: boolean;
	website: string;
	assumption: string;
	notes: string | null;
	programmes: ProgrammeMembership[];
	ngccLevel: NgccLevel | null;
	recommended: boolean;
	implementation: false | string;
	findingsUrl: string | null;
}

export type KemAxisField =
	| 'pk'
	| 'ct'
	| 'pkPlusCt'
	| 'keygenCycles'
	| 'encapsCycles'
	| 'decapsCycles'
	| 'keygenUs'
	| 'encapsUs'
	| 'decapsUs'
	| 'keygenTime'
	| 'encapsTime'
	| 'decapsTime';

export interface KemDataRanges {
	pk: [number, number];
	ct: [number, number];
	pkPlusCt: [number, number];
	/** Displayed times in µs (extrapolated from cycles where needed); null when no row has one. */
	keygenUs: [number, number] | null;
	encapsUs: [number, number] | null;
	decapsUs: [number, number] | null;
}

export interface KemFilterState extends CompareFilter {
	minPk: number;
	maxPk: number;
	minCt: number;
	maxCt: number;
	minPkPlusCt: number;
	maxPkPlusCt: number;
	minKeygenUs: number;
	maxKeygenUs: number;
	minEncapsUs: number;
	maxEncapsUs: number;
	minDecapsUs: number;
	maxDecapsUs: number;
}

export interface BenchmarkEnv {
	license: string;
	attribution: string;
	date: string;
	cpu: {
		model: string;
		cores: number;
		threads_per_core: number;
		max_freq_mhz: number;
		governor: string;
		turbo: string;
	};
	os: string;
	kernel: string;
	compiler: string;
	openssl: string;
	notes: string;
	sources: Record<string, string>;
}

export type HistoryType = 'update' | 'attack' | 'standardization' | 'milestone';

export interface HistoryEntry {
	/** ISO date, quoted in YAML so it stays a string. */
	date: string;
	type: HistoryType;
	/** Trusted HTML from our own YAML. */
	description: string;
	/** Affected scheme names (signature or KEM `name:`); their programmes are derived. */
	schemes?: string[];
	/** Extra programme ids (data/programmes.yaml), added to those derived from `schemes`. */
	programmes?: string[];
}

/** `signingTime` / `verificationTime`: displayed µs (measured, else extrapolated from cycles). */
export type AxisField =
	| 'pk'
	| 'sig'
	| 'pkPlusSig'
	| 'signingCycles'
	| 'verificationCycles'
	| 'signingUs'
	| 'verificationUs'
	| 'signingTime'
	| 'verificationTime';
export type ScaleType = 'log' | 'linear';

export interface FilterState extends CompareFilter {
	minPk: number;
	maxPk: number;
	minSig: number;
	maxSig: number;
	minPkPlusSig: number;
	maxPkPlusSig: number;
	minSigningCycles: number;
	maxSigningCycles: number;
	minVerificationCycles: number;
	maxVerificationCycles: number;
	minSigningUs: number;
	maxSigningUs: number;
	minVerificationUs: number;
	maxVerificationUs: number;
}

export interface DataRanges {
	pk: [number, number];
	sig: [number, number];
	pkPlusSig: [number, number];
	signingCycles: [number, number];
	verificationCycles: [number, number];
	signingUs: [number, number] | null;
	verificationUs: [number, number] | null;
}

// Standardisation programmes (data/programmes.yaml)

export interface ProgrammeStageYaml {
	id: string;
	label: string;
	/** Badge text after the programme's short name; "" shows the short name alone. Default: the label, with "round N" as "RN". */
	chip?: string;
}

export interface ProgrammeLinkYaml {
	label: string;
	url: string;
}

/** One step on a programme page's timeline. `date` is YYYY, YYYY-MM or YYYY-MM-DD; omit when not announced. */
export interface ProgrammeTimelineEntry {
	date?: string;
	label: string;
	note?: string;
	/** The step the programme is at now (at most one per programme). */
	current?: boolean;
}

/** A programme's security-level scale, shown as a table. The first cell of each row is its row header. */
export interface SecurityLevelTableYaml {
	intro: string;
	note?: string;
	columns: string[];
	rows: string[][];
}

/** Where a programme's published findings come from, and how we credit them. */
export interface FindingsSourceYaml {
	label: string;
	url: string;
	credit: string;
	note?: string;
}

export interface ProgrammeYaml {
	id: string;
	name: string;
	short: string;
	organiser: string;
	region: string;
	url?: string;
	primitives: ('signature' | 'kem')[];
	stages: ProgrammeStageYaml[];
	// Programme page content (all optional; the page omits what is missing)
	/** Breadcrumb, card and tab title, e.g. "China NGCC". Falls back to `short`. */
	nav_label?: string;
	/** Facts-box organiser, e.g. "ICCS, Beijing". Falls back to `organiser`. */
	organiser_detail?: string;
	description?: string;
	links?: ProgrammeLinkYaml[];
	/** Facts-box "Not tracked" text, e.g. "9 key exchange, 35 hash functions". */
	out_of_scope?: string;
	candidates_heading?: string;
	/** Official candidate counts at the current stage, for "34 candidates · 0 in our data". */
	official_counts?: { signature?: number; kem?: number };
	timeline?: ProgrammeTimelineEntry[];
	security_levels?: SecurityLevelTableYaml;
	findings?: FindingsSourceYaml;
}

/** One programme a scheme took part in, and how far it got. */
export interface ProgrammeMembership {
	programme: string;
	stage: string;
}

/** Selects the versions that were at `stage` in `programme`. */
export interface StageFilter {
	programme: string;
	stage: string;
}

// Compare-page filters shared by the signatures and KEM pages

/** Programme filter buckets on the compare pages (see rowProgrammeGroups in compareFilters.ts). */
export type ProgrammeGroup =
	'nist-standards' | 'nist-pqc-rounds' | 'nist-onramp' | 'china-ngcc' | 'kpqc' | 'iso' | 'research' | 'pre-quantum';

export type ChartMode = 'sizes' | 'speed';

/** Data-independent filter fields shared by both compare pages, with one URL codec. */
export interface CommonFilter {
	programmes: Set<ProgrammeGroup>;
	levels: Set<NistLevel>;
	ngccLevels: Set<NgccLevel>;
	paramSets: 'recommended' | 'all';
	hideBroken: boolean;
	hideImplementation: boolean;
	hideWarnings: boolean;
	/** Main compare chart axes. View state, kept here so it shares the URL codec. */
	chart: ChartMode;
}

export type SortDir = 'asc' | 'desc';

/** Filter state of a compare page; each bound of its FilterSpec adds `min<Field>` / `max<Field>`. */
export interface CompareFilter extends CommonFilter {
	schemes: Set<string>;
	sortCol: string;
	sortDir: SortDir;
}
