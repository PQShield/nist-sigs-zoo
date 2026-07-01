import type { BenchmarkEnv, KemSchemeYaml } from './types';

const modules = import.meta.glob('../../data/kems/*.yaml', {
	eager: true,
}) as Record<string, { default: KemSchemeYaml }>;

export const allKemData: KemSchemeYaml[] = Object.values(modules).map((m) => m.default);

const benchEnvModule = import.meta.glob('../../data/kem_benchmark_env.yaml', {
	eager: true,
}) as Record<string, { default: BenchmarkEnv }>;

export const kemBenchmarkEnv: BenchmarkEnv | null =
	Object.values(benchEnvModule)[0]?.default ?? null;
