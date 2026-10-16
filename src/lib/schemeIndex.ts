import { allKemData } from './kemSchemeData';
import { allSchemeData } from './schemeData';
import { buildSchemeIndex, type SchemeRef } from './slugs';

/** Every scheme page, built once from the bundled YAML. Throws at build time on a slug or name clash. */
export const schemeIndex: SchemeRef[] = buildSchemeIndex(allSchemeData, allKemData);

const byName = new Map(schemeIndex.map((r) => [r.name, r]));

export function slugOf(name: string): string {
	const ref = byName.get(name);
	if (!ref) throw new Error(`no scheme page for "${name}"`);
	return ref.slug;
}
