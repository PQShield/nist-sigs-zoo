import { allKemData, processKems } from '$lib/kemData';
import { createKemStore } from '$lib/kemStore';
import type { PageLoad } from './$types';

export const prerender = true;

export const load: PageLoad = async () => {
	const { kems, parameterSets, ranges } = processKems(allKemData);
	const categories = [...new Set(kems.map((k) => k.category))].sort();

	// During prerender url.searchParams is unavailable; URL params applied client-side in onMount
	createKemStore(parameterSets, new URLSearchParams());

	return { kems, parameterSets, ranges, categories };
};
