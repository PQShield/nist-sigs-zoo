import { describe, it, expect } from 'vitest';
import { readFileSync, readdirSync } from 'node:fs';
import { join } from 'node:path';
import yaml from 'js-yaml';
import { envCpuModels, envLabel, envOf } from '$lib/benchEnv';
import type { BenchmarkEnv, BenchmarkEnvFile } from '$lib/types';

const env = (id: string, model: string, instance_type?: string): BenchmarkEnv => ({
	id,
	date: `${id.slice(0, 10)}T00:00:00Z`,
	instance_type,
	cpu: { model, cores: 1, threads_per_core: 1, max_freq_mhz: 0, governor: '', turbo: '' },
	os: '',
	kernel: '',
	compiler: '',
	openssl: '',
	notes: ''
});

describe('benchEnv helpers', () => {
	it('labels environments with their CPU, machine and date', () => {
		expect(envLabel(env('2026-10-13-c7i.4xlarge', 'Xeon', 'c7i.4xlarge'))).toBe('Xeon (EC2 c7i.4xlarge), 2026-10-13');
		expect(envLabel(env('2026-06-25-local-i7', 'Core i7', 'local'))).toBe('Core i7, 2026-06-25');
	});

	it('lists distinct CPU models and finds environments by id', () => {
		const file: BenchmarkEnvFile = {
			license: 'CC-BY-4.0',
			attribution: 'x',
			environments: [env('2026-10-16-a', 'A'), env('2026-10-13-b', 'B'), env('2026-10-12-c', 'A')]
		};
		expect(envCpuModels(file)).toBe('A and B');
		expect(envCpuModels({ ...file, environments: [] })).toBeNull();
		expect(envOf(file.environments, '2026-10-13-b')?.cpu.model).toBe('B');
		expect(envOf(file.environments, 'nope')).toBeNull();
		expect(envOf(file.environments, null)).toBeNull();
	});
});

describe('benchmark environment files', () => {
	const DATA = join(process.cwd(), 'data');
	const ids = (node: unknown, out: string[] = []): string[] => {
		if (Array.isArray(node)) node.forEach((n) => ids(n, out));
		else if (node && typeof node === 'object') {
			const o = node as Record<string, unknown>;
			if (typeof o.bench_env === 'string') out.push(o.bench_env);
			Object.values(o).forEach((n) => ids(n, out));
		}
		return out;
	};
	for (const [envFile, dir] of [
		['benchmark_env.yaml', 'schemes'],
		['kem_benchmark_env.yaml', 'kems']
	]) {
		it(`${envFile}: every bench_env in data/${dir} names an environment, and every environment is used`, () => {
			const file = yaml.load(readFileSync(join(DATA, envFile), 'utf8')) as BenchmarkEnvFile;
			const known = new Set(file.environments.map((e) => e.id));
			expect(known.size).toBe(file.environments.length);
			const used = new Set(
				readdirSync(join(DATA, dir))
					.filter((f) => f.endsWith('.yaml'))
					.flatMap((f) => ids(yaml.load(readFileSync(join(DATA, dir, f), 'utf8'))))
			);
			expect([...used].filter((id) => !known.has(id))).toEqual([]);
			expect([...known].filter((id) => !used.has(id))).toEqual([]);
		});
	}
});
