const MONTHS = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];

/**
 * "2026-08-31" -> "31 Aug 2026", "2025-01" -> "Jan 2025", "2021" -> "2021". Parsed by hand: no time
 * zone, no locale-dependent month names. Anything else (including a bad month) is returned unchanged.
 */
export function fmtDate(date: string): string {
	const m = /^(\d{4})(?:-(\d{2})(?:-(\d{2}))?)?$/.exec(date);
	if (!m) return date;
	const [, year, month, day] = m;
	if (!month) return year;
	const mon = MONTHS[Number(month) - 1];
	if (!mon) return date;
	return day ? `${Number(day)} ${mon} ${year}` : `${mon} ${year}`;
}
