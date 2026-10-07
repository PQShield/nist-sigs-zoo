import { describe, expect, it } from 'vitest';
import { chartClickHref } from '$lib/chartNav';

const DATUM = { scheme: 'MAYO', href: '/schemes/mayo/' };

describe('chartClickHref', () => {
	it('returns the point href for a plain click', () => {
		expect(chartClickHref(DATUM, { x: 10, y: 10 }, { x: 12, y: 13 })).toBe('/schemes/mayo/');
	});

	it('ignores a drag (pan) that ends on a point', () => {
		expect(chartClickHref(DATUM, { x: 10, y: 10 }, { x: 40, y: 10 })).toBeNull();
	});

	it('accepts a click when no mousedown was seen (e.g. a tap)', () => {
		expect(chartClickHref(DATUM, null, { x: 0, y: 0 })).toBe('/schemes/mayo/');
	});

	it('ignores background clicks and data without an href', () => {
		expect(chartClickHref(undefined, null, { x: 0, y: 0 })).toBeNull();
		expect(chartClickHref({ scheme: 'X' }, null, { x: 0, y: 0 })).toBeNull();
		expect(chartClickHref({ href: 42 }, null, { x: 0, y: 0 })).toBeNull();
		expect(chartClickHref({ href: '' }, null, { x: 0, y: 0 })).toBeNull();
	});
});
