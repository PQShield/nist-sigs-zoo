import { compareData, type CompareData } from './compare';
import { processYamlSchemes } from './data';
import { computeKemRanges, processKemSchemes } from './kemData';
import { allKemData } from './kemSchemeData';
import { roundToFilter, type Round } from './roundStore';
import { allSchemeData } from './schemeData';
import type { DataRanges, KemDataRanges, KemParameterSet, ParameterSet } from './types';

/** Signature compare data for an on-ramp round view. */
export function signatureCompareData(round: Round): CompareData<ParameterSet, DataRanges> {
	const { filter, useLatestVersion, includeOtherProgrammes } = roundToFilter(round);
	const { schemes, parameterSets, ranges } = processYamlSchemes(allSchemeData, filter, {
		useLatestVersion,
		includeOtherProgrammes
	});
	return compareData(schemes, parameterSets, ranges);
}

/** KEM compare data (a single snapshot: no rounds). */
export function kemCompareData(): CompareData<KemParameterSet, KemDataRanges> {
	const { schemes, parameterSets } = processKemSchemes(allKemData);
	return compareData(schemes, parameterSets, computeKemRanges(parameterSets));
}
