import { allHistory } from './historyData';
import { kemBenchmarkEnv } from './kemSchemeData';
import { latestDate } from './lastDataChange';
import { allSchemeData, benchmarkEnv } from './schemeData';

/** Date of the most recent change to the site's data (history, scheme versions, benchmarks). */
export const lastDataChange: string = latestDate([
	...allHistory.map((e) => e.date),
	...allSchemeData.flatMap((s) => s.versions.map((v) => v.date)),
	...(benchmarkEnv?.environments ?? []).map((e) => e.date),
	...(kemBenchmarkEnv?.environments ?? []).map((e) => e.date)
]);
