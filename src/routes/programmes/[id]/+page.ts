import { error } from '@sveltejs/kit';
import { allProgrammes } from '$lib/programmeData';
import type { EntryGenerator, PageLoad } from './$types';

export const prerender = true;

/** Prerender a page for every programme in data/programmes.yaml. */
export const entries: EntryGenerator = () => allProgrammes.map((p) => ({ id: p.id }));

export const load: PageLoad = ({ params }) => {
	const programme = allProgrammes.find((p) => p.id === params.id);
	if (!programme) error(404, `Unknown programme "${params.id}"`);
	return { programme };
};
