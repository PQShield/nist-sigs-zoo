import type { FilterSpec } from './compare';
import { kemTimes, sigTimes } from './timing';
import type { DataRanges, FilterState, KemDataRanges, KemFilterState, KemParameterSet, ParameterSet } from './types';

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
