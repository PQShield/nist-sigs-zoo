import { allHistory } from '$lib/historyData';
import { allKemData } from '$lib/kemSchemeData';
import { allProgrammes } from '$lib/programmeData';
import { allSchemeData } from '$lib/schemeData';
import { presentTypes, programmeOptions, schemeProgrammeIndex, timelineItems } from '$lib/timeline';

export const prerender = true;
export const trailingSlash = 'always';

export function load() {
	const items = timelineItems(allHistory, schemeProgrammeIndex(allSchemeData, allKemData), allProgrammes);
	return {
		items,
		programmeOptions: programmeOptions(items, allProgrammes),
		types: presentTypes(items)
	};
}
