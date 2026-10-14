/**
 * The newest date (YYYY-MM-DD) among the given dates and timestamps. Used for the site's
 * "last data change": every data change adds a history.yaml entry (see CLAUDE.md), new
 * scheme versions carry a date, and benchmark imports stamp the benchmark env files.
 */
export function latestDate(dates: (string | null | undefined)[]): string {
	return (
		dates
			.map((d) => (d ?? '').slice(0, 10))
			.filter((d) => /^\d{4}-\d{2}-\d{2}$/.test(d))
			.sort()
			.at(-1) ?? ''
	);
}
