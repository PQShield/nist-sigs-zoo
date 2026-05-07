import type { HistoryEntry } from '$lib/types';

export const prerender = true;

const raw = import.meta.glob('../../../data/history.yaml', { eager: true }) as Record<
	string,
	{ default: HistoryEntry[] }
>;

const entries: HistoryEntry[] = Object.values(raw)[0]?.default ?? [];

export function load() {
	const sorted = [...entries].sort((a, b) => b.date.localeCompare(a.date));
	return { entries: sorted };
}
