import type { BenchmarkEnvFile, KemSchemeYaml } from './types';

const modules = import.meta.glob('../../data/kems/*.yaml', {
	eager: true
}) as Record<string, { default: KemSchemeYaml }>;

export const allKemData: KemSchemeYaml[] = Object.values(modules).map((m) => m.default);

const benchEnvModule = import.meta.glob('../../data/kem_benchmark_env.yaml', {
	eager: true
}) as Record<string, { default: BenchmarkEnvFile }>;

/** Every environment our timings come from (one per imported run; usually one). */
export const kemBenchmarkEnv: BenchmarkEnvFile | null = Object.values(benchEnvModule)[0]?.default ?? null;
