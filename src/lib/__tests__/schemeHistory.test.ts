import { describe, expect, it } from 'vitest';
import { schemeEvents } from '$lib/schemeHistory';
import type { HistoryEntry } from '$lib/types';

const HISTORY: HistoryEntry[] = [
	{ date: '2025-01-01', type: 'update', description: 'v2 released', schemes: ['Toy'] },
	{ date: '2025-01-01', type: 'attack', description: 'same-day attack', schemes: ['Toy', 'Other'] },
	{ date: '2024-06-01', type: 'milestone', description: 'round 2', schemes: ['Other', 'Toy'] },
	{ date: '2024-03-01', type: 'update', description: 'other only', schemes: ['Other'] },
	{ date: '2023-01-01', type: 'milestone', description: 'no schemes' }
];

const VERSIONS = [
	{ version: 'v2', date: '2025-01-01' },
	{ version: 'v1', date: '2024-01-01' }
];

describe('schemeEvents', () => {
	it('keeps only entries naming the scheme, merges same-day updates into versions, newest first', () => {
		expect(schemeEvents(HISTORY, 'Toy', VERSIONS)).toEqual([
			{ date: '2025-01-01', kind: 'version', title: 'v2', html: 'v2 released' },
			{ date: '2025-01-01', kind: 'attack', title: null, html: 'same-day attack' },
			{ date: '2024-06-01', kind: 'milestone', title: null, html: 'round 2' },
			{ date: '2024-01-01', kind: 'version', title: 'v1', html: null }
		]);
	});

	it('matches names exactly, not as substrings', () => {
		expect(schemeEvents(HISTORY, 'Oth', [])).toEqual([]);
	});

	it('works without versions (KEMs)', () => {
		expect(schemeEvents(HISTORY, 'Other', []).map((e) => e.kind)).toEqual(['attack', 'milestone', 'update']);
	});
});
