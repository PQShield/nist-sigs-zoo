/** Shared number formatting for the size/performance tables. */

export function fmt(n: number): string {
	return n.toLocaleString();
}

export function fmtCycles(n: number): string {
	if (n <= 0) return '—';
	if (n >= 1_000_000_000) return (n / 1_000_000_000).toFixed(2) + 'G';
	if (n >= 1_000_000) return (n / 1_000_000).toFixed(1) + 'M';
	if (n >= 1_000) return (n / 1_000).toFixed(0) + 'K';
	return String(n);
}

export function fmtTime(us: number): string {
	if (us >= 1_000_000) return (us / 1_000_000).toFixed(2) + ' s';
	if (us >= 1_000) return (us / 1_000).toFixed(2) + ' ms';
	return us.toFixed(1) + ' µs';
}
