import { allHistory } from '#lib/historyData.ts';
import { allKemData } from '#lib/kemSchemeData.ts';
import { allProgrammes } from '#lib/programmeData.ts';
import { allSchemeData } from '#lib/schemeData.ts';
import { presentTypes, programmeOptions, schemeProgrammeIndex, timelineItems } from '#lib/timeline.ts';

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
