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
	website: string;
	category: string;
	assumption: string;
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
	website: string;
	category: string;
	assumption: string;
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

export type KemSortableColumn =
	| 'scheme'
	| 'category'
	| 'status'
	| 'parameterset'
	| 'level'
	| 'pk'
	| 'ct'
	| 'pkPlusCt'
	| 'keygenUs'
	| 'encapsUs'
	| 'decapsUs';

export interface KemDataRanges {
	pk: [number, number];
	ct: [number, number];
	pkPlusCt: [number, number];
	/** Displayed times in µs (extrapolated from cycles where needed); null when no row has one. */
	keygenUs: [number, number] | null;
	encapsUs: [number, number] | null;
	decapsUs: [number, number] | null;
}

export interface KemFilterState extends CommonFilter {
	schemes: Set<string>;
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

export interface HistoryEntry {
	date: string;
	type: 'update' | 'attack' | 'standardization' | 'milestone';
	description: string;
	schemes?: string[];
}

/** `signingTime` / `verificationTime`: displayed µs (measured, else extrapolated from cycles). */
export type AxisField =
	| 'pk' | 'sig' | 'pkPlusSig' | 'signingCycles' | 'verificationCycles' | 'signingUs' | 'verificationUs'
	| 'signingTime' | 'verificationTime';
export type ScaleType = 'log' | 'linear';

export type SortableColumn =
	| 'scheme'
	| 'category'
	| 'status'
	| 'parameterset'
	| 'level'
	| 'pk'
	| 'sig'
	| 'pkPlusSig'
	| 'signingCycles'
	| 'verificationCycles'
	| 'signingUs'
	| 'verificationUs';

export interface FilterState extends CommonFilter {
	schemes: Set<string>;
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
	sortCol: SortableColumn;
	sortDir: 'asc' | 'desc';
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
	| 'nist-standards'
	| 'nist-pqc-rounds'
	| 'nist-onramp'
	| 'china-ngcc'
	| 'kpqc'
	| 'iso'
	| 'research'
	| 'pre-quantum';

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
