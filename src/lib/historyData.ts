import type { HistoryEntry } from './types';

const modules = import.meta.glob('../../data/history.yaml', { eager: true }) as Record<
	string,
	{ default: HistoryEntry[] }
>;

/**
 * The single runtime loader for data/history.yaml (all entries, file order). Pages and plan 6's
 * timeline should import this rather than loading the YAML themselves. Filter per scheme with
 * `schemeEvents()` from `$lib/schemeHistory`. Not importable from unit tests (needs Vite).
 */
export const allHistory: HistoryEntry[] = Object.values(modules)[0]?.default ?? [];
