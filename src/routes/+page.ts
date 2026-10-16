import { allProgrammes } from '$lib/programmeData';
import { programmeSummaries } from '$lib/programmes';
import { allSchemeData } from '$lib/schemeData';
import { allKemData } from '$lib/kemSchemeData';
import { allHistory as history } from '$lib/historyData';

export function load() {
	const pq = (cat: string) => cat !== 'Pre-Quantum';
	return {
		summaries: programmeSummaries(allProgrammes, allSchemeData, allKemData),
		signatureCount: allSchemeData.filter((s) => pq(s.category)).length,
		kemCount: allKemData.filter((k) => pq(k.category)).length,
		latest: [...history].sort((a, b) => b.date.localeCompare(a.date)).slice(0, 3)
	};
}
