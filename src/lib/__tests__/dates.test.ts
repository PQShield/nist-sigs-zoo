import { describe, expect, it } from 'vitest';
import { fmtDate } from '$lib/dates';

describe('fmtDate', () => {
	it('formats ISO dates as "D Mon YYYY" without time-zone shifts', () => {
		expect(fmtDate('2026-08-31')).toBe('31 Aug 2026');
		expect(fmtDate('2026-09-01')).toBe('1 Sep 2026');
		expect(fmtDate('2025-01-01')).toBe('1 Jan 2025');
	});

	it('returns anything that is not an ISO date unchanged', () => {
		expect(fmtDate('soon')).toBe('soon');
	});
});
