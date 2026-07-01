import { computeKemRanges, processKemSchemes } from '$lib/kemData';
import { allKemData } from '$lib/kemSchemeData';
import type { PageLoad } from './$types';

export const prerender = true;
export const trailingSlash = 'always';

export const load: PageLoad = async () => {
	const { schemes, parameterSets } = processKemSchemes(allKemData);
	const ranges = computeKemRanges(parameterSets);
	const categories = [...new Set(schemes.map((s) => s.category))].sort();
	return { schemes, parameterSets, ranges, categories };
};
