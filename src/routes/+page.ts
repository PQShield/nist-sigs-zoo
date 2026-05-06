import { parseParameterSets, parseSchemes } from '$lib/data';
import { createFilterStore } from '$lib/filterStore';
import type { PageLoad } from './$types';

export const prerender = true;

export const load: PageLoad = async ({ fetch, url }) => {
	const [schemesText, paramsText] = await Promise.all([
		fetch('/data/schemes.csv').then((r) => r.text()),
		fetch('/data/parametersets.csv').then((r) => r.text()),
	]);

	const schemes = parseSchemes(schemesText);
	const { rows: parameterSets, ranges } = parseParameterSets(paramsText, schemes);

	const categories = [...new Set(schemes.map((s) => s.category))].sort();

	// During prerender url.searchParams is unavailable; URL params applied client-side in onMount
	createFilterStore(parameterSets, ranges, new URLSearchParams());

	return { schemes, parameterSets, ranges, categories };
};
