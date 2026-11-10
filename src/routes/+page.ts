import { allProgrammes } from '#lib/programmeData.ts';
import { programmeSummaries } from '#lib/programmes.ts';
import { allSchemeData } from '#lib/schemeData.ts';
import { allKemData } from '#lib/kemSchemeData.ts';
import { allHistory as history } from '#lib/historyData.ts';

export function load() {
	const pq = (cat: string) => cat !== 'Pre-Quantum';
	return {
		summaries: programmeSummaries(allProgrammes, allSchemeData, allKemData),
		signatureCount: allSchemeData.filter((s) => pq(s.category)).length,
		kemCount: allKemData.filter((k) => pq(k.category)).length,
		latest: [...history].sort((a, b) => b.date.localeCompare(a.date)).slice(0, 3)
	};
}
