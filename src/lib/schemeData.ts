import type { BenchmarkEnv, SchemeYaml } from './types';

const modules = import.meta.glob('../../data/schemes/*.yaml', {
	eager: true,
}) as Record<string, { default: SchemeYaml }>;

export const allSchemeData: SchemeYaml[] = Object.values(modules).map((m) => m.default);

const benchEnvModule = import.meta.glob('../../data/benchmark_env.yaml', {
	eager: true,
}) as Record<string, { default: BenchmarkEnv }>;

export const benchmarkEnv: BenchmarkEnv | null =
	Object.values(benchEnvModule)[0]?.default ?? null;
