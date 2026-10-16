import type { BenchmarkEnv, BenchmarkEnvFile } from './types';

/** Short description of where a run was measured, e.g. "Intel(R) Xeon(R) Platinum 8488C (EC2 c7i.4xlarge), 2026-10-13". */
export function envLabel(env: BenchmarkEnv): string {
	const where = env.instance_type && env.instance_type !== 'local' ? ` (EC2 ${env.instance_type})` : '';
	return `${env.cpu.model}${where}, ${String(env.date).slice(0, 10)}`;
}

/** The distinct CPU models of a file's environments, e.g. "A" or "A and B". */
export function envCpuModels(file: BenchmarkEnvFile | null): string | null {
	const models = [...new Set((file?.environments ?? []).map((e) => e.cpu.model))];
	if (models.length === 0) return null;
	return models.length === 1 ? models[0] : `${models.slice(0, -1).join(', ')} and ${models.at(-1)}`;
}

/** The environment a parameter set's timings come from, if it names one that exists. */
export function envOf(envs: BenchmarkEnv[], id: string | null | undefined): BenchmarkEnv | null {
	return (id && envs.find((e) => e.id === id)) || null;
}
