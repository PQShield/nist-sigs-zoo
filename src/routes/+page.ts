import { processYamlSchemes } from '$lib/data';
import { createFilterStore } from '$lib/filterStore';
import type { SchemeYaml } from '$lib/types';
import type { PageLoad } from './$types';

const schemeModules = import.meta.glob('../../data/schemes/*.yaml', {
	eager: true,
}) as Record<string, { default: SchemeYaml }>;

export const prerender = true;

export const load: PageLoad = async () => {
	const schemeData = Object.values(schemeModules).map((m) => m.default);
	const { schemes, parameterSets, ranges } = processYamlSchemes(schemeData);

	const categories = [...new Set(schemes.map((s) => s.category))].sort();

	// During prerender url.searchParams is unavailable; URL params applied client-side in onMount
	createFilterStore(parameterSets, ranges, new URLSearchParams());

	return { schemes, parameterSets, ranges, categories };
};
