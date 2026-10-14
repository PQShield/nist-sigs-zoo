import type { Chip } from './badges';
import { EVENT_LABELS } from './schemeHistory';
import type { HistoryEntry, HistoryType, KemSchemeYaml, ProgrammeYaml, SchemeYaml } from './types';

/** Filter option for entries tied to no programme (site news such as page launches). Never used in YAML. */
export const SITE_PROGRAMME = 'site';

/** Event types in filter order. */
export const HISTORY_TYPES: readonly HistoryType[] = ['milestone', 'standardization', 'attack', 'update'];

/** Display labels (British spelling; shared with the scheme pages). */
export const HISTORY_TYPE_LABELS: Record<HistoryType, string> = EVENT_LABELS;

export function isHistoryType(v: unknown): v is HistoryType {
	return (HISTORY_TYPES as readonly unknown[]).includes(v);
}

export interface TimelineItem {
	/** Unique and stable: date plus position in data/history.yaml. */
	key: string;
	entry: HistoryEntry;
	/** Programme ids in registry order, or [SITE_PROGRAMME] when none applies. */
	programmes: string[];
}

export interface ProgrammeOption {
	id: string;
	label: string;
}

export interface TimelineFilter {
	programmes: Set<string>;
	types: Set<HistoryType>;
}

/** Scheme name → every programme any signature version or KEM of that name belongs to. */
export function schemeProgrammeIndex(sigs: SchemeYaml[], kems: KemSchemeYaml[]): Map<string, Set<string>> {
	const index = new Map<string, Set<string>>();
	const add = (name: string, ids: string[]) => {
		const set = index.get(name) ?? new Set<string>();
		for (const id of ids) set.add(id);
		index.set(name, set);
	};
	for (const s of sigs) add(s.name, s.versions.flatMap((v) => Object.keys(v.programmes ?? {})));
	for (const k of kems) add(k.name, Object.keys(k.programmes ?? {}));
	return index;
}

/** Union of the explicit `programmes:` and the named schemes' programmes, in registry order. */
export function entryProgrammes(
	entry: HistoryEntry,
	index: Map<string, Set<string>>,
	registry: ProgrammeYaml[]
): string[] {
	const ids = new Set(entry.programmes ?? []);
	for (const name of entry.schemes ?? []) for (const id of index.get(name) ?? []) ids.add(id);
	const ordered = registry.map((p) => p.id).filter((id) => ids.has(id));
	return ordered.length > 0 ? ordered : [SITE_PROGRAMME];
}

/** Newest first; entries with the same date keep their file order (Array.prototype.sort is stable). */
export function timelineItems(
	entries: HistoryEntry[],
	index: Map<string, Set<string>>,
	registry: ProgrammeYaml[]
): TimelineItem[] {
	return entries
		.map((entry, i) => ({ key: `${entry.date}#${i}`, entry, programmes: entryProgrammes(entry, index, registry) }))
		.sort((a, b) => b.entry.date.localeCompare(a.entry.date));
}

export function programmeLabel(id: string, registry: ProgrammeYaml[]): string {
	if (id === SITE_PROGRAMME) return 'Site updates';
	const p = registry.find((p) => p.id === id);
	return p?.nav_label ?? p?.short ?? id;
}

/** Programmes that occur in the items, in registry order, then "Site updates" if used. */
export function programmeOptions(items: TimelineItem[], registry: ProgrammeYaml[]): ProgrammeOption[] {
	const used = new Set(items.flatMap((i) => i.programmes));
	return [...registry.map((p) => p.id), SITE_PROGRAMME]
		.filter((id) => used.has(id))
		.map((id) => ({ id, label: programmeLabel(id, registry) }));
}

/** Event types that occur in the items, in HISTORY_TYPES order. */
export function presentTypes(items: TimelineItem[]): HistoryType[] {
	const used = new Set(items.map((i) => i.entry.type));
	return HISTORY_TYPES.filter((t) => used.has(t));
}

export function entryChips(programmes: string[], options: ProgrammeOption[]): Chip[] {
	return programmes.map(
		(id): Chip => ({
			label: options.find((o) => o.id === id)?.label ?? id,
			tone: 'plain',
			...(id === SITE_PROGRAMME ? {} : { programme: id })
		})
	);
}

export function defaultTimelineFilter(programmes: string[], types: HistoryType[]): TimelineFilter {
	return { programmes: new Set(programmes), types: new Set(types) };
}

export function filterTimeline(items: TimelineItem[], f: TimelineFilter): TimelineItem[] {
	return items.filter((i) => f.types.has(i.entry.type) && i.programmes.some((p) => f.programmes.has(p)));
}

const list = (v: string | null): string[] => (v ?? '').split(',').filter(Boolean);

/** Missing param = all, empty = none, unknown values ignored. `programmes`/`types` are the offered options. */
export function decodeTimelineFilter(
	params: URLSearchParams,
	programmes: string[],
	types: HistoryType[]
): TimelineFilter {
	const f = defaultTimelineFilter(programmes, types);
	if (params.has('p')) f.programmes = new Set(list(params.get('p')).filter((id) => programmes.includes(id)));
	if (params.has('t')) {
		f.types = new Set(list(params.get('t')).filter((t): t is HistoryType => (types as string[]).includes(t)));
	}
	return f;
}

/** Only params that differ from "everything offered"; values in option order. */
export function encodeTimelineFilter(f: TimelineFilter, programmes: string[], types: HistoryType[]): URLSearchParams {
	const params = new URLSearchParams();
	const isAll = <T>(all: readonly T[], chosen: Set<T>) => chosen.size === all.length && all.every((v) => chosen.has(v));
	if (!isAll(programmes, f.programmes)) params.set('p', programmes.filter((id) => f.programmes.has(id)).join(','));
	if (!isAll(types, f.types)) params.set('t', types.filter((t) => f.types.has(t)).join(','));
	return params;
}

export function timelineSummary(shown: number, total: number): string {
	const noun = total === 1 ? 'event' : 'events';
	return shown === total ? `Showing all ${total} ${noun}` : `Showing ${shown} of ${total} ${noun}`;
}
