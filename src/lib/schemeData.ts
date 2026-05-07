import type { SchemeYaml } from './types';

const modules = import.meta.glob('../../data/schemes/*.yaml', {
	eager: true,
}) as Record<string, { default: SchemeYaml }>;

export const allSchemeData: SchemeYaml[] = Object.values(modules).map((m) => m.default);

export const lastUpdated: string = allSchemeData
	.flatMap((s) => s.versions.map((v) => v.date))
	.filter(Boolean)
	.sort()
	.at(-1) ?? '';
