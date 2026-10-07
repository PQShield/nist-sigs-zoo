import { describe, it, expect } from 'vitest';
import { urlSyncTarget } from '$lib/urlSync';

describe('urlSyncTarget', () => {
	it('skips when the query string already matches', () => {
		expect(urlSyncTarget('a=1', '/kems/', '', '?a=1', false)).toBeNull();
	});
	it('skips when both are empty (including a bare "?")', () => {
		expect(urlSyncTarget('', '/kems/', '', '', false)).toBeNull();
		expect(urlSyncTarget('', '/kems/', '', '?', false)).toBeNull();
	});
	it('skips while a navigation is in progress', () => {
		expect(urlSyncTarget('a=2', '/kems/', '', '?a=1', true)).toBeNull();
	});
	it('returns the new query, keeping the hash', () => {
		expect(urlSyncTarget('a=2', '/kems/', '#x', '?a=1', false)).toBe('?a=2#x');
	});
	it('returns the pathname when clearing filters', () => {
		expect(urlSyncTarget('', '/kems/', '', '?a=1', false)).toBe('/kems/');
	});
});
