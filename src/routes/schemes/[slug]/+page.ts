import { error } from '@sveltejs/kit';
import { allHistory } from '$lib/historyData';
import { allKemData, kemBenchmarkEnv } from '$lib/kemSchemeData';
import { allProgrammes } from '$lib/programmeData';
import { allSchemeData, benchmarkEnv } from '$lib/schemeData';
import { schemeIndex } from '$lib/schemeIndex';
import { kemPage, signaturePage, type SchemePageData } from '$lib/schemePage';
import type { EntryGenerator, PageLoad } from './$types';

export const prerender = true;
export const trailingSlash = 'always';

/** Every scheme, including ones no compare view shows by default (e.g. round-1-only schemes). */
export const entries: EntryGenerator = () => schemeIndex.map((r) => ({ slug: r.slug }));

export const load: PageLoad = ({ params }): { page: SchemePageData } => {
	const ref = schemeIndex.find((r) => r.slug === params.slug);
	if (!ref) error(404, `No scheme page "${params.slug}"`);
	if (ref.primitive === 'signature') {
		const scheme = allSchemeData.find((s) => s.name === ref.name)!;
		return { page: signaturePage(scheme, allHistory, allProgrammes, benchmarkEnv?.environments ?? []) };
	}
	const kem = allKemData.find((k) => k.name === ref.name)!;
	return { page: kemPage(kem, allHistory, allProgrammes, kemBenchmarkEnv?.environments ?? []) };
};
