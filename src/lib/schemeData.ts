import type { BenchmarkEnvFile, SchemeYaml } from './types';

const modules = import.meta.glob('../../data/schemes/*.yaml', {
	eager: true
}) as Record<string, { default: SchemeYaml }>;

export const allSchemeData: SchemeYaml[] = Object.values(modules).map((m) => m.default);

const benchEnvModule = import.meta.glob('../../data/benchmark_env.yaml', {
	eager: true
}) as Record<string, { default: BenchmarkEnvFile }>;

/** Every environment our timings come from (one per imported run; usually one). */
export const benchmarkEnv: BenchmarkEnvFile | null = Object.values(benchEnvModule)[0]?.default ?? null;
