import type { HistoryEntry } from './types';

export type SchemeEventKind = HistoryEntry['type'] | 'version';

export interface SchemeEvent {
	date: string;
	kind: SchemeEventKind;
	/** Version label for 'version' events. */
	title: string | null;
	/** Trusted HTML from history.yaml. */
	html: string | null;
}

export const EVENT_LABELS: Record<SchemeEventKind, string> = {
	version: 'New version',
	update: 'Update',
	attack: 'Attack',
	milestone: 'Milestone',
	standardization: 'Standardisation'
};

const KIND_ORDER: SchemeEventKind[] = ['version', 'attack', 'update', 'milestone', 'standardization'];

/**
 * The scheme's slice of history.yaml plus one 'version' event per version. An 'update' entry on
 * the same date as a version is merged into it. Newest first; same-day events in KIND_ORDER.
 */
export function schemeEvents(
	history: HistoryEntry[],
	name: string,
	versions: { version: string; date: string }[]
): SchemeEvent[] {
	const mine = history.filter((e) => e.schemes?.includes(name));
	const merged = new Set<HistoryEntry>();
	const events: SchemeEvent[] = versions.map((v) => {
		const update = mine.find((e) => e.type === 'update' && e.date === v.date && !merged.has(e));
		if (update) merged.add(update);
		return { date: v.date, kind: 'version', title: v.version, html: update?.description ?? null };
	});
	for (const e of mine) {
		if (!merged.has(e)) events.push({ date: e.date, kind: e.type, title: null, html: e.description });
	}
	return events.sort(
		(a, b) => b.date.localeCompare(a.date) || KIND_ORDER.indexOf(a.kind) - KIND_ORDER.indexOf(b.kind)
	);
}
