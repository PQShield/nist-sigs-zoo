import { processYamlSchemes } from '$lib/data';
import { createFilterStore } from '$lib/filterStore';
import { allSchemeData } from '$lib/schemeData';
import type { PageLoad } from './$types';

export const prerender = true;

export const load: PageLoad = async () => {
	const { schemes, parameterSets, ranges } = processYamlSchemes(allSchemeData, 'round-2');

	const categories = [...new Set(schemes.map((s) => s.category))].sort();

	// During prerender url.searchParams is unavailable; URL params applied client-side in onMount
	createFilterStore(parameterSets, ranges, new URLSearchParams());

	return { schemes, parameterSets, ranges, categories };
};
