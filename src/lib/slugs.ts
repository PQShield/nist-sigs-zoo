/** Scheme-page URLs: /schemes/<slug>/, one namespace shared by signatures and KEMs. */

export type Primitive = 'signature' | 'kem';

export interface SchemeRef {
	primitive: Primitive;
	name: string;
	slug: string;
}

const VALID_SLUG = /^[a-z0-9]+(?:-[a-z0-9]+)*$/;

/** Lowercase ASCII slug: strip diacritics, "+" -> "plus", other runs -> "-". Empty if nothing is left. */
export function slugify(name: string): string {
	return name
		.normalize('NFKD')
		.replace(/[\u0300-\u036f]/g, '')
		.replace(/\+/g, ' plus ')
		.toLowerCase()
		.replace(/[^a-z0-9]+/g, '-')
		.replace(/^-+|-+$/g, '');
}

export function schemeSlug(s: { name: string; slug?: string }): string {
	const slug = s.slug ?? slugify(s.name);
	if (!VALID_SLUG.test(slug)) {
		throw new Error(
			`scheme "${s.name}": slug "${slug}" is not lowercase ASCII (a-z, 0-9, single hyphens); add a "slug:" field to its YAML file`
		);
	}
	return slug;
}

export function buildSchemeIndex(
	sigs: { name: string; slug?: string }[],
	kems: { name: string; slug?: string }[]
): SchemeRef[] {
	const refs: SchemeRef[] = [
		...sigs.map((s) => ({ primitive: 'signature' as const, name: s.name, slug: schemeSlug(s) })),
		...kems.map((k) => ({ primitive: 'kem' as const, name: k.name, slug: schemeSlug(k) }))
	];
	const bySlug = new Map<string, SchemeRef>();
	const byName = new Map<string, SchemeRef>();
	for (const ref of refs) {
		const slugClash = bySlug.get(ref.slug);
		if (slugClash) {
			throw new Error(
				`slug "${ref.slug}" is used by ${slugClash.primitive} "${slugClash.name}" and ${ref.primitive} "${ref.name}"; add a "slug:" field to one of them`
			);
		}
		const nameClash = byName.get(ref.name);
		if (nameClash) {
			throw new Error(
				`name "${ref.name}" is used by a ${nameClash.primitive} and a ${ref.primitive}; history.yaml refers to schemes by name, so names must be unique`
			);
		}
		bySlug.set(ref.slug, ref);
		byName.set(ref.name, ref);
	}
	return refs.sort((a, b) => (a.slug < b.slug ? -1 : a.slug > b.slug ? 1 : 0));
}

export function schemeHref(base: string, slug: string): string {
	return `${base}/schemes/${slug}/`;
}
