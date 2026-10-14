import type { CompareConfig, FilterSpec } from './compare';
import { kemAxisValue, sigAxisValue } from './chartData';
import { STANDARDISED } from './chartLegend';
import { kemTimes, sigTimes } from './timing';
import type {
	AxisField,
	DataRanges,
	FilterState,
	KemAxisField,
	KemDataRanges,
	KemFilterState,
	KemParameterSet,
	ParameterSet
} from './types';

const SIZES = 'Sizes (bytes)';
const CYCLES = 'Performance (cycles)';
const US = 'Performance (µs)';

export const SIGNATURE_FILTER: FilterSpec<ParameterSet, DataRanges, FilterState> = {
	bounds: [
		{ field: 'Pk', url: 'pk', label: 'pk', section: SIZES, value: (r) => r.pk, range: (g) => g.pk },
		{ field: 'Sig', url: 'sig', label: 'sig', section: SIZES, value: (r) => r.sig, range: (g) => g.sig },
		{ field: 'PkPlusSig', url: 'ps', label: 'pk+sig', section: SIZES, value: (r) => r.pkPlusSig, range: (g) => g.pkPlusSig },
		// Unbenchmarked rows (0 cycles) pass the cycle filters.
		{ field: 'SigningCycles', url: 'sc', label: 'signing', section: CYCLES, value: (r) => (r.signingCycles > 0 ? r.signingCycles : null), range: (g) => g.signingCycles },
		{ field: 'VerificationCycles', url: 'vc', label: 'verification', section: CYCLES, value: (r) => (r.verificationCycles > 0 ? r.verificationCycles : null), range: (g) => g.verificationCycles },
		{ field: 'SigningUs', url: 'su', label: 'signing µs', section: US, value: (r) => r.signingUs, range: (g) => g.signingUs },
		{ field: 'VerificationUs', url: 'vu', label: 'verification µs', section: US, value: (r) => r.verificationUs, range: (g) => g.verificationUs }
	],
	defaultSort: 'scheme',
	sortInUrl: true,
	numericSort: {
		pk: (r) => r.pk,
		sig: (r) => r.sig,
		pkPlusSig: (r) => r.pkPlusSig,
		signingCycles: (r) => r.signingCycles,
		verificationCycles: (r) => r.verificationCycles
	},
	timeSort: {
		signingUs: (r) => sigTimes(r).sign,
		verificationUs: (r) => sigTimes(r).verify
	}
};

export const KEM_FILTER: FilterSpec<KemParameterSet, KemDataRanges, KemFilterState> = {
	bounds: [
		{ field: 'Pk', url: 'pk', label: 'pk', section: SIZES, value: (r) => r.pk, range: (g) => g.pk },
		{ field: 'Ct', url: 'ct', label: 'ct', section: SIZES, value: (r) => r.ct, range: (g) => g.ct },
		{ field: 'PkPlusCt', url: 'pc', label: 'pk+ct', section: SIZES, value: (r) => r.pkPlusCt, range: (g) => g.pkPlusCt },
		{ field: 'KeygenUs', url: 'ku', label: 'keygen', section: US, value: (r) => kemTimes(r).keygen, range: (g) => g.keygenUs },
		{ field: 'EncapsUs', url: 'eu', label: 'encaps', section: US, value: (r) => kemTimes(r).encaps, range: (g) => g.encapsUs },
		{ field: 'DecapsUs', url: 'du', label: 'decaps', section: US, value: (r) => kemTimes(r).decaps, range: (g) => g.decapsUs }
	],
	defaultSort: 'pkPlusCt',
	sortInUrl: false,
	numericSort: { pk: (r) => r.pk, ct: (r) => r.ct, pkPlusCt: (r) => r.pkPlusCt },
	timeSort: {
		keygenUs: (r) => kemTimes(r).keygen,
		encapsUs: (r) => kemTimes(r).encaps,
		decapsUs: (r) => kemTimes(r).decaps
	}
};

export const SIGNATURES: CompareConfig<ParameterSet, DataRanges, FilterState, AxisField> = {
	filter: SIGNATURE_FILTER,
	table: {
		columns: [
			{ key: 'pk', label: 'pk (B)', size: (r) => r.pk },
			{ key: 'sig', label: 'sig (B)', size: (r) => r.sig },
			{ key: 'pkPlusSig', label: 'pk+sig (B)', size: (r) => r.pkPlusSig },
			{ key: 'signingUs', label: 'Sign', time: (r) => [r.signingUs, r.signingCycles] },
			{ key: 'verificationUs', label: 'Verify', time: (r) => [r.verificationUs, r.verificationCycles] }
		],
		shortParamSet: false,
		emptyText: 'No parameter sets match the current filters.'
	},
	chart: {
		axisTitles: {
			pk: 'Public key (bytes)',
			sig: 'Signature (bytes)',
			pkPlusSig: 'pk + sig (bytes)',
			signingCycles: 'Signing (cycles)',
			verificationCycles: 'Verification (cycles)',
			signingUs: 'Signing (µs)',
			verificationUs: 'Verification (µs)',
			signingTime: 'Signing time (µs)',
			verificationTime: 'Verification time (µs)'
		},
		axisShort: {
			pk: 'pk size',
			sig: 'sig size',
			pkPlusSig: 'pk+sig',
			signingCycles: 'signing (cycles)',
			verificationCycles: 'verification (cycles)',
			signingUs: 'signing (µs)',
			verificationUs: 'verification (µs)',
			signingTime: 'signing time',
			verificationTime: 'verification time'
		},
		axisOptions: ['pk', 'sig', 'pkPlusSig', 'signingCycles', 'verificationCycles', 'signingUs', 'verificationUs'],
		value: sigAxisValue,
		views: {
			sizes: { title: 'pk size vs. sig size', x: 'pk', y: 'sig' },
			speed: { title: 'Signing vs. verification time', x: 'signingTime', y: 'verificationTime' }
		},
		sizes: [
			['pk (bytes)', (r) => r.pk],
			['sig (bytes)', (r) => r.sig],
			['pk+sig (bytes)', (r) => r.pkPlusSig]
		],
		ops: [
			{ label: 'Signing', us: (r) => r.signingUs, cycles: (r) => r.signingCycles },
			{ label: 'Verification', us: (r) => r.verificationUs, cycles: (r) => r.verificationCycles }
		],
		rowNotes: false,
		marks: { size: 100, families: { [STANDARDISED]: 180, Lattices: 70 }, ring: 120 }
	}
};

export const KEMS: CompareConfig<KemParameterSet, KemDataRanges, KemFilterState, KemAxisField> = {
	filter: KEM_FILTER,
	table: {
		columns: [
			{ key: 'pk', label: 'pk (B)', size: (r) => r.pk },
			{ key: 'ct', label: 'ct (B)', size: (r) => r.ct },
			{ key: 'pkPlusCt', label: 'pk+ct (B)', size: (r) => r.pkPlusCt },
			{ key: 'keygenUs', label: 'Keygen', time: (r) => [r.keygenUs, r.keygenCycles] },
			{ key: 'encapsUs', label: 'Encaps', time: (r) => [r.encapsUs, r.encapsCycles] },
			{ key: 'decapsUs', label: 'Decaps', time: (r) => [r.decapsUs, r.decapsCycles] }
		],
		shortParamSet: true,
		emptyText: 'No KEMs to display.'
	},
	chart: {
		axisTitles: {
			pk: 'Public key (bytes)',
			ct: 'Ciphertext (bytes)',
			pkPlusCt: 'pk + ct (bytes)',
			keygenCycles: 'Keygen (cycles)',
			encapsCycles: 'Encapsulation (cycles)',
			decapsCycles: 'Decapsulation (cycles)',
			keygenUs: 'Keygen (µs)',
			encapsUs: 'Encapsulation (µs)',
			decapsUs: 'Decapsulation (µs)',
			keygenTime: 'Keygen time (µs)',
			encapsTime: 'Encapsulation time (µs)',
			decapsTime: 'Decapsulation time (µs)'
		},
		axisShort: {
			pk: 'pk size',
			ct: 'ct size',
			pkPlusCt: 'pk+ct',
			keygenCycles: 'keygen (cycles)',
			encapsCycles: 'encaps (cycles)',
			decapsCycles: 'decaps (cycles)',
			keygenUs: 'keygen (µs)',
			encapsUs: 'encaps (µs)',
			decapsUs: 'decaps (µs)',
			keygenTime: 'keygen time',
			encapsTime: 'encaps time',
			decapsTime: 'decaps time'
		},
		axisOptions: ['pk', 'ct', 'pkPlusCt', 'keygenCycles', 'encapsCycles', 'decapsCycles', 'keygenUs', 'encapsUs', 'decapsUs'],
		value: kemAxisValue,
		views: {
			sizes: { title: 'pk size vs. ciphertext size', x: 'pk', y: 'ct' },
			speed: { title: 'Encapsulation vs. decapsulation time', x: 'encapsTime', y: 'decapsTime' }
		},
		sizes: [
			['pk (bytes)', (r) => r.pk],
			['ct (bytes)', (r) => r.ct],
			['pk+ct (bytes)', (r) => r.pkPlusCt]
		],
		ops: [
			{ label: 'Keygen', us: (r) => r.keygenUs, cycles: (r) => r.keygenCycles },
			{ label: 'Encapsulation', us: (r) => r.encapsUs, cycles: (r) => r.encapsCycles },
			{ label: 'Decapsulation', us: (r) => r.decapsUs, cycles: (r) => r.decapsCycles }
		],
		rowNotes: true,
		marks: { size: 120, families: { [STANDARDISED]: 200 }, ring: 140 }
	}
};
