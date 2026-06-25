import type { KemSchemeYaml } from './types';

const modules = import.meta.glob('../../data/kems/*.yaml', {
	eager: true,
}) as Record<string, { default: KemSchemeYaml }>;

export const allKemData: KemSchemeYaml[] = Object.values(modules).map((m) => m.default);
