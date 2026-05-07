export type NistLevel = 'Pre-Quantum' | 1 | 2 | 3 | 4 | 5;

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
	tags: string[];
}

export interface ParameterSet {
	scheme: string;
	parameterset: string;
	category: string;
	status: string;
	level: NistLevel;
	pk: number;
	sig: number;
	pkPlusSig: number;
	signingCycles: number;
	verificationCycles: number;
	signingMs: number | null;
	verificationMs: number | null;
	extrapolated: boolean;
	broken: false | string;
	warning: false | string;
	info: false | string;
	classical: boolean;
	website: string;
	assumption: string;
	notes: string | null;
	version: string;
}

// YAML schema types

export interface ParameterSetYaml {
	name: string;
	level: number | 'Pre-Quantum';
	pk: number;
	sig: number;
	signing_cycles?: number;
	verification_cycles?: number;
	signing_ms?: number;
	verification_ms?: number;
	broken?: string;
	warning?: string;
	info?: string;
	notes?: string;
}

export interface VersionYaml {
	version: string;
	date: string;
	status: string;
	tags?: string[];
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
	| 'verificationCycles';

export interface FilterState {
	schemes: Set<string>;
	levels: Set<NistLevel>;
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
	sortCol: SortableColumn;
	sortDir: 'asc' | 'desc';
}

export interface DataRanges {
	pk: [number, number];
	sig: [number, number];
	pkPlusSig: [number, number];
	signingCycles: [number, number];
	verificationCycles: [number, number];
}
