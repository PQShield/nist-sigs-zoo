const MONTHS = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];

/** "2026-08-31" -> "31 Aug 2026". Parsed by hand: no time zone, no locale-dependent month names. */
export function fmtDate(iso: string): string {
	const m = /^(\d{4})-(\d{2})-(\d{2})/.exec(iso);
	if (!m) return iso;
	const month = MONTHS[Number(m[2]) - 1];
	return month ? `${Number(m[3])} ${month} ${m[1]}` : iso;
}
