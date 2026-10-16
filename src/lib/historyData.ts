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
const loaded = Object.values(modules)[0]?.default;
if (!loaded) throw new Error('data/history.yaml not found: the history glob matched no file');
export const allHistory: HistoryEntry[] = loaded;
