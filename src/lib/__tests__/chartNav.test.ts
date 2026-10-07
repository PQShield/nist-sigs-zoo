import { describe, expect, it } from 'vitest';
import { attachChartNav, chartClickHref } from '$lib/chartNav';

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

describe('attachChartNav', () => {
	function setup() {
		const listeners: Record<string, (e: unknown) => void> = {};
		const container = {
			addEventListener: (t: string, h: (e: unknown) => void) => (listeners[t] = h),
			removeEventListener: (t: string) => delete listeners[t]
		} as unknown as HTMLElement;
		let click: (e: Event, item?: { datum?: unknown } | null) => void = () => {};
		const view = { addEventListener: (_t: 'click', h: typeof click) => (click = h) };
		const nav: string[] = [];
		const detach = attachChartNav(container, view, (h) => nav.push(h));
		return { listeners, click: (e: unknown, i?: { datum?: unknown }) => click(e as Event, i), nav, detach };
	}

	it('navigates on a click without movement and ignores a pan after pointerdown', () => {
		const t = setup();
		t.listeners.pointerdown({ clientX: 10, clientY: 10 });
		t.click({ clientX: 11, clientY: 10 }, { datum: DATUM });
		t.listeners.pointerdown({ clientX: 10, clientY: 10 });
		t.click({ clientX: 60, clientY: 10 }, { datum: DATUM });
		expect(t.nav).toEqual(['/schemes/mayo/']);
	});

	it('removes the pointerdown listener on detach', () => {
		const t = setup();
		t.detach();
		expect(t.listeners.pointerdown).toBeUndefined();
	});
});
