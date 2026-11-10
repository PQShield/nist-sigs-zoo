import { describe, expect, it } from 'vitest';
import { latestDate } from '$lib/lastDataChange';

describe('latestDate', () => {
	it('picks the newest date and trims timestamps to the day', () => {
		expect(latestDate(['2026-09-20', '2026-10-13T15:19:58Z', '2026-10-08'])).toBe('2026-10-13');
	});

	it('ignores missing and malformed values', () => {
		expect(latestDate([null, undefined, '', 'soon', '2025-01-02'])).toBe('2025-01-02');
		expect(latestDate([])).toBe('');
	});
});
