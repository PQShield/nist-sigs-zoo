import { afterEach, beforeEach, describe, it, expect, vi } from 'vitest';
import { scheduleUrlSync, urlSyncTarget, type UrlSyncEnv } from '$lib/urlSync';

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

describe('scheduleUrlSync', () => {
	beforeEach(() => vi.useFakeTimers());
	afterEach(() => vi.useRealTimers());

	function env(search = '?a=1') {
		const state = { navigating: false, calls: [] as string[] };
		const e: UrlSyncEnv = {
			location: { pathname: '/kems/', hash: '', search },
			isNavigating: () => state.navigating,
			goto: (t) => state.calls.push(t)
		};
		return { e, state };
	}

	it('syncs after the debounce', () => {
		const { e, state } = env();
		scheduleUrlSync('a=2', e);
		vi.advanceTimersByTime(299);
		expect(state.calls).toEqual([]);
		vi.advanceTimersByTime(1);
		expect(state.calls).toEqual(['?a=2']);
	});

	it('reschedules a sync that falls due during a navigation instead of dropping it', () => {
		const { e, state } = env();
		state.navigating = true;
		scheduleUrlSync('a=2', e);
		vi.advanceTimersByTime(900);
		expect(state.calls).toEqual([]);
		state.navigating = false;
		vi.advanceTimersByTime(300);
		expect(state.calls).toEqual(['?a=2']);
	});

	it('does nothing once cancelled, also after a reschedule', () => {
		const { e, state } = env();
		state.navigating = true;
		const cancel = scheduleUrlSync('a=2', e);
		vi.advanceTimersByTime(600);
		cancel();
		state.navigating = false;
		vi.advanceTimersByTime(3000);
		expect(state.calls).toEqual([]);
	});

	it('skips a no-op sync', () => {
		const { e, state } = env('?a=2');
		scheduleUrlSync('a=2', e);
		vi.advanceTimersByTime(1000);
		expect(state.calls).toEqual([]);
	});
});
