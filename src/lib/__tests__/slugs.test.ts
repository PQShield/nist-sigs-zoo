import { describe, expect, it } from 'vitest';
import { buildSchemeIndex, schemeHref, schemeSlug, slugify } from '$lib/slugs';

describe('slugify', () => {
	it.each([
		['MAYO', 'mayo'],
		['ML-KEM', 'ml-kem'],
		['NTRU+', 'ntru-plus'],
		['Aigis-Sig+', 'aigis-sig-plus'],
		['EHTv3 / EHTv4', 'ehtv3-ehtv4'],
		['eMLE-Sig 2.0', 'emle-sig-2-0'],
		['Xifrat1-Sign.I', 'xifrat1-sign-i'],
		['Classic McEliece', 'classic-mceliece'],
		['Café', 'cafe']
	])('%s -> %s', (name, slug) => {
		expect(slugify(name)).toBe(slug);
	});

	it('returns an empty string for a name without ASCII letters or digits', () => {
		expect(slugify('格密码')).toBe('');
	});
});

describe('schemeSlug', () => {
	it('uses the explicit slug when given', () => {
		expect(schemeSlug({ name: '格密码', slug: 'lattice-sig' })).toBe('lattice-sig');
	});

	it('rejects a name that gives no slug, and says how to fix it', () => {
		expect(() => schemeSlug({ name: '格密码' })).toThrow(/格密码.*slug:/);
	});

	it('rejects an explicit slug that is not lowercase ASCII', () => {
		expect(() => schemeSlug({ name: 'X', slug: 'Bad Slug' })).toThrow(/Bad Slug/);
	});
});

describe('buildSchemeIndex', () => {
	it('indexes both primitives, sorted by slug', () => {
		expect(buildSchemeIndex([{ name: 'MAYO' }], [{ name: 'ML-KEM' }, { name: 'BAT' }])).toEqual([
			{ primitive: 'kem', name: 'BAT', slug: 'bat' },
			{ primitive: 'signature', name: 'MAYO', slug: 'mayo' },
			{ primitive: 'kem', name: 'ML-KEM', slug: 'ml-kem' }
		]);
	});

	it('rejects a slug shared by a signature and a KEM, naming both', () => {
		expect(() => buildSchemeIndex([{ name: 'Foo+' }], [{ name: 'foo plus' }])).toThrow(
			/signature "Foo\+".*kem "foo plus".*slug:/
		);
	});

	it('rejects the same name used twice, even with distinct slugs', () => {
		expect(() => buildSchemeIndex([{ name: 'Foo' }], [{ name: 'Foo', slug: 'foo-kem' }])).toThrow(/name "Foo"/);
	});

	it('lets an explicit slug resolve a clash', () => {
		const index = buildSchemeIndex([{ name: 'Foo+' }], [{ name: 'foo plus', slug: 'foo-plus-kem' }]);
		expect(index.map((r) => r.slug)).toEqual(['foo-plus', 'foo-plus-kem']);
	});
});

describe('schemeHref', () => {
	it('prefixes the base path and ends with a slash', () => {
		expect(schemeHref('', 'mayo')).toBe('/schemes/mayo/');
		expect(schemeHref('/nist-sigs-zoo', 'mayo')).toBe('/nist-sigs-zoo/schemes/mayo/');
	});
});
