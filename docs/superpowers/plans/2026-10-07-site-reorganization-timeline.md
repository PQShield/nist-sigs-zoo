# Site Reorganization: Timeline Filters and Polish Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let readers filter `/timeline/` by programme and event type, with the choice kept in the URL. Each event shows chips for its programmes, and scheme names can link to scheme pages once they exist. This plan also fixes the carry-over polish items from plans 1–2 that still apply, and brings `CLAUDE.md` up to date.

**Architecture:** All timeline logic is pure and lives in a new module, `src/lib/timeline.ts`. It covers:
- building a scheme → programmes index from the signature and KEM YAML;
- working out each entry's programmes: the union of its `schemes:` memberships and an optional explicit `programmes:` list, or the pseudo-programme `site` when neither gives one;
- the `TimelineFilter` predicate;
- the URL codec (`p`, `t`);
- labels and chips.

A second pure module, `src/lib/schemeLinks.ts`, owns the scheme-page slug and an `href` helper that returns `null` until plan 5 turns on `SCHEME_PAGES`. The timeline route builds the items in its prerendered `load` from a new `src/lib/historyData.ts` loader (shared with the home page). The page keeps the filter in a local `$state` rune, the same design as the KEM page, and syncs it to the URL with `goto(..., { replaceState: true })`. Data integrity checks for `data/history.yaml` go into `dataFiles.test.ts`.

**Tech Stack:** SvelteKit 2 + Svelte 5 runes, adapter-static, Tailwind v4 (`pqs-*` tokens in `src/routes/layout.css`), TypeScript, js-yaml, Vitest, Playwright.

**Spec:** The controller brief for plan 6 (Thom, 2026-10-07): a timeline filter by programme and event type in the URL, programme chips per entry, conditional scheme links, carry-over polish and CLAUDE.md. The site rename and domain are **out of scope** (Thom deferred them). Also read the plan-1 roadmap entry 6 in `docs/superpowers/plans/2026-10-07-site-reorganization-foundation.md`, and `.superpowers/sdd/carryover.md` for the polish items.

## Global Constraints

- Everything stays on branch `feat/programmes-foundation`. Never switch branches. Small focused commits, at least one per task. Stage specific files (never `git add -A` / `git add .`). End every commit message with the trailer on its own line: `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- Plans 3 (NGCC import), 4 (programme pages, home, nav) and 5 (scheme pages) run in parallel on the same branch. Before editing a file another plan also touches (see **Overlaps**), re-read it, keep the other plan's changes, and make only the edit this plan describes.
- Never import third-party benchmark numbers. This plan adds no timings.
- Old-URL compatibility is low priority. `/history/` → `/timeline/` must keep working (existing `e2e/redirects.spec.ts`), but don't add compatibility code for anything else.
- Copy uses British spelling: "Standardisation", "programme". The YAML value `standardization` stays as it is (it is a data key, not copy).
- Programme ids are exactly `nist-pqc`, `nist-onramp`, `china-ngcc`, `kpqc`, `iso`, and `data/programmes.yaml` is the only registry. The timeline's extra filter option is the pseudo-id `site`, which never appears in YAML.
- Timeline URL params: `p` (comma-separated programme ids, including `site`) and `t` (comma-separated event types). A missing param means "all", an empty value (`p=`) means "none", and unknown values are ignored. A param equal to the default is left out of the URL.
- Accessibility: filter groups are `<fieldset>`s with a `<legend>`, every checkbox has a visible label, the result count is a `role="status"` live region, chips and scheme lists carry screen-reader prefixes, and text meets 4.5:1 contrast in both themes.
- Pure logic goes in `src/lib/*.ts` with Vitest tests in `src/lib/__tests__/`. Unit tests never import `$app/*`, `$lib/schemeData`, `$lib/kemSchemeData`, `$lib/programmeData` or `$lib/historyData`.
- Run `npm run check` and `npm run test` before every commit. Run `npm run test:e2e` (or the touched spec: `npx playwright test e2e/<spec>.ts`) in tasks that touch components or routes.
- Do not modify `round-1/`.

## Review Focus

1. **Site news with no `schemes:` and no `programmes:`** (for example "Published a standalone KEMs comparison page"): it must appear by default, carry a "Site updates" chip, and disappear only when "Site updates" is unticked. It must not vanish as soon as any programme box is touched. Pinned by Task 1 `entryProgrammes` "falls back to site" and `filterTimeline` "site entries" tests.
2. **A typo or a renamed scheme in `data/history.yaml`** (`schemes: [SABRE]`), or plan 3 adding an NGCC history entry before the candidate's YAML exists: `npm run test` must fail and name the entry. The entry must not silently lose its chips. Pinned by Task 3 `historyErrors` fixture tests.
3. **An unquoted date** (`- date: 2026-10-07`): js-yaml turns it into a `Date`, and the `localeCompare` sort in `load` throws at build time. The data test must catch it first, naming the entry. Pinned by Task 3 "rejects an unquoted date".
4. **A hand-edited link such as `/timeline/?p=bogus&t=attack,foo` or `?p=`:** unknown values are ignored, an empty programme list shows "No events match these filters." with a working "Show all events" button, and the page never throws. Pinned by Task 1 codec tests and Task 4 e2e "empty programme list".
5. **A scheme in several programmes, or one name used by both a signature and a KEM** (possible once NGCC lands): the entry gets the union of the programmes, each chip once, in registry order. Pinned by Task 1 `schemeProgrammeIndex` / `entryProgrammes` "Dual" tests.

---

## File Structure

| File | Responsibility |
|---|---|
| `src/lib/types.ts` (modify) | `HistoryType`; `HistoryEntry.programmes?` |
| `src/lib/timeline.ts` (create) | Scheme → programme index, entry programmes, items, options and labels, chips, filter predicate, URL codec, count summary |
| `src/lib/schemeLinks.ts` (create) | `SCHEME_PAGES` switch, `schemeSlug()`, `schemeHref()` |
| `src/lib/historyData.ts` (create) | `import.meta.glob` loader → `allHistory: HistoryEntry[]` |
| `src/lib/__tests__/timeline.test.ts` (create) | Unit tests for `timeline.ts` |
| `src/lib/__tests__/schemeLinks.test.ts` (create) | Unit tests for `schemeLinks.ts` |
| `src/lib/__tests__/dataFiles.test.ts` (modify) | `data/history.yaml` integrity; programme-primitive coverage (carry-over) |
| `src/routes/timeline/+page.ts` (modify) | Builds items, programme options and present types |
| `src/routes/timeline/+page.svelte` (rewrite) | Filter column, live count, events with type badge, programme chips and scheme names/links, URL sync |
| `src/routes/+page.ts` (modify, one import) | Uses `allHistory` instead of its own glob (carry-over) |
| `e2e/timeline.spec.ts` (create) | Timeline filters, URL round trip, empty state |
| `src/lib/components/OmittedNote.svelte` (create) | "N parameter sets are not shown" note, shared by both scatter plots |
| `src/routes/layout.css` (modify) | `pqs-violet*` tokens for that note |
| `src/lib/components/ScatterPlot.svelte`, `KemScatterPlot.svelte` (modify) | Use `OmittedNote`; ignore stale renders after the async import |
| `src/lib/components/SchemeTable.svelte`, `KemTable.svelte` (modify) | Time cells use `displayUs()` |
| `src/routes/signatures/advanced/+page.svelte`, `src/routes/kems/advanced/+page.svelte` (modify) | Contrast of the back link, axis labels and scale buttons; one `getFilterStore()` call (signatures) |
| `src/lib/__tests__/compareFilters.test.ts` (modify) | Hide-flag codec cases (carry-over) |
| `e2e/signatures.spec.ts` (modify) | Speed-view omitted note e2e (carry-over) |
| `CLAUDE.md` (modify) | History schema, timeline route and modules, test lists |

## Overlaps with plans 3–5

| File | Other plan | How to handle it |
|---|---|---|
| `data/history.yaml` | Plan 3 adds NGCC entries | This plan does **not** edit it. Plan 3 entries may use `programmes: [china-ngcc]`. Task 3's tests require every named scheme to exist in `data/schemes`/`data/kems`, so plan 3 must commit candidate YAML no later than history entries that name them. |
| `src/lib/__tests__/dataFiles.test.ts` | Plan 3 (NGCC checks) | Append new `describe` blocks only; re-read before editing. |
| `src/lib/types.ts` | Plans 3, 4, 5 | Only the `HistoryEntry` block changes. |
| `src/routes/+page.ts` (home) | Plan 4 owns the home page | One-line import swap (Task 4). If plan 4 already rewrote the file, apply the same swap to its history glob, or leave it and note it for plan 4. |
| `src/routes/+layout.svelte` (nav) | Plan 4 | Not touched. The carry-over items for the layout (the repeated `startsWith`, no mobile-nav e2e) are left to plan 4. |
| `src/lib/schemeLinks.ts` | Plan 5 | Plan 5 sets `SCHEME_PAGES = true` and must build `/schemes/[slug]/` from `schemeSlug()`. If plan 5 lands first with its own slug helper, Task 2 reuses it and keeps the `schemeHref(name, base, enabled?)` signature. |
| `CLAUDE.md` | All plans | Task 6 runs last; re-read first and edit only the sections it names. |

---

### Task 1: Pure timeline logic

**Files:**
- Modify: `src/lib/types.ts` (the `HistoryEntry` interface)
- Create: `src/lib/timeline.ts`
- Test: `src/lib/__tests__/timeline.test.ts`

**Interfaces:**
- Consumes: `isProgrammeGroup`, `PROGRAMME_GROUP_LABELS` (`$lib/compareFilters`); `STANDARDS_PROGRAMME` (`$lib/programmes`); `Chip` (`$lib/badges`); `SchemeYaml`, `KemSchemeYaml`, `ProgrammeYaml` (`$lib/types`).
- Produces:
  - `type HistoryType = 'update' | 'attack' | 'standardization' | 'milestone'`; `HistoryEntry.programmes?: string[]`
  - `SITE_PROGRAMME = 'site'`
  - `HISTORY_TYPES: readonly HistoryType[]`, `HISTORY_TYPE_LABELS: Record<HistoryType, string>`, `isHistoryType(v: unknown): v is HistoryType`
  - `interface TimelineItem { key: string; entry: HistoryEntry; programmes: string[] }`
  - `interface ProgrammeOption { id: string; label: string }`
  - `interface TimelineFilter { programmes: Set<string>; types: Set<HistoryType> }`
  - `schemeProgrammeIndex(sigs: SchemeYaml[], kems: KemSchemeYaml[]): Map<string, Set<string>>`
  - `entryProgrammes(entry: HistoryEntry, index: Map<string, Set<string>>, registry: ProgrammeYaml[]): string[]`
  - `timelineItems(entries: HistoryEntry[], index: Map<string, Set<string>>, registry: ProgrammeYaml[]): TimelineItem[]`
  - `programmeLabel(id: string, registry: ProgrammeYaml[]): string`
  - `programmeOptions(items: TimelineItem[], registry: ProgrammeYaml[]): ProgrammeOption[]`
  - `presentTypes(items: TimelineItem[]): HistoryType[]`
  - `entryChips(programmes: string[], options: ProgrammeOption[]): Chip[]`
  - `defaultTimelineFilter(programmes: string[], types: HistoryType[]): TimelineFilter`
  - `filterTimeline(items: TimelineItem[], f: TimelineFilter): TimelineItem[]`
  - `decodeTimelineFilter(params: URLSearchParams, programmes: string[], types: HistoryType[]): TimelineFilter`
  - `encodeTimelineFilter(f: TimelineFilter, programmes: string[], types: HistoryType[]): URLSearchParams`
  - `timelineSummary(shown: number, total: number): string`

- [ ] **Step 1: Extend the history type**

In `src/lib/types.ts`, replace

```ts
export interface HistoryEntry {
	date: string;
	type: 'update' | 'attack' | 'standardization' | 'milestone';
	description: string;
	schemes?: string[];
}
```

with

```ts
export type HistoryType = 'update' | 'attack' | 'standardization' | 'milestone';

export interface HistoryEntry {
	/** ISO date, quoted in YAML so it stays a string. */
	date: string;
	type: HistoryType;
	/** Trusted HTML from our own YAML. */
	description: string;
	/** Affected scheme names (signature or KEM `name:`); their programmes are derived. */
	schemes?: string[];
	/** Extra programme ids (data/programmes.yaml), added to those derived from `schemes`. */
	programmes?: string[];
}
```

- [ ] **Step 2: Write the failing tests**

Create `src/lib/__tests__/timeline.test.ts`:

```ts
import { describe, expect, it } from 'vitest';
import {
	decodeTimelineFilter,
	defaultTimelineFilter,
	encodeTimelineFilter,
	entryChips,
	entryProgrammes,
	filterTimeline,
	HISTORY_TYPE_LABELS,
	isHistoryType,
	presentTypes,
	programmeLabel,
	programmeOptions,
	schemeProgrammeIndex,
	SITE_PROGRAMME,
	timelineItems,
	timelineSummary
} from '$lib/timeline';
import type { HistoryEntry, HistoryType, KemSchemeYaml, ProgrammeYaml, SchemeYaml } from '$lib/types';

const programme = (id: string, short: string): ProgrammeYaml => ({
	id,
	name: id,
	short,
	organiser: short,
	region: 'Somewhere',
	primitives: ['signature', 'kem'],
	stages: [{ id: 'round-1', label: 'round 1' }]
});

// Registry order, as in data/programmes.yaml.
const REGISTRY: ProgrammeYaml[] = [
	programme('nist-pqc', 'NIST'),
	programme('nist-onramp', 'On-ramp'),
	programme('china-ngcc', 'NGCC'),
	programme('kpqc', 'KpqC'),
	programme('iso', 'ISO')
];

function sig(name: string, ...versions: Record<string, string>[]): SchemeYaml {
	return {
		name,
		website: '',
		category: 'Lattice',
		assumption: '',
		versions: versions.map((programmes, i) => ({
			version: `v${i}`,
			date: `2026-0${i + 1}-01`,
			status: 'On-ramp',
			programmes,
			parametersets: []
		}))
	};
}

function kem(name: string, programmes?: Record<string, string>): KemSchemeYaml {
	return { name, website: '', category: 'Lattice', assumption: '', status: 'Research', programmes, parametersets: [] } as KemSchemeYaml;
}

// MAYO: the newest version has left the on-ramp map (post-round update) but MAYO still counts as on-ramp.
// Dual: a signature in KpqC and a KEM of the same name in NGCC.
const INDEX = schemeProgrammeIndex(
	[sig('MAYO', { 'nist-onramp': 'round-1' }, {}), sig('ML-DSA', { 'nist-pqc': 'standardised' }), sig('Dual', { kpqc: 'round-1' })],
	[kem('Dual', { 'china-ngcc': 'round-1' }), kem('ML-KEM', { 'nist-pqc': 'standardised' }), kem('Plain')]
);

const entry = (o: Partial<HistoryEntry> = {}): HistoryEntry => ({ date: '2026-01-01', type: 'update', description: 'x', ...o });

describe('schemeProgrammeIndex', () => {
	it('collects programmes from every version', () => {
		expect([...INDEX.get('MAYO')!]).toEqual(['nist-onramp']);
	});
	it('merges a signature and a KEM that share a name', () => {
		expect([...INDEX.get('Dual')!].sort()).toEqual(['china-ngcc', 'kpqc']);
	});
	it('keeps schemes with no programmes as an empty set', () => {
		expect(INDEX.get('Plain')!.size).toBe(0);
	});
});

describe('entryProgrammes', () => {
	it('derives programmes from the named schemes', () => {
		expect(entryProgrammes(entry({ schemes: ['MAYO'] }), INDEX, REGISTRY)).toEqual(['nist-onramp']);
	});
	it('returns registry order without duplicates', () => {
		expect(entryProgrammes(entry({ schemes: ['Dual', 'ML-KEM'], programmes: ['kpqc'] }), INDEX, REGISTRY)).toEqual([
			'nist-pqc',
			'china-ngcc',
			'kpqc'
		]);
	});
	it('adds explicit programmes to the derived ones', () => {
		expect(entryProgrammes(entry({ schemes: ['MAYO'], programmes: ['nist-pqc'] }), INDEX, REGISTRY)).toEqual([
			'nist-pqc',
			'nist-onramp'
		]);
	});
	it('uses explicit programmes alone when no scheme is named', () => {
		expect(entryProgrammes(entry({ programmes: ['china-ngcc'] }), INDEX, REGISTRY)).toEqual(['china-ngcc']);
	});
	it('falls back to site for entries tied to no programme', () => {
		expect(entryProgrammes(entry(), INDEX, REGISTRY)).toEqual([SITE_PROGRAMME]);
		expect(entryProgrammes(entry({ schemes: ['Plain'] }), INDEX, REGISTRY)).toEqual([SITE_PROGRAMME]);
	});
	it('ignores unknown schemes and programme ids (the data test reports them)', () => {
		expect(entryProgrammes(entry({ schemes: ['Nope'], programmes: ['bogus'] }), INDEX, REGISTRY)).toEqual([SITE_PROGRAMME]);
	});
});

describe('timelineItems', () => {
	const entries = [
		entry({ date: '2026-01-01', description: 'old' }),
		entry({ date: '2026-03-01', description: 'new A' }),
		entry({ date: '2026-03-01', description: 'new B' })
	];
	const items = timelineItems(entries, INDEX, REGISTRY);
	it('sorts newest first and keeps file order within a date', () => {
		expect(items.map((i) => i.entry.description)).toEqual(['new A', 'new B', 'old']);
	});
	it('gives every item a unique key', () => {
		expect(new Set(items.map((i) => i.key)).size).toBe(3);
	});
	it('leaves the input unsorted', () => {
		expect(entries[0].description).toBe('old');
	});
});

describe('options and labels', () => {
	const items = timelineItems(
		[entry({ schemes: ['Dual'] }), entry({ type: 'attack', schemes: ['MAYO'] }), entry({ type: 'milestone' })],
		INDEX,
		REGISTRY
	);
	it('offers only programmes that occur, in registry order, then site', () => {
		expect(programmeOptions(items, REGISTRY).map((o) => o.id)).toEqual(['nist-onramp', 'china-ngcc', 'kpqc', SITE_PROGRAMME]);
	});
	it('offers only event types that occur, in filter order', () => {
		expect(presentTypes(items)).toEqual(['milestone', 'attack', 'update']);
	});
	it('labels programmes like the compare-page filter', () => {
		expect(programmeLabel('nist-pqc', REGISTRY)).toBe('NIST PQC');
		expect(programmeLabel('nist-onramp', REGISTRY)).toBe('NIST on-ramp');
		expect(programmeLabel('china-ngcc', REGISTRY)).toBe('China NGCC');
		expect(programmeLabel(SITE_PROGRAMME, REGISTRY)).toBe('Site updates');
	});
	it('labels types in British spelling', () => {
		expect(HISTORY_TYPE_LABELS.standardization).toBe('Standardisation');
	});
	it('recognises event types', () => {
		expect(isHistoryType('attack')).toBe(true);
		expect(isHistoryType('standardisation')).toBe(false);
	});
	it('builds chips with the option labels; NGCC gets the "new" tone', () => {
		const options = programmeOptions(items, REGISTRY);
		expect(entryChips(['china-ngcc', 'kpqc'], options)).toEqual([
			{ label: 'China NGCC', tone: 'new' },
			{ label: 'KpqC', tone: 'plain' }
		]);
	});
});

describe('filterTimeline', () => {
	const items = timelineItems(
		[entry({ schemes: ['MAYO'] }), entry({ type: 'attack', schemes: ['Dual'] }), entry({ description: 'site news' })],
		INDEX,
		REGISTRY
	);
	const programmes = programmeOptions(items, REGISTRY).map((o) => o.id);
	const types = presentTypes(items);
	const all = defaultTimelineFilter(programmes, types);

	it('shows everything by default, including site entries', () => {
		expect(filterTimeline(items, all)).toHaveLength(3);
	});
	it('hides site entries only when site is unticked', () => {
		const noOnramp = { ...all, programmes: new Set(programmes.filter((p) => p !== 'nist-onramp')) };
		expect(filterTimeline(items, noOnramp).map((i) => i.entry.description)).toContain('site news');
		const noSite = { ...all, programmes: new Set(programmes.filter((p) => p !== SITE_PROGRAMME)) };
		expect(filterTimeline(items, noSite).map((i) => i.entry.description)).not.toContain('site news');
	});
	it('keeps an entry when any of its programmes is ticked', () => {
		const kpqcOnly = { ...all, programmes: new Set(['kpqc']) };
		expect(filterTimeline(items, kpqcOnly).map((i) => i.entry.type)).toEqual(['attack']);
	});
	it('filters by type', () => {
		const attacks = { ...all, types: new Set<HistoryType>(['attack']) };
		expect(filterTimeline(items, attacks)).toHaveLength(1);
	});
});

describe('timeline URL codec', () => {
	const programmes = ['nist-onramp', 'kpqc', SITE_PROGRAMME];
	const types: HistoryType[] = ['milestone', 'attack', 'update'];
	const all = defaultTimelineFilter(programmes, types);

	it('leaves the defaults out of the URL', () => {
		expect(encodeTimelineFilter(all, programmes, types).toString()).toBe('');
	});
	it('round-trips a narrowed filter in option order', () => {
		const f = { programmes: new Set(['kpqc', 'nist-onramp']), types: new Set<HistoryType>(['update', 'attack']) };
		const p = encodeTimelineFilter(f, programmes, types);
		expect(p.get('p')).toBe('nist-onramp,kpqc');
		expect(p.get('t')).toBe('attack,update');
		expect(decodeTimelineFilter(p, programmes, types)).toEqual(f);
	});
	it('reads a missing param as all', () => {
		expect(decodeTimelineFilter(new URLSearchParams('t=attack'), programmes, types).programmes).toEqual(new Set(programmes));
	});
	it('reads an empty param as none, and encodes none as an empty param', () => {
		const f = decodeTimelineFilter(new URLSearchParams('p='), programmes, types);
		expect(f.programmes.size).toBe(0);
		expect(encodeTimelineFilter(f, programmes, types).toString()).toBe('p=');
	});
	it('ignores unknown values', () => {
		const f = decodeTimelineFilter(new URLSearchParams('p=bogus,kpqc&t=attack,foo,standardization'), programmes, types);
		expect([...f.programmes]).toEqual(['kpqc']);
		// standardization is a real type but not offered (absent from the data), so it is dropped too.
		expect([...f.types]).toEqual(['attack']);
	});
	it('reads only-unknown values as none', () => {
		expect(decodeTimelineFilter(new URLSearchParams('p=bogus'), programmes, types).programmes.size).toBe(0);
	});
});

describe('timelineSummary', () => {
	it('says "all" when nothing is filtered out', () => {
		expect(timelineSummary(28, 28)).toBe('Showing all 28 events');
		expect(timelineSummary(1, 1)).toBe('Showing all 1 event');
	});
	it('gives shown and total otherwise', () => {
		expect(timelineSummary(2, 28)).toBe('Showing 2 of 28 events');
		expect(timelineSummary(0, 28)).toBe('Showing 0 of 28 events');
	});
});
```

- [ ] **Step 3: Run the tests to verify they fail**

Run: `npx vitest run src/lib/__tests__/timeline.test.ts`
Expected: FAIL, "Failed to resolve import "$lib/timeline"".

- [ ] **Step 4: Write the implementation**

Create `src/lib/timeline.ts`:

```ts
import type { Chip } from './badges';
import { isProgrammeGroup, PROGRAMME_GROUP_LABELS } from './compareFilters';
import { STANDARDS_PROGRAMME } from './programmes';
import type { HistoryEntry, HistoryType, KemSchemeYaml, ProgrammeYaml, SchemeYaml } from './types';

/** Filter option for entries tied to no programme (site news such as page launches). Never used in YAML. */
export const SITE_PROGRAMME = 'site';

/** Event types in filter order. */
export const HISTORY_TYPES: readonly HistoryType[] = ['milestone', 'standardization', 'attack', 'update'];

/** Display labels. British spelling; the YAML value stays `standardization`. */
export const HISTORY_TYPE_LABELS: Record<HistoryType, string> = {
	milestone: 'Milestone',
	standardization: 'Standardisation',
	attack: 'Attack',
	update: 'Update'
};

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
	if (id === STANDARDS_PROGRAMME) return 'NIST PQC';
	if (isProgrammeGroup(id)) return PROGRAMME_GROUP_LABELS[id];
	return registry.find((p) => p.id === id)?.short ?? id;
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
	return programmes.map((id): Chip => ({
		label: options.find((o) => o.id === id)?.label ?? id,
		tone: id === 'china-ngcc' ? 'new' : 'plain'
	}));
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
```

- [ ] **Step 5: Run the tests to verify they pass**

Run: `npx vitest run src/lib/__tests__/timeline.test.ts`
Expected: PASS (all tests).

Note: `p.get('p')` returns the decoded value. `URLSearchParams.toString()` encodes `,` as `%2C`, and `decodeTimelineFilter` reads both forms. The test `toString()).toBe('p=')` has no comma, so it is unaffected.

- [ ] **Step 6: Type-check and run the full unit suite**

Run: `npm run check && npm run test`
Expected: 0 errors; all tests pass. The current timeline page indexes `typeLabel[entry.type]` with a `Record<string, string>`, so it still type-checks.

- [ ] **Step 7: Commit**

```bash
git add src/lib/types.ts src/lib/timeline.ts src/lib/__tests__/timeline.test.ts
git commit -m "Add timeline programme/type filter logic and URL codec

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 2: Scheme-page link helper

**Files:**
- Create: `src/lib/schemeLinks.ts`
- Test: `src/lib/__tests__/schemeLinks.test.ts`

**Interfaces:**
- Consumes: nothing.
- Produces: `SCHEME_PAGES: boolean` (false until plan 5); `schemeSlug(name: string): string`; `schemeHref(name: string, base: string, enabled?: boolean): string | null` (`enabled` defaults to `SCHEME_PAGES`). Task 4 calls `schemeHref(name, base)`.

First check: if `src/lib/schemeLinks.ts` or another scheme-slug helper already exists (plan 5 landed first), don't create a second one. Re-export or adapt it so `schemeHref(name, base, enabled?)` exists with this signature, and skip to Step 4.

- [ ] **Step 1: Write the failing tests**

Create `src/lib/__tests__/schemeLinks.test.ts`:

```ts
import { describe, expect, it } from 'vitest';
import { SCHEME_PAGES, schemeHref, schemeSlug } from '$lib/schemeLinks';

describe('schemeSlug', () => {
	it.each([
		['ML-DSA', 'ml-dsa'],
		['QR-UOV', 'qr-uov'],
		['NTRU+', 'ntru-plus'],
		['SPHINCS+', 'sphincs-plus'],
		['EHTv3 / EHTv4', 'ehtv3-ehtv4'],
		['eMLE-Sig 2.0', 'emle-sig-2-0'],
		['Xifrat1-Sign.I', 'xifrat1-sign-i'],
		['Classic McEliece', 'classic-mceliece']
	])('%s → %s', (name, slug) => {
		expect(schemeSlug(name)).toBe(slug);
	});
});

describe('schemeHref', () => {
	it('returns null while scheme pages are switched off', () => {
		expect(schemeHref('MAYO', '/nist-sigs-zoo', false)).toBeNull();
	});
	it('builds a base-prefixed link when switched on', () => {
		expect(schemeHref('NTRU+', '/nist-sigs-zoo', true)).toBe('/nist-sigs-zoo/schemes/ntru-plus/');
		expect(schemeHref('ML-DSA', '', true)).toBe('/schemes/ml-dsa/');
	});
	it('follows SCHEME_PAGES by default', () => {
		expect(schemeHref('MAYO', '') === null).toBe(!SCHEME_PAGES);
	});
});
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `npx vitest run src/lib/__tests__/schemeLinks.test.ts`
Expected: FAIL, "Failed to resolve import "$lib/schemeLinks"".

- [ ] **Step 3: Write the implementation**

Create `src/lib/schemeLinks.ts`:

```ts
/**
 * Whether /schemes/[slug]/ pages exist. Plan 5 (scheme pages) sets this to true and builds its
 * routes from schemeSlug(), so every link made through schemeHref() resolves.
 */
export const SCHEME_PAGES = false;

/** URL slug for a scheme name: lower case, "+" → "-plus", other non-alphanumerics → single "-". */
export function schemeSlug(name: string): string {
	return name
		.toLowerCase()
		.replace(/\+/g, '-plus')
		.replace(/[^a-z0-9]+/g, '-')
		.replace(/^-+|-+$/g, '');
}

/** Link to a scheme's page, or null while scheme pages don't exist (render the name as text). */
export function schemeHref(name: string, base: string, enabled: boolean = SCHEME_PAGES): string | null {
	return enabled ? `${base}/schemes/${schemeSlug(name)}/` : null;
}
```

- [ ] **Step 4: Run the tests to verify they pass**

Run: `npx vitest run src/lib/__tests__/schemeLinks.test.ts && npm run check`
Expected: PASS; 0 errors.

- [ ] **Step 5: Commit**

```bash
git add src/lib/schemeLinks.ts src/lib/__tests__/schemeLinks.test.ts
git commit -m "Add a scheme-page link helper, off until scheme pages exist

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 3: History data integrity checks

**Files:**
- Modify: `src/lib/__tests__/dataFiles.test.ts` (append a `describe` block; add one import)

**Interfaces:**
- Consumes: `isHistoryType` from `$lib/timeline` (Task 1).
- Produces: nothing for later tasks. The test guards `data/history.yaml` for plan 3's NGCC entries.

The checker is a small function inside the test file, so its failure cases can be pinned with fixtures. Never break the real `data/history.yaml` to watch a test fail: other plans edit it in the same working tree.

- [ ] **Step 1: Write the tests**

In `src/lib/__tests__/dataFiles.test.ts`, add to the imports:

```ts
import { isHistoryType } from '$lib/timeline';
```

and append at the end of the file:

```ts
describe('data/history.yaml', () => {
	type Entry = { date?: unknown; type?: unknown; description?: unknown; schemes?: unknown; programmes?: unknown };

	/** One message per problem, naming the entry by position and date. */
	function historyErrors(entries: Entry[], schemeNames: Set<string>, programmeIds: Set<string>): string[] {
		return entries.flatMap((e, i) => {
			const where = `history.yaml entry ${i + 1} (${String(e.date)})`;
			const errors: string[] = [];
			if (typeof e.date !== 'string' || !/^\d{4}-\d{2}-\d{2}$/.test(e.date)) {
				errors.push(`${where}: date must be a quoted 'YYYY-MM-DD' string`);
			}
			if (!isHistoryType(e.type)) errors.push(`${where}: unknown type "${String(e.type)}"`);
			if (typeof e.description !== 'string' || e.description.trim() === '') errors.push(`${where}: empty description`);
			for (const [field, known] of [
				['schemes', schemeNames],
				['programmes', programmeIds]
			] as const) {
				const value = e[field];
				if (value === undefined) continue;
				if (!Array.isArray(value) || value.length === 0) {
					errors.push(`${where}: ${field} must be a non-empty list`);
					continue;
				}
				for (const v of value) if (!known.has(v)) errors.push(`${where}: unknown ${field.slice(0, -1)} "${v}"`);
			}
			return errors;
		});
	}

	const yamlFiles = (dir: string) => readdirSync(join(DATA, dir)).filter((f) => f.endsWith('.yaml'));
	const schemeNames = new Set([
		...yamlFiles('schemes').map((f) => load<SchemeYaml>(join(DATA, 'schemes', f)).name),
		...yamlFiles('kems').map((f) => load<KemSchemeYaml>(join(DATA, 'kems', f)).name)
	]);
	const programmeIds = new Set(programmes.map((p) => p.id));

	it('every entry is well formed and names only known schemes and programmes', () => {
		const history = load<Entry[]>(join(DATA, 'history.yaml'));
		expect(history.length).toBeGreaterThan(0);
		expect(historyErrors(history, schemeNames, programmeIds)).toEqual([]);
	});

	it('reports an unknown scheme or programme, naming the entry', () => {
		const bad: Entry[] = [{ date: '2026-01-02', type: 'attack', description: 'x', schemes: ['SABRE'], programmes: ['ngcc'] }];
		expect(historyErrors(bad, new Set(['SABER']), programmeIds)).toEqual([
			'history.yaml entry 1 (2026-01-02): unknown scheme "SABRE"',
			'history.yaml entry 1 (2026-01-02): unknown programme "ngcc"'
		]);
	});

	it('rejects an unquoted date (js-yaml reads it as a Date)', () => {
		const parsed = yaml.load("- date: 2026-01-02\n  type: update\n  description: x\n") as Entry[];
		expect(historyErrors(parsed, schemeNames, programmeIds)[0]).toMatch(/entry 1 .*date must be a quoted/);
	});

	it('rejects an unknown type, an empty description and an empty list', () => {
		const bad: Entry[] = [{ date: '2026-01-02', type: 'standardisation', description: ' ', schemes: [] }];
		expect(historyErrors(bad, schemeNames, programmeIds)).toEqual([
			'history.yaml entry 1 (2026-01-02): unknown type "standardisation"',
			'history.yaml entry 1 (2026-01-02): empty description',
			'history.yaml entry 1 (2026-01-02): schemes must be a non-empty list'
		]);
	});
});
```

- [ ] **Step 2: Run the tests**

Run: `npx vitest run src/lib/__tests__/dataFiles.test.ts`
Expected: PASS. The fixture tests prove the checker bites, and the real-data test proves today's file is clean. If the real-data test fails, it names the entry. Fix the data only if the entry is wrong (check with `git log -p data/history.yaml`). If another plan's uncommitted entry is the cause, report it instead of editing.

- [ ] **Step 3: Commit**

```bash
git add src/lib/__tests__/dataFiles.test.ts
git commit -m "Check history entries' dates, types, schemes and programmes

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 4: Timeline page with filters, chips and scheme links

**Files:**
- Create: `src/lib/historyData.ts`
- Modify: `src/routes/timeline/+page.ts`
- Rewrite: `src/routes/timeline/+page.svelte`
- Modify: `src/routes/+page.ts` (the history glob only; plan-4 overlap)
- Test: `e2e/timeline.spec.ts`

**Interfaces:**
- Consumes: everything in Task 1's **Produces**; `schemeHref` (Task 2); `allProgrammes` (`$lib/programmeData`), `allSchemeData` (`$lib/schemeData`), `allKemData` (`$lib/kemSchemeData`); `ProgrammeChips.svelte` (`chips: Chip[]`); `FILTER_HEADING`, `FILTER_OPTION`, `FILTER_OPTION_TEXT` (`$lib/components/filterStyles`).
- Produces: `allHistory: HistoryEntry[]` (`$lib/historyData`). The page data is `{ items: TimelineItem[]; programmeOptions: ProgrammeOption[]; types: HistoryType[] }`.

- [ ] **Step 1: Write the failing e2e tests**

Create `e2e/timeline.spec.ts`:

```ts
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import yaml from 'js-yaml';
import { test, expect, type Page } from '@playwright/test';

// Expected counts come from the YAML (the source of truth), not from the rendered page.
const DATA = path.join(path.dirname(fileURLToPath(import.meta.url)), '..', 'data');
const load = <T>(file: string): T => yaml.load(fs.readFileSync(file, 'utf8')) as T;
const yamlFiles = (dir: string) => fs.readdirSync(path.join(DATA, dir)).filter((f) => f.endsWith('.yaml'));

interface Entry {
	type: string;
	schemes?: string[];
	programmes?: string[];
}
const HISTORY = load<Entry[]>(path.join(DATA, 'history.yaml'));

function programmesByScheme(): Map<string, Set<string>> {
	const index = new Map<string, Set<string>>();
	const add = (name: string, ids: string[]) => {
		const set = index.get(name) ?? new Set<string>();
		ids.forEach((id) => set.add(id));
		index.set(name, set);
	};
	for (const f of yamlFiles('schemes')) {
		const s = load<{ name: string; versions: { programmes?: Record<string, unknown> }[] }>(path.join(DATA, 'schemes', f));
		add(s.name, s.versions.flatMap((v) => Object.keys(v.programmes ?? {})));
	}
	for (const f of yamlFiles('kems')) {
		const k = load<{ name: string; programmes?: Record<string, unknown> }>(path.join(DATA, 'kems', f));
		add(k.name, Object.keys(k.programmes ?? {}));
	}
	return index;
}
const INDEX = programmesByScheme();
const inProgramme = (e: Entry, id: string) =>
	(e.programmes ?? []).includes(id) || (e.schemes ?? []).some((n) => INDEX.get(n)?.has(id));

const count = (page: Page) => page.getByRole('status').filter({ hasText: /^Showing/ });
const events = (page: Page) => page.getByRole('list', { name: 'Events' }).locator(':scope > li');

async function open(page: Page, url: string) {
	await page.goto(url);
	await page.waitForLoadState('networkidle');
}

test.describe('timeline', () => {
	test('lists every event by default, newest first', async ({ page }) => {
		await open(page, '/timeline/');
		await expect(count(page)).toHaveText(`Showing all ${HISTORY.length} events`);
		await expect(events(page)).toHaveCount(HISTORY.length);
		const dates = await page.getByRole('list', { name: 'Events' }).locator('time').allTextContents();
		expect(dates).toEqual([...dates].sort().reverse());
	});

	test('filters by event type and keeps the choice in the URL', async ({ page }) => {
		await open(page, '/timeline/');
		const types = page.getByRole('group', { name: 'Event type' });
		for (const name of ['Update', 'Milestone', 'Standardisation']) {
			const box = types.getByRole('checkbox', { name, exact: true });
			if (await box.count()) await box.uncheck();
		}
		const attacks = HISTORY.filter((e) => e.type === 'attack').length;
		await expect(count(page)).toHaveText(`Showing ${attacks} of ${HISTORY.length} events`);
		await expect(page).toHaveURL(/[?&]t=attack(&|$)/);

		await page.reload();
		await page.waitForLoadState('networkidle');
		await expect(count(page)).toHaveText(`Showing ${attacks} of ${HISTORY.length} events`);
		await expect(types.getByRole('checkbox', { name: 'Update', exact: true })).not.toBeChecked();
		await expect(types.getByRole('checkbox', { name: 'Attack', exact: true })).toBeChecked();
	});

	test('?p=kpqc shows only KpqC events, each with a KpqC chip', async ({ page }) => {
		const n = HISTORY.filter((e) => inProgramme(e, 'kpqc')).length;
		expect(n).toBeGreaterThan(0);
		await open(page, '/timeline/?p=kpqc');
		await expect(count(page)).toHaveText(`Showing ${n} of ${HISTORY.length} events`);
		await expect(events(page)).toHaveCount(n);
		for (let i = 0; i < n; i++) await expect(events(page).nth(i)).toContainText('KpqC');
		const programmes = page.getByRole('group', { name: 'Programme' });
		await expect(programmes.getByRole('checkbox', { name: 'KpqC', exact: true })).toBeChecked();
		await expect(programmes.getByRole('checkbox', { name: 'NIST on-ramp', exact: true })).not.toBeChecked();
	});

	test('an empty programme list shows the empty state and a reset', async ({ page }) => {
		await open(page, '/timeline/?p=');
		await expect(page.getByText('No events match these filters.')).toBeVisible();
		await page.getByRole('button', { name: 'Show all events' }).click();
		await expect(count(page)).toHaveText(`Showing all ${HISTORY.length} events`);
		await expect(page).not.toHaveURL(/[?&]p=/);
	});
});
```

- [ ] **Step 2: Run the e2e tests to verify they fail**

Run: `npx playwright test e2e/timeline.spec.ts`
Expected: FAIL. There is no `status` region, no "Events" list and no filter groups yet.

- [ ] **Step 3: Add the shared history loader**

Create `src/lib/historyData.ts`:

```ts
import type { HistoryEntry } from './types';

const modules = import.meta.glob('../../data/history.yaml', { eager: true }) as Record<
	string,
	{ default: HistoryEntry[] }
>;

/** data/history.yaml in file order. */
export const allHistory: HistoryEntry[] = Object.values(modules)[0]?.default ?? [];
```

In `src/routes/+page.ts` (home), re-read the file first because plan 4 may have changed it. Then replace

```ts
import type { HistoryEntry } from '$lib/types';

const raw = import.meta.glob('../../data/history.yaml', { eager: true }) as Record<
	string,
	{ default: HistoryEntry[] }
>;
const history: HistoryEntry[] = Object.values(raw)[0]?.default ?? [];
```

with

```ts
import { allHistory as history } from '$lib/historyData';
```

Leave the rest of the file as it is (`latest: [...history].sort(...)` keeps working).

- [ ] **Step 4: Rewrite the timeline load**

Replace the whole of `src/routes/timeline/+page.ts` with:

```ts
import { allHistory } from '$lib/historyData';
import { allKemData } from '$lib/kemSchemeData';
import { allProgrammes } from '$lib/programmeData';
import { allSchemeData } from '$lib/schemeData';
import { presentTypes, programmeOptions, schemeProgrammeIndex, timelineItems } from '$lib/timeline';

export const prerender = true;
export const trailingSlash = 'always';

export function load() {
	const items = timelineItems(allHistory, schemeProgrammeIndex(allSchemeData, allKemData), allProgrammes);
	return {
		items,
		programmeOptions: programmeOptions(items, allProgrammes),
		types: presentTypes(items)
	};
}
```

- [ ] **Step 5: Rewrite the timeline page**

Replace the whole of `src/routes/timeline/+page.svelte` with:

```svelte
<script lang="ts">
	import { onMount } from 'svelte';
	import { goto } from '$app/navigation';
	import { base } from '$app/paths';
	import { SITE_NAME } from '$lib/constants';
	import ProgrammeChips from '$lib/components/ProgrammeChips.svelte';
	import { FILTER_HEADING, FILTER_OPTION, FILTER_OPTION_TEXT } from '$lib/components/filterStyles';
	import { schemeHref } from '$lib/schemeLinks';
	import {
		decodeTimelineFilter,
		defaultTimelineFilter,
		encodeTimelineFilter,
		entryChips,
		filterTimeline,
		HISTORY_TYPE_LABELS,
		timelineSummary,
		type TimelineFilter
	} from '$lib/timeline';
	import type { HistoryType } from '$lib/types';
	import type { PageData } from './$types';

	let { data }: { data: PageData } = $props();

	const programmeIds = data.programmeOptions.map((o) => o.id);

	// The prerendered page shows everything; the URL's filter is applied once hydrated.
	let filter = $state<TimelineFilter>(defaultTimelineFilter(programmeIds, data.types));
	let ready = $state(false);
	const shown = $derived(filterTimeline(data.items, filter));

	onMount(() => {
		filter = decodeTimelineFilter(new URLSearchParams(location.search), programmeIds, data.types);
		ready = true;
	});

	$effect(() => {
		if (!ready) return;
		const qs = encodeTimelineFilter(filter, programmeIds, data.types).toString();
		if (qs === location.search.replace(/^\?/, '')) return;
		goto((qs ? `?${qs}` : location.pathname) + location.hash, { replaceState: true, keepFocus: true, noScroll: true });
	});

	function toggleProgramme(id: string) {
		const next = new Set(filter.programmes);
		if (next.has(id)) next.delete(id);
		else next.add(id);
		filter = { ...filter, programmes: next };
	}

	function toggleType(type: HistoryType) {
		const next = new Set(filter.types);
		if (next.has(type)) next.delete(type);
		else next.add(type);
		filter = { ...filter, types: next };
	}

	function showAll() {
		filter = defaultTimelineFilter(programmeIds, data.types);
	}

	const TYPE_BADGE: Record<HistoryType, string> = {
		update: 'bg-blue-100 text-blue-800 dark:bg-blue-900/40 dark:text-blue-300',
		attack: 'bg-red-100 text-red-800 dark:bg-red-900/40 dark:text-red-300',
		standardization: 'bg-green-100 text-green-800 dark:bg-green-900/40 dark:text-green-300',
		milestone: 'bg-purple-100 text-purple-800 dark:bg-purple-900/40 dark:text-purple-300'
	};
</script>

<svelte:head>
	<title>Timeline — {SITE_NAME}</title>
</svelte:head>

<div class="mx-auto max-w-screen-lg px-6 py-10">
	<h1 class="font-heading text-2xl font-bold text-pqs-midnight dark:text-pqs-smoke">Timeline</h1>
	<p class="mt-2 text-sm text-pqs-steel dark:text-pqs-bluegray">
		Scheme data updates, attacks and programme milestones tracked on this site.
	</p>

	<div class="mt-8 grid gap-8 md:grid-cols-[200px_1fr]">
		<form aria-label="Filter events" onsubmit={(e) => e.preventDefault()}>
			<fieldset class="mb-5">
				<legend class={FILTER_HEADING}>Programme</legend>
				<div class="space-y-0.5">
					{#each data.programmeOptions as option (option.id)}
						<label class={FILTER_OPTION}>
							<input
								type="checkbox"
								class="accent-pqs-apricot"
								checked={filter.programmes.has(option.id)}
								onchange={() => toggleProgramme(option.id)}
							/>
							<span class={FILTER_OPTION_TEXT}>{option.label}</span>
						</label>
					{/each}
				</div>
			</fieldset>
			<fieldset class="mb-5">
				<legend class={FILTER_HEADING}>Event type</legend>
				<div class="space-y-0.5">
					{#each data.types as type (type)}
						<label class={FILTER_OPTION}>
							<input
								type="checkbox"
								class="accent-pqs-apricot"
								checked={filter.types.has(type)}
								onchange={() => toggleType(type)}
							/>
							<span class={FILTER_OPTION_TEXT}>{HISTORY_TYPE_LABELS[type]}</span>
						</label>
					{/each}
				</div>
			</fieldset>
			<button
				type="button"
				onclick={showAll}
				class="font-heading text-sm font-semibold text-pqs-steel underline hover:text-pqs-apricot dark:text-pqs-apricot"
			>Show all events</button>
		</form>

		<div>
			<p role="status" class="text-sm text-pqs-steel dark:text-pqs-bluegray">
				{timelineSummary(shown.length, data.items.length)}
			</p>

			{#if shown.length === 0}
				<p class="mt-6 text-sm text-pqs-midnight dark:text-pqs-smoke">No events match these filters.</p>
			{/if}

			<ol aria-label="Events" class="mt-6 space-y-0">
				{#each shown as item (item.key)}
					<li class="relative flex gap-6 pb-8 last:pb-0">
						<!-- spine -->
						<div class="flex flex-col items-center" aria-hidden="true">
							<div class="mt-1 h-3 w-3 shrink-0 rounded-full border-2 border-pqs-apricot bg-pqs-smoke dark:bg-pqs-midnight"></div>
							<div class="mt-1 w-px flex-1 bg-pqs-ashgray dark:bg-pqs-steel/40"></div>
						</div>
						<!-- content -->
						<div class="min-w-0 pb-2">
							<div class="flex flex-wrap items-center gap-2">
								<time datetime={item.entry.date} class="font-mono text-xs text-pqs-steel dark:text-pqs-bluegray">{item.entry.date}</time>
								<span class="rounded px-1.5 py-0.5 font-heading text-xs font-semibold {TYPE_BADGE[item.entry.type]}">
									{HISTORY_TYPE_LABELS[item.entry.type]}
								</span>
								<span class="sr-only">Programmes:</span>
								<ProgrammeChips chips={entryChips(item.programmes, data.programmeOptions)} />
							</div>
							<p class="mt-1 text-sm text-pqs-midnight dark:text-pqs-smoke [&_a]:text-pqs-steel [&_a]:underline dark:[&_a]:text-pqs-apricot">
								{@html item.entry.description}
							</p>
							{#if item.entry.schemes && item.entry.schemes.length}
								{@const names = item.entry.schemes}
								<p class="mt-1 text-xs text-pqs-steel dark:text-pqs-bluegray">
									<span class="sr-only">Schemes:</span>
									{#each names as name, i (name)}
										{@const href = schemeHref(name, base)}
										{#if href}<a {href} class="underline hover:text-pqs-apricot">{name}</a>{:else}{name}{/if}{i < names.length - 1 ? ', ' : ''}
									{/each}
								</p>
							{/if}
						</div>
					</li>
				{/each}
			</ol>
		</div>
	</div>
</div>
```

Notes for the implementer:
- Description links move from `text-pqs-apricot` (about 2.4:1 on the light background) to `text-pqs-steel` underlined in light mode, matching the home page's "Latest changes". Apricot stays in dark mode.
- The spine is `aria-hidden`. The old `last:hidden` on an inner `div` never matched (that `div` is always the last child), so it is dropped.
- Keep the `<ol>` rendered when it is empty, so `aria-label="Events"` always resolves.

- [ ] **Step 6: Run checks and the e2e tests**

Run: `npm run check && npm run test && npx playwright test e2e/timeline.spec.ts e2e/redirects.spec.ts e2e/home.spec.ts`
Expected: 0 errors; unit tests pass; all timeline tests pass. `/history/` still forwards to `/timeline/`, and the home page's "Latest changes" is unchanged.

- [ ] **Step 7: Check both themes by eye**

Run `npm run dev` and open `http://localhost:5173/timeline/` in light and dark mode. Check:
- the filter labels, count, chips and description links are readable;
- the NGCC chip (once plan 3 adds entries) uses the "new" tone;
- on a phone width (375 px) the filter column sits above the list, with no horizontal scroll.

- [ ] **Step 8: Commit**

```bash
git add src/lib/historyData.ts src/routes/+page.ts src/routes/timeline/+page.ts src/routes/timeline/+page.svelte e2e/timeline.spec.ts
git commit -m "Filter the timeline by programme and event type, with programme chips

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 5: Carry-over polish from plans 1–2

Each item in `.superpowers/sdd/carryover.md` was checked against the code at `a3e31c4`.

**Included** (still present):

| # | Carry-over item | Evidence | Fix |
|---|---|---|---|
| A | Plan 2 T10: omitted-rows note uses hard-coded purple hex and is duplicated | `ScatterPlot.svelte:227`. `KemScatterPlot.svelte` imports `omittedNote` and sets `omitted` but **never renders the note** (a regression: the KEM Speed view will silently drop unbenchmarked NGCC KEMs once plan 3 lands) | `OmittedNote.svelte` with `pqs-violet*` tokens, used by both plots |
| B | Plan 2 T10: omitted count set before the stale-render check | `doRender` sets `omitted` after `await import('vega-embed')` without re-checking `seq` | Re-check `seq !== renderSeq` after the await (both plots) |
| C | Plan 2 T10: omitted note / Speed view has no e2e | none in `e2e/` | e2e on `/signatures/?r=1&v=speed&ps=all` (EagleSign has no timings) |
| D | Plan 1 "← Back" dim in dark mode; plan 2 T11 dark-mode leftovers | both advanced pages: `text-pqs-bluegray … dark:text-pqs-steel` on the back link, X/Y-axis labels and inactive scale buttons (#064058 on #061128 ≈ 1.2:1; light mode #C5CADA on white ≈ 1.6:1) | Swap to `text-pqs-steel dark:text-pqs-bluegray` |
| E | Plan 2 T4: tables duplicate the µs-or-cycles rule now in `timing.ts` | `SchemeTable.svelte`/`KemTable.svelte` `timeCell` compute `cyc / CYCLES_PER_US` | Use `displayUs()` |
| F | Plan 2 T7: double `getFilterStore()` in the signatures advanced page | `signatures/advanced/+page.svelte:30-31` and a shadowing re-declaration in `onMount` | One destructure |
| G | Plan 2 T1: codec test lacks `hi`/`hw` unknown values and an `hb=0` override | `compareFilters.test.ts` only tests `hb=yes` | Add the test |
| H | Plan 1 T1: no check that a programme's `primitives` covers the scheme's primitive | `dataFiles.test.ts` | Add the check (data passes today) |
| I | Plan 1 T8: history glob duplicated between home and timeline | done in Task 4 (`historyData.ts`) | none |

**Skipped**:
- Obsolete or already fixed:
  - fixed sleeps / `waitForTimeout` in e2e (none left after `1c96419`);
  - "N/A" pre-quantum level (none left in `src/`);
  - chart caption / "Reset zoom" / "Advanced graph →" contrast (now `dark:text-pqs-bluegray`);
  - CLAUDE.md test lists, tree, `/kems/advanced/` and `includes(filter.stage)` (fixed in `7e9ab13`);
  - `kemData.ts` "no round tags" comment (gone);
  - History nav bounce and stale e2e titles (nav links straight to `timeline/`, and the titles are gone);
  - home on-ramp row listing all 40 schemes (`summariseGroups` collapses ended rounds);
  - organiser repeated in the subtitle (guarded);
  - scheme-tree "Pre-Quantum" heading (rendered as "Pre-quantum");
  - scatter colour domains without NGCC (plan 2 added the NGCC scale).
- Owned by a parallel plan:
  - layout `startsWith` repeated, and no mobile-nav or `BASE_PATH` e2e (plan 4, nav);
  - weak `getByText('NIST')` home locator (plan 4, home);
  - the `programmes.test.ts` summary test placement and `every()` (plan 4 edits `programmes.ts`);
  - SMAUG-T/NTRU+ "KpqC selected" vs status "KpqC standard" (data label, plan 4 programme pages).
- Plan-mandated or low value:
  - `programmeData.ts` `?? []` (dataFiles already fails on a missing registry);
  - `dataFiles` relying on `process.cwd()`;
  - the non-idempotent one-off `migrate_programmes.py`;
  - report evidence and redundant prerender;
  - the `SIGNATURE_PARAMS` guard and the cosmetic `legacyHomeRedirect` cast (old-URL compatibility is low priority);
  - `l=1.0` being accepted (harmless: it means level 1);
  - `data.ts` inline ternary / `schemeProgrammes` computed twice, and the hard-coded round options (refactor of a file plan 5 is likely to touch);
  - `kemData.test.ts` `JSON.stringify` equality;
  - the KEM badge toggle e2e (shared component already covered on signatures);
  - 'No level' legend rows and the legend/scale colour-equality test (unreachable per the dataFiles level test);
  - the nested level ternary and duplicated `levelOrder` (pre-existing);
  - separate cycles/µs sections and the double `getFilterStore()` in the signatures page subscriber (intentional: defaults are read at sync time).

**Files:**
- Create: `src/lib/components/OmittedNote.svelte`
- Modify: `src/routes/layout.css`, `src/lib/components/ScatterPlot.svelte`, `src/lib/components/KemScatterPlot.svelte`, `src/lib/components/SchemeTable.svelte`, `src/lib/components/KemTable.svelte`, `src/routes/signatures/advanced/+page.svelte`, `src/routes/kems/advanced/+page.svelte`, `src/lib/__tests__/compareFilters.test.ts`, `src/lib/__tests__/dataFiles.test.ts`, `e2e/signatures.spec.ts`

**Interfaces:**
- Consumes: `omittedNote(n: number): string` (`$lib/chartData`), `displayUs(us: number | null, cycles: number): number | null` (`$lib/timing`), `decodeCommonFilters(defaults, params)` (`$lib/compareFilters`).
- Produces: `OmittedNote.svelte` with props `{ count: number }`; tokens `--color-pqs-violet`, `-pale`, `-ink`, `-deep`, `-mist`.

Commit 5a (items A–C): the omitted-rows note.

- [ ] **Step 1: Write the failing e2e test**

In `e2e/signatures.spec.ts`, directly after the test `'Speed toggle switches the chart to signing vs. verification time'`, add:

```ts
	test('Speed view notes parameter sets left out because they are not benchmarked', async ({ page }) => {
		// EagleSign (on-ramp round 1) has sizes but no timings; ps=all so its sets are not hidden as non-recommended.
		await page.goto('/signatures/?r=1&v=speed&ps=all');
		await page.waitForFunction(() => [...document.querySelectorAll('svg')].some((s) => s.querySelector('g')), { timeout: 15_000 });
		await expect(page.getByRole('note').filter({ hasText: 'not yet benchmarked' })).toBeVisible();
	});
```

- [ ] **Step 2: Run it**

Run: `npx playwright test e2e/signatures.spec.ts -g "not benchmarked"`
Expected: PASS on the signatures page, because the note exists there today. This test pins the behaviour before the refactor. The KEM plot cannot be covered by e2e yet (every KEM is benchmarked). After plan 3, NGCC KEMs exercise the same component.

- [ ] **Step 3: Add the colour tokens**

In `src/routes/layout.css`, inside `@theme { … }` directly after `--color-pqs-smoke: #F5F7FA;`, add:

```css
  /* Omitted-rows note; violet matches the NGCC level scale in chartLegend.ts */
  --color-pqs-violet: #A855F7;
  --color-pqs-violet-pale: #faf5ff;
  --color-pqs-violet-ink: #4c1d95;
  --color-pqs-violet-deep: #2e1065;
  --color-pqs-violet-mist: #e9d5ff;
```

- [ ] **Step 4: Create the component**

Create `src/lib/components/OmittedNote.svelte`:

```svelte
<script lang="ts">
	import { omittedNote } from '$lib/chartData';

	let { count }: { count: number } = $props();
</script>

{#if count > 0}
	<p
		role="note"
		class="mt-2 rounded border border-dashed border-pqs-violet bg-pqs-violet-pale px-2.5 py-1.5 text-xs text-pqs-violet-ink dark:bg-pqs-violet-deep/40 dark:text-pqs-violet-mist"
	>
		{omittedNote(count)} They will appear once they are added to our benchmark harness.
	</p>
{/if}
```

- [ ] **Step 5: Use it in both plots and drop stale renders**

In `src/lib/components/ScatterPlot.svelte`:
1. In the imports, change `import { omittedNote, partitionPlottable, sigAxisValue, timeTooltip } from '$lib/chartData';` to `import { partitionPlottable, sigAxisValue, timeTooltip } from '$lib/chartData';` and add `import OmittedNote from './OmittedNote.svelte';` after `import ChartLegend from './ChartLegend.svelte';`.
2. Replace

```svelte
		{#if omitted > 0}
			<p role="note" class="mt-2 rounded border border-dashed border-[#A855F7] bg-[#faf5ff] px-2.5 py-1.5 text-xs text-[#4c1d95] dark:bg-[#2e1065]/40 dark:text-[#e9d5ff]">
				{omittedNote(omitted)} They will appear once they are added to our benchmark harness.
			</p>
		{/if}
```

with

```svelte
		<OmittedNote count={omitted} />
```

3. Replace

```ts
		const { default: embed } = await import('vega-embed');
		const { plotted: data, omitted: missing } = partitionPlottable($filteredRows, sigAxisValue, xField, yField);
```

with

```ts
		const { default: embed } = await import('vega-embed');
		// A newer render was queued while the import resolved: let that one set the state.
		if (seq !== renderSeq) return;
		const { plotted: data, omitted: missing } = partitionPlottable($filteredRows, sigAxisValue, xField, yField);
```

In `src/lib/components/KemScatterPlot.svelte`:
1. Change `import { kemAxisValue, omittedNote, partitionPlottable, timeTooltip } from '$lib/chartData';` to `import { kemAxisValue, partitionPlottable, timeTooltip } from '$lib/chartData';` and add `import OmittedNote from './OmittedNote.svelte';` after `import ChartLegend from './ChartLegend.svelte';`.
2. Replace

```svelte
		{#if renderError}
			<p class="mt-1 text-xs text-pqs-scarlet" role="alert">Chart failed to render.</p>
		{/if}
		<div class="mt-1 flex items-center justify-between">
```

with

```svelte
		{#if renderError}
			<p class="mt-1 text-xs text-pqs-scarlet" role="alert">Chart failed to render.</p>
		{/if}
		<OmittedNote count={omitted} />
		<div class="mt-1 flex items-center justify-between">
```

3. Replace

```ts
		const { default: embed } = await import('vega-embed');
		const { plotted: data, omitted: missing } = partitionPlottable(rows, kemAxisValue, xField, yField);
```

with

```ts
		const { default: embed } = await import('vega-embed');
		// A newer render was queued while the import resolved: let that one set the state.
		if (seq !== renderSeq) return;
		const { plotted: data, omitted: missing } = partitionPlottable(rows, kemAxisValue, xField, yField);
```

- [ ] **Step 6: Verify and commit 5a**

Run: `npm run check && npm run test && npx playwright test e2e/signatures.spec.ts e2e/kems.spec.ts`
Expected: 0 errors; all pass, including the new note test.

```bash
git add src/routes/layout.css src/lib/components/OmittedNote.svelte src/lib/components/ScatterPlot.svelte src/lib/components/KemScatterPlot.svelte e2e/signatures.spec.ts
git commit -m "Show the not-benchmarked note on the KEM chart too, from one component

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

Commit 5b (item D): advanced-page contrast.

- [ ] **Step 7: Fix the colours**

Run, from the repo root:

```bash
perl -i -pe 's/font-normal text-pqs-bluegray/font-normal text-pqs-steel dark:text-pqs-bluegray/; s/(?<![:\w-])text-pqs-bluegray(?=[^"]*dark:text-pqs-steel(?![-\w]))/text-pqs-steel/g; s/dark:text-pqs-steel(?![-\w])/dark:text-pqs-bluegray/g' src/routes/signatures/advanced/+page.svelte src/routes/kems/advanced/+page.svelte
```

Then verify that no unprefixed `text-pqs-bluegray` and no `dark:text-pqs-steel` remain on these pages:

```bash
grep -nP '(?<![:\w-])text-pqs-bluegray|dark:text-pqs-steel(?![-\w])' src/routes/signatures/advanced/+page.svelte src/routes/kems/advanced/+page.svelte
```

Expected: no output. `git diff` shows 8 changed lines per file: the back link, the "(log–log scale)" span, two axis labels and four inactive scale buttons. Each now reads `text-pqs-steel … dark:text-pqs-bluegray` (the scale buttons keep `dark:bg-pqs-midnight-mid`).

- [ ] **Step 8: Verify by eye and commit 5b**

Run `npm run dev`, then open `/signatures/advanced/` and `/kems/advanced/` in both themes. "← Back", the "X axis"/"Y axis" labels and the unselected Log/Linear buttons should be clearly readable.

Run: `npm run check && npx playwright test e2e/advanced.spec.ts`
Expected: 0 errors; pass.

```bash
git add src/routes/signatures/advanced/+page.svelte src/routes/kems/advanced/+page.svelte
git commit -m "Fix contrast of back link, axis labels and scale buttons on advanced pages

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

Commit 5c (items E, F): one time rule, one store lookup.

- [ ] **Step 9: Use `displayUs` in both tables**

In `src/lib/components/SchemeTable.svelte` **and** `src/lib/components/KemTable.svelte`:
1. Change `import { CYCLES_PER_US, CPU_GHZ_LABEL } from '$lib/constants';` to `import { CPU_GHZ_LABEL } from '$lib/constants';`.
2. Add `displayUs` to the `$lib/timing` import. `KemTable.svelte` becomes `import { displayUs, kemTimes } from '$lib/timing';`. `SchemeTable.svelte` has no timing import yet, so add `import { displayUs } from '$lib/timing';` after the `$lib/format` import.
3. Replace the `timeCell` snippet

```svelte
{#snippet timeCell(us: number | null, cyc: number)}
	<td class="whitespace-nowrap px-2 py-1.5 text-right tabular-nums">
		{#if us != null}
			{fmtTime(us)}
		{:else if cyc > 0}
			<span
				class="underline decoration-wavy decoration-pqs-scarlet"
				title="Estimated from {fmtCycles(cyc)} cycles @ {CPU_GHZ_LABEL}"
			>{fmtTime(cyc / CYCLES_PER_US)}<span class="sr-only"> (estimated from {fmtCycles(cyc)} cycles @ {CPU_GHZ_LABEL})</span></span>
```

with

```svelte
{#snippet timeCell(us: number | null, cyc: number)}
	{@const shown = displayUs(us, cyc)}
	<td class="whitespace-nowrap px-2 py-1.5 text-right tabular-nums">
		{#if us != null}
			{fmtTime(us)}
		{:else if shown != null}
			<span
				class="underline decoration-wavy decoration-pqs-scarlet"
				title="Estimated from {fmtCycles(cyc)} cycles @ {CPU_GHZ_LABEL}"
			>{fmtTime(shown)}<span class="sr-only"> (estimated from {fmtCycles(cyc)} cycles @ {CPU_GHZ_LABEL})</span></span>
```

(The `{:else}` "not yet benchmarked" branch and `</td>` stay as they are.) If `npm run check` reports `CYCLES_PER_US` still used elsewhere in a file, keep it in that file's import.

- [ ] **Step 10: One `getFilterStore()` in the signatures advanced page**

In `src/routes/signatures/advanced/+page.svelte`, replace

```ts
	const { applyUrl } = getFilterStore();
	const { store } = getFilterStore();
```

with

```ts
	// Stable module-level stores: one lookup serves the whole page.
	const { applyUrl, store } = getFilterStore();
```

and, inside `onMount`, delete the shadowing line `		const { store } = getFilterStore();` that directly precedes `const unsubFilter = store.subscribe(syncUrl);`. Leave the `const { store, defaults } = getFilterStore();` in `syncUrl` as it is: `defaults` must be read at sync time.

- [ ] **Step 11: Verify and commit 5c**

Run: `npm run check && npm run test && npx playwright test e2e/signatures.spec.ts e2e/advanced.spec.ts e2e/kems.spec.ts`
Expected: 0 errors; all pass (the wavy "estimated" cells and the "not yet benchmarked" dashes are unchanged).

```bash
git add src/lib/components/SchemeTable.svelte src/lib/components/KemTable.svelte src/routes/signatures/advanced/+page.svelte
git commit -m "Use the shared displayed-time rule in tables; one filter-store lookup

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

Commit 5d (items G, H): test gaps.

- [ ] **Step 12: Add the codec test**

In `src/lib/__tests__/compareFilters.test.ts`, inside `describe('common URL codec', …)` directly before `it('decodes an empty programme list as "none selected"', …)`, add:

```ts
	it('ignores unknown hide-flag values and lets 0 switch a flag off', () => {
		const on: CommonFilter = { ...ALL, hideBroken: true, hideImplementation: true, hideWarnings: true };
		const s = decodeCommonFilters(on, new URLSearchParams('hb=0&hi=yes&hw=2'));
		expect(s.hideBroken).toBe(false);
		expect(s.hideImplementation).toBe(true);
		expect(s.hideWarnings).toBe(true);
	});
```

- [ ] **Step 13: Add the primitive-coverage check**

In `src/lib/__tests__/dataFiles.test.ts`, inside `describe('programme references in scheme data', …)`, after the test `'every KEM references a known programme and stage'`, add:

```ts
	it('every membership is in a programme that covers that primitive', () => {
		const primitivesOf = new Map(programmes.map((p) => [p.id, p.primitives]));
		const wrong = (file: string, ids: string[], primitive: 'signature' | 'kem') =>
			ids
				.filter((id) => primitivesOf.has(id) && !primitivesOf.get(id)!.includes(primitive))
				.map((id) => `${file}: programme "${id}" does not cover ${primitive}s`);
		const sigErrors = readdirSync(join(DATA, 'schemes'))
			.filter((f) => f.endsWith('.yaml'))
			.flatMap((f) =>
				wrong(f, [...new Set(load<SchemeYaml>(join(DATA, 'schemes', f)).versions.flatMap((v) => Object.keys(v.programmes ?? {})))], 'signature')
			);
		const kemErrors = readdirSync(join(DATA, 'kems'))
			.filter((f) => f.endsWith('.yaml'))
			.flatMap((f) => wrong(f, Object.keys(load<KemSchemeYaml>(join(DATA, 'kems', f)).programmes ?? {}), 'kem'));
		expect([...sigErrors, ...kemErrors]).toEqual([]);
	});
```

- [ ] **Step 14: Verify and commit 5d**

Run: `npx vitest run src/lib/__tests__/compareFilters.test.ts src/lib/__tests__/dataFiles.test.ts && npm run check`
Expected: PASS. Today's data has no on-ramp KEM and no ISO signature. If the coverage test fails, it names the file. Report that to Thom rather than editing the registry.

```bash
git add src/lib/__tests__/compareFilters.test.ts src/lib/__tests__/dataFiles.test.ts
git commit -m "Test hide-flag URL values and programme primitive coverage

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

- [ ] **Step 15: Tick off the carry-over list**

`.superpowers/sdd/carryover.md` is a working note. If it is tracked (`git ls-files .superpowers/sdd/carryover.md` prints it), append one line, `Plan 6 Task 5: fixed A–I (see plan); remaining items skipped with reasons in the plan`, and commit it on its own. If it is untracked, edit it without committing.

---

### Task 6: Update CLAUDE.md

**Files:**
- Modify: `CLAUDE.md`

**Interfaces:**
- Consumes: names from Tasks 1–5.
- Produces: documentation only.

Re-read `CLAUDE.md` first, because plans 3–5 may have edited it. Make only the edits below, adapting the anchor text if a neighbouring line moved.

- [ ] **Step 1: History schema**

Replace the `### History` section body (from "`data/history.yaml` — chronological log …" through the "Types:" line) with:

````markdown
`data/history.yaml` — chronological log of notable events, shown on `/timeline/` (filterable) and
in the home page's "Latest changes". **Update this file whenever scheme data changes** (spec
updates, new attacks, milestone events).

```yaml
- date: '2026-05-26'          # ISO date, quoted (unquoted dates become Date objects and fail the data test)
  type: update                 # update | attack | milestone | standardization
  description: "HTML string. Link tags allowed."
  schemes: [SNOVA]             # optional; signature or KEM `name:`s — each must exist
  programmes: [china-ngcc]     # optional; programme ids from data/programmes.yaml
```

Types: `update` (spec/data change), `attack` (new cryptanalysis result), `milestone` (programme
process event), `standardization` (a standard published; shown as "Standardisation").

An entry's programmes are the union of its `schemes:`' memberships (any version) and its explicit
`programmes:`. Use `programmes:` for events with no scheme (e.g. an NGCC round announcement).
Entries with neither are "Site updates". `dataFiles.test.ts` checks dates, types, scheme names and
programme ids.

Timeline URL params: `p` (programme ids, plus `site`) and `t` (types), comma-separated. A missing
param means all, `p=` means none, and unknown values are ignored. Logic is in `src/lib/timeline.ts`
(pure, unit-tested). The page keeps the filter in a `$state` rune and syncs it with
`goto(…, { replaceState: true })`.
````

- [ ] **Step 2: Module tree, routes and test lists**

In the architecture tree under `src/lib/`, after the `programmeData.ts` line, add:

```
│   ├── historyData.ts    # history YAML loader → allHistory (home + timeline)
│   ├── timeline.ts       # timeline: scheme→programme index, entry programmes, filter, URL codec (p, t)
│   ├── schemeLinks.ts    # schemeSlug(), schemeHref() — null until SCHEME_PAGES (plan 5) is on
```

Under `components/`, after the `ChartModeToggle.svelte` line, add:

```
│       ├── OmittedNote.svelte    # "N parameter sets are not shown: not yet benchmarked" under both scatter plots
```

Replace `    ├── timeline/         # history panel (was /history/)` with:

```
    ├── timeline/         # /timeline/: events filtered by programme and type (URL p, t), programme chips
```

In the unit-test bullet list, after the `` `html.test.ts` `` line, add:

```
- `timeline.test.ts` — entry programmes, timeline filter and URL codec; `schemeLinks.test.ts` — scheme slugs/links
```

and change the `dataFiles.test.ts` mention to `` `dataFiles.test.ts` — validates `data/**/*.yaml` (incl. history references and programme primitives) ``.

In the E2E bullet list, add:

```
- `timeline.spec.ts` — timeline filters (type, programme), URL round trip, empty state
```

In the tree's test comment, add `schemeLinks, timeline` to the `src/lib/__tests__/` list, and add `├── timeline.spec.ts` under `e2e/`.

- [ ] **Step 3: Verify and commit**

Run: `grep -n "timeline\|historyData\|schemeLinks\|OmittedNote" CLAUDE.md`
Expected: the new lines appear once each. No section is duplicated.

```bash
git add CLAUDE.md
git commit -m "Document timeline filters, history programmes and new modules

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

- [ ] **Step 4: Final full verification**

Run: `npm run check && npm run test && npm run test:e2e`
Expected: 0 errors; all unit and e2e tests pass.

---

## Decisions for Thom to confirm

1. Explicit `programmes:` on a history entry is **added** to the programmes derived from `schemes:`, so it can widen but never narrow. Narrowing would need a separate override field.
2. Entries tied to no programme get a pseudo-option "Site updates" (`p=site`). Without it, they would vanish as soon as any programme box is unticked.
3. The timeline filter uses registry programme ids (NIST PQC is one option), not the compare pages' `nist-standards` / `nist-pqc-rounds` split.
4. Only programmes and event types that occur in the data are offered. `standardization` has no entries today, so it is hidden until one exists.
5. The scheme slug rule (`NTRU+` → `ntru-plus`) and the `SCHEME_PAGES` switch are provisional. Plan 5 owns the final rule.
6. The display label is "Standardisation", while the YAML value stays `standardization`.
