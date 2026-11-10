import type { ProgrammeYaml } from './types';

const modules = import.meta.glob('../../data/programmes.yaml', { eager: true }) as Record<
	string,
	{ default: ProgrammeYaml[] }
>;

export const allProgrammes: ProgrammeYaml[] = Object.values(modules)[0]?.default ?? [];
