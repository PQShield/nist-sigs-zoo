# Site Reorganization: Compare Pages Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bring `/signatures/` and `/kems/` up to the design canvas. That means a programme filter and Programmes column, NIST and NGCC security-level groups that are never mixed, a Recommended/All parameter-set toggle, and separate "Hide …" status filters. It also means security badges that open a full-width detail row, a Sizes/Speed chart toggle, an HTML chart legend beside the plot, and a KEM page with the same layout as the signatures page.

**Architecture:** All filter logic that both pages share goes in one pure module, `src/lib/compareFilters.ts`. It holds the `CommonFilter` fields (programme groups, NIST levels, NGCC targets, parameter-set mode, hide flags, chart mode), the row predicate, and the URL codec for those fields. The signatures `FilterState` and the KEM `KemFilterState` both extend `CommonFilter`. The signatures store (module-level, stable) and the KEM page (local `$state`) keep their current designs and call the shared functions. Other pure modules shape the data for the components:
- `badges.ts`: security badges and programme chips;
- `chartLegend.ts`: shapes, colour scales and legend groups;
- `chartData.ts`: axis values and leaving out unbenchmarked rows;
- `timing.ts`: displayed µs.

Components only render and stay thin. Small shared filter-section components and a `FilterSidebar` wrapper replace the duplicated mobile/desktop filter panels.

**Tech Stack:** SvelteKit 2 + Svelte 5 runes, adapter-static, Tailwind v4 (`pqs-*` tokens in `src/routes/layout.css`), TypeScript, Vega-Lite (vega-embed), Vitest, Playwright.

**Spec:** The design canvas boards "Compare signatures" and "Compare KEMs", copied as `.superpowers/spec/Compare.dc.html` and `.superpowers/spec/Kems.dc.html`. Use them for layout, wording and behaviour; they are HTML mockups, so don't copy their code. The NGCC rows and points on the canvas are illustrative: no NGCC data exists until plan 3. The plan-1 roadmap entry "Compare pages" in `docs/superpowers/plans/2026-10-07-site-reorganization-foundation.md` lists the scope.

## Global Constraints

- Everything stays on branch `feat/programmes-foundation` (this is a single-branch project). Never switch branches.
- Small, focused commits, one per task. Stage specific files and never use `git add -A` or `git add .`. End each commit message with the trailer on its own line: `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- Never import third-party benchmark numbers. Timings come only from `bench/` and `bench-kem/` (or from submission documents for historic NIST rounds, as today).
- Level scales are never converted. A row has a NIST `level` (1–5 or `Pre-Quantum`) **or** an `ngccLevel` (128/256/384/512 classical bits). NGCC rows never match a NIST level filter, never get a NIST colour, and the table shows them as "128-bit".
- `broken` means a design break. `implementation` (implementation findings) is separate and never implies `broken`.
- Old URLs and existing URL params must keep working: `r`, `s`, `l`, the range keys, `sort`, `dir`, and the KEM keys `pkMin…pcMax`. Add every new signature filter param to `SIGNATURE_PARAMS` in `src/lib/legacy.ts`.
- New URL params (both pages): `p` programme groups, `nl` NGCC targets, `ps=all`, `hb`/`hi`/`hw` (`1`/`0`) for the hide filters, `v=speed`. KEM-only: `kuMin kuMax euMin euMax duMin duMax` (µs).
- Pure logic (filtering, URL codec, grouping, chart data shaping) goes in `src/lib/*.ts` with Vitest tests in `src/lib/__tests__/`. Components stay thin. Unit tests never import `$app/*`, `$lib/schemeData`, `$lib/kemSchemeData` or `$lib/programmeData`; build fixtures from YAML-shaped literals.
- Keep the signatures `filterStore` design (stable module-level stores, `createFilterStore()` resets in place) and the KEM page's local `$state` + pure-function design.
- Accessibility: badges are real `<button>`s with `aria-expanded` and `aria-controls`. Every input has a label, and filter groups are `<fieldset>`s with a `<legend>`. The chart toggle uses `aria-pressed`. Icons need 3:1 contrast against the background in both themes.
- E2E tests must not depend on NGCC data (there is none yet). Unit tests build NGCC-scale rows from fixtures.
- Copy uses British spelling, matching the canvas ("Standardised", "programme").
- Run `npm run check` and `npm run test` before every commit; run `npm run test:e2e` in tasks that touch components or routes.
- Another agent may commit to this branch. Before editing `src/routes/signatures/+page.svelte`, re-read it and keep its URL-sync behaviour: the round applied from the URL first, defaults read at sync time, the hash preserved.

## Review Focus

1. **An old shared link `/signatures/?l=1,2` (from before NGCC targets existed):** it must still show only NIST level 1–2 rows. Once plan 3 lands, NGCC 128-bit rows must not appear just because `nl` is missing. Pinned by Task 1's codec test "reads an old level-only link as no NGCC targets".
2. **Changing the on-ramp round after setting "Hide warnings" or unticking a programme:** the data-independent filters (programmes, levels, parameter sets, status, chart, sort) must survive the round change, and the URL must keep `hw=1` next to `r=2`. Pinned by Task 8's e2e test.
3. **A hand-edited or truncated URL such as `?p=bogus,kpqc&l=1,9,x&nl=100,512&ps=some&hb=yes&v=3d`:** unknown values are ignored and the page renders. `p=` (empty) means "no programme selected", which shows zero rows rather than everything. Pinned by Task 1's codec tests.
4. **The Speed view when no visible row has timings** (for example only unbenchmarked schemes selected): the chart renders empty with the "not yet benchmarked" note, without a Vega error or a legend for points that aren't drawn. Pinned by Task 10's `partitionPlottable` and `chartLegend` empty-input tests.
5. **A row with several flags (broken by design and implementation findings, or warning and note):** each badge has its own button. Clicking one shows that text in the single detail row under the parameter set, and `aria-controls` always names an element that exists. Pinned by Task 5's badge-order tests and Task 6's e2e test that resolves `aria-controls`.

---

## File Structure

| File | Responsibility |
|---|---|
| `src/lib/constants.ts` (modify) | `NGCC_LEVELS` |
| `src/lib/types.ts` (modify) | `ProgrammeGroup`, `ChartMode`, `CommonFilter`; `FilterState` and `KemFilterState` extend `CommonFilter`; KEM µs ranges and filter bounds; speed axis fields |
| `src/lib/compareFilters.ts` (create) | Programme groups of a row, default common filter, row predicate, common URL codec, filter summary |
| `src/lib/timing.ts` (create) | Displayed µs (measured or extrapolated from cycles), time ranges |
| `src/lib/badges.ts` (create) | Security badges per row (pre-quantum, broken, implementation, warning, note); programme chips |
| `src/lib/chartLegend.ts` (create) | Family shapes (SVG paths), NIST and NGCC colour scales, level keys, legend groups |
| `src/lib/chartData.ts` (create) | Axis value per field, splitting rows into plotted and omitted, the omitted-rows note |
| `src/lib/filterStore.ts` (modify) | Uses the common filter and codec; groups from the data |
| `src/lib/kemData.ts` (modify) | Uses the common filter and codec; µs ranges and filters |
| `src/lib/data.ts` (modify) | The stage filter only scopes the filtered programme's entrants |
| `src/lib/legacy.ts` (modify) | New keys in `SIGNATURE_PARAMS` |
| `src/lib/components/FilterSidebar.svelte` (create) | Single filter column: collapsible on mobile, sticky on desktop |
| `src/lib/components/{ProgrammeFilter,LevelFilter,ParamSetFilter,StatusFilter}.svelte` (create) | Shared filter sections (plain props) |
| `src/lib/components/filterStyles.ts` (create) | Shared heading classes for filter sections |
| `src/lib/components/{BadgeIcon,ProgrammeChips,TableLegend}.svelte` (create) | Table pieces shared by both tables |
| `src/lib/components/{ChartLegend,ChartModeToggle}.svelte` (create) | HTML chart legend; Sizes/Speed toggle |
| `src/lib/components/SecurityBadge.svelte` (delete) | Replaced by badges + detail rows |
| `src/lib/components/{FilterPanel,KemFilterPanel,SchemeTable,KemTable,ScatterPlot,KemScatterPlot,RoundSelect}.svelte` (modify) | Use the above |
| `src/routes/signatures/+page.svelte`, `signatures/advanced/+page.svelte`, `kems/+page.svelte`, `kems/advanced/+page.svelte` (modify) | Layout, programme groups, chart toggle |
| `src/lib/__tests__/{compareFilters,timing,badges,chartLegend,chartData}.test.ts` (create) | Unit tests |
| `src/lib/__tests__/{filterStore,kemData,data,legacy,dataFiles}.test.ts` (modify) | Unit tests |
| `e2e/signatures.spec.ts`, `e2e/kems.spec.ts` (modify) | E2E tests |
| `CLAUDE.md` (modify) | Document modules, URL params and inclusion rule |

---

### Task 1: Shared compare-filter logic

**Files:**
- Modify: `src/lib/constants.ts`, `src/lib/types.ts`
- Create: `src/lib/compareFilters.ts`
- Test: `src/lib/__tests__/compareFilters.test.ts`, `src/lib/__tests__/dataFiles.test.ts`

**Interfaces:**
- Consumes: `ProgrammeMembership`, `NistLevel`, `NgccLevel` (types.ts); `STANDARDS_PROGRAMME` (programmes.ts); `ALL_LEVELS` (constants.ts).
- Produces (types.ts):
  ```ts
  export type ProgrammeGroup = 'nist-standards' | 'nist-pqc-rounds' | 'nist-onramp' | 'china-ngcc' | 'kpqc' | 'iso' | 'research' | 'pre-quantum';
  export type ChartMode = 'sizes' | 'speed';
  export interface CommonFilter {
  	programmes: Set<ProgrammeGroup>; levels: Set<NistLevel>; ngccLevels: Set<NgccLevel>;
  	paramSets: 'recommended' | 'all'; hideBroken: boolean; hideImplementation: boolean; hideWarnings: boolean;
  	chart: ChartMode;
  }
  ```
- Produces (constants.ts): `NGCC_LEVELS = [128, 256, 384, 512] as const`.
- Produces (compareFilters.ts):
  - `PROGRAMME_GROUP_ORDER: readonly ProgrammeGroup[]`, `PROGRAMME_GROUP_LABELS: Record<ProgrammeGroup, string>`
  - `interface FilterableRow { programmes; classical; broken; warning; implementation; recommended; level; ngccLevel }`: `ParameterSet` and `KemParameterSet` both satisfy it.
  - `isProgrammeGroup(v: string): v is ProgrammeGroup`
  - `rowProgrammeGroups(row): ProgrammeGroup[]`, `presentProgrammeGroups(rows): ProgrammeGroup[]`
  - `defaultCommonFilter(groups: Iterable<ProgrammeGroup>): CommonFilter`
  - `passesCommonFilters(row: FilterableRow, f: CommonFilter): boolean`
  - `encodeCommonFilters(state: CommonFilter, defaults: CommonFilter, params: URLSearchParams): void`
  - `decodeCommonFilters(state: CommonFilter, params: URLSearchParams): CommonFilter`: returns only the common fields; callers spread it over their full state.
  - `filterSummary(f: CommonFilter, schemeCount: number): string`

Programme groups: `nist-pqc` with stage `selected`/`standardised` → `nist-standards`, any other `nist-pqc` stage → `nist-pqc-rounds`. `nist-onramp`, `china-ngcc`, `kpqc` and `iso` map to themselves. `classical` adds `pre-quantum`, and a row with no group is `research`. A row passes when **any** of its groups is selected.

- [ ] **Step 1: Write the failing tests**

Create `src/lib/__tests__/compareFilters.test.ts`:

```ts
import { describe, expect, it } from 'vitest';
import {
	decodeCommonFilters,
	defaultCommonFilter,
	encodeCommonFilters,
	filterSummary,
	passesCommonFilters,
	presentProgrammeGroups,
	rowProgrammeGroups,
	type FilterableRow
} from '$lib/compareFilters';
import type { CommonFilter, NgccLevel, NistLevel, ProgrammeGroup } from '$lib/types';

function row(overrides: Partial<FilterableRow> = {}): FilterableRow {
	return {
		programmes: [],
		classical: false,
		broken: false,
		warning: false,
		implementation: false,
		recommended: true,
		level: 1,
		ngccLevel: null,
		...overrides
	};
}

const GROUPS: ProgrammeGroup[] = ['nist-standards', 'nist-onramp', 'china-ngcc', 'kpqc', 'research', 'pre-quantum'];
const ALL = defaultCommonFilter(GROUPS);

describe('rowProgrammeGroups', () => {
	it('splits NIST PQC into standards and earlier rounds', () => {
		expect(rowProgrammeGroups(row({ programmes: [{ programme: 'nist-pqc', stage: 'standardised' }] }))).toEqual(['nist-standards']);
		expect(rowProgrammeGroups(row({ programmes: [{ programme: 'nist-pqc', stage: 'selected' }] }))).toEqual(['nist-standards']);
		expect(rowProgrammeGroups(row({ programmes: [{ programme: 'nist-pqc', stage: 'round-3' }] }))).toEqual(['nist-pqc-rounds']);
	});

	it('lists every programme a scheme is in, in display order', () => {
		const r = row({ programmes: [{ programme: 'kpqc', stage: 'selected' }, { programme: 'nist-onramp', stage: 'round-1' }] });
		expect(rowProgrammeGroups(r)).toEqual(['nist-onramp', 'kpqc']);
	});

	it('files schemes outside every programme under research, pre-quantum ones under pre-quantum', () => {
		expect(rowProgrammeGroups(row())).toEqual(['research']);
		expect(rowProgrammeGroups(row({ classical: true, broken: 'classical', level: 'Pre-Quantum' }))).toEqual(['pre-quantum']);
	});
});

describe('presentProgrammeGroups', () => {
	it('returns the groups that occur, in display order', () => {
		const rows = [
			row({ classical: true, level: 'Pre-Quantum' }),
			row({ programmes: [{ programme: 'iso', stage: 'in-progress' }] }),
			row({ programmes: [{ programme: 'nist-pqc', stage: 'standardised' }] })
		];
		expect(presentProgrammeGroups(rows)).toEqual(['nist-standards', 'iso', 'pre-quantum']);
	});
});

describe('passesCommonFilters', () => {
	it('keeps a row when any of its programmes is selected', () => {
		const r = row({ programmes: [{ programme: 'nist-onramp', stage: 'round-1' }, { programme: 'kpqc', stage: 'selected' }] });
		expect(passesCommonFilters(r, { ...ALL, programmes: new Set<ProgrammeGroup>(['kpqc']) })).toBe(true);
		expect(passesCommonFilters(r, { ...ALL, programmes: new Set<ProgrammeGroup>(['china-ngcc']) })).toBe(false);
	});

	it('matches NGCC targets only against the NGCC group, never a NIST level', () => {
		const ngcc = row({ level: null, ngccLevel: 128, programmes: [{ programme: 'china-ngcc', stage: 'round-1' }] });
		expect(passesCommonFilters(ngcc, ALL)).toBe(true);
		expect(passesCommonFilters(ngcc, { ...ALL, ngccLevels: new Set() })).toBe(false);
		expect(passesCommonFilters(ngcc, { ...ALL, levels: new Set(), ngccLevels: new Set<NgccLevel>([128]) })).toBe(true);
		expect(passesCommonFilters(row({ level: 1 }), { ...ALL, levels: new Set(), ngccLevels: new Set<NgccLevel>([128]) })).toBe(false);
	});

	it('drops a row that states no level on either scale', () => {
		expect(passesCommonFilters(row({ level: null, ngccLevel: null }), ALL)).toBe(false);
	});

	it('shows only recommended parameter sets by default, all on request', () => {
		const extra = row({ recommended: false });
		expect(passesCommonFilters(extra, ALL)).toBe(false);
		expect(passesCommonFilters(extra, { ...ALL, paramSets: 'all' })).toBe(true);
	});

	it('hides design breaks but never pre-quantum schemes with "Hide broken by design"', () => {
		expect(passesCommonFilters(row({ broken: 'key recovery' }), { ...ALL, hideBroken: true })).toBe(false);
		const ecdsa = row({ broken: 'classical', classical: true, level: 'Pre-Quantum' });
		expect(passesCommonFilters(ecdsa, { ...ALL, hideBroken: true })).toBe(true);
	});

	it('hides implementation findings separately from design breaks', () => {
		const buggy = row({ implementation: 'stack overflow in verify' });
		expect(passesCommonFilters(buggy, { ...ALL, hideBroken: true })).toBe(true);
		expect(passesCommonFilters(buggy, { ...ALL, hideImplementation: true })).toBe(false);
	});

	it('hides warnings', () => {
		expect(passesCommonFilters(row({ warning: 'weaker than claimed' }), { ...ALL, hideWarnings: true })).toBe(false);
	});
});

describe('common URL codec', () => {
	function encode(state: CommonFilter): URLSearchParams {
		const p = new URLSearchParams();
		encodeCommonFilters(state, ALL, p);
		return p;
	}

	it('encodes nothing for the defaults', () => {
		expect(encode(ALL).toString()).toBe('');
	});

	it('round-trips every field', () => {
		const state: CommonFilter = {
			programmes: new Set<ProgrammeGroup>(['kpqc', 'nist-onramp']),
			levels: new Set<NistLevel>([1, 3]),
			ngccLevels: new Set<NgccLevel>([256]),
			paramSets: 'all',
			hideBroken: true,
			hideImplementation: true,
			hideWarnings: true,
			chart: 'speed'
		};
		const p = encode(state);
		expect(p.get('p')).toBe('nist-onramp,kpqc');
		expect(p.get('l')).toBe('1,3');
		expect(p.get('nl')).toBe('256');
		expect(p.get('ps')).toBe('all');
		expect([p.get('hb'), p.get('hi'), p.get('hw')]).toEqual(['1', '1', '1']);
		expect(p.get('v')).toBe('speed');
		expect(decodeCommonFilters(ALL, p)).toEqual(state);
	});

	it('reads an old level-only link as "no NGCC targets"', () => {
		const s = decodeCommonFilters(ALL, new URLSearchParams('l=1,2'));
		expect([...s.levels]).toEqual([1, 2]);
		expect(s.ngccLevels.size).toBe(0);
	});

	it('always sends nl with a changed NIST level list, so the pair round-trips', () => {
		const p = encode({ ...ALL, levels: new Set<NistLevel>([1]) });
		expect(p.get('nl')).toBe('128,256,384,512');
		expect(decodeCommonFilters(ALL, p).ngccLevels.size).toBe(4);
	});

	it('encodes Pre-Quantum as PQ', () => {
		expect(encode({ ...ALL, levels: new Set<NistLevel>(['Pre-Quantum']) }).get('l')).toBe('PQ');
	});

	it('ignores unknown values', () => {
		const s = decodeCommonFilters(ALL, new URLSearchParams('p=bogus,kpqc&l=1,9,x&nl=100,512&ps=some&hb=yes&v=3d'));
		expect([...s.programmes]).toEqual(['kpqc']);
		expect([...s.levels]).toEqual([1]);
		expect([...s.ngccLevels]).toEqual([512]);
		expect(s.paramSets).toBe('recommended');
		expect(s.hideBroken).toBe(false);
		expect(s.chart).toBe('sizes');
	});

	it('decodes an empty programme list as "none selected"', () => {
		expect(decodeCommonFilters(ALL, new URLSearchParams('p=')).programmes.size).toBe(0);
	});

	it('leaves the defaults object unmutated', () => {
		decodeCommonFilters(ALL, new URLSearchParams('p=kpqc&l=1&nl=128'));
		expect(ALL.programmes.size).toBe(GROUPS.length);
		expect(ALL.levels.size).toBe(6);
		expect(ALL.ngccLevels.size).toBe(4);
	});
});

describe('filterSummary', () => {
	it('summarises programmes, schemes and levels', () => {
		expect(filterSummary(ALL, 18)).toBe('6 programmes · 18 schemes · all levels');
		expect(filterSummary({ ...ALL, levels: new Set<NistLevel>([1]), ngccLevels: new Set() }, 1)).toBe('6 programmes · 1 scheme · 1 level');
	});
});
```

Append to `src/lib/__tests__/dataFiles.test.ts`, inside `describe('data/programmes.yaml', …)`, after the "unique stage ids" test. Add `import { isProgrammeGroup } from '$lib/compareFilters';` to the imports at the top.

```ts
	it('maps every programme onto a compare-page filter group', () => {
		// nist-pqc splits into nist-standards / nist-pqc-rounds; every other id is its own group.
		const unmapped = programmes.map((p) => p.id).filter((id) => id !== 'nist-pqc' && !isProgrammeGroup(id));
		expect(unmapped).toEqual([]);
	});
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `npx vitest run src/lib/__tests__/compareFilters.test.ts src/lib/__tests__/dataFiles.test.ts`
Expected: FAIL with "Failed to resolve import "$lib/compareFilters"".

- [ ] **Step 3: Add the constant and the types**

In `src/lib/constants.ts`, after `ALL_LEVELS`:

```ts
/** China NGCC claimed classical security targets, in bits. Never mapped onto NIST levels. */
export const NGCC_LEVELS = [128, 256, 384, 512] as const;
```

Append to `src/lib/types.ts`:

```ts
// Compare-page filters shared by the signatures and KEM pages

/** Programme filter buckets on the compare pages (see rowProgrammeGroups in compareFilters.ts). */
export type ProgrammeGroup =
	| 'nist-standards'
	| 'nist-pqc-rounds'
	| 'nist-onramp'
	| 'china-ngcc'
	| 'kpqc'
	| 'iso'
	| 'research'
	| 'pre-quantum';

export type ChartMode = 'sizes' | 'speed';

/** Data-independent filter fields shared by both compare pages, with one URL codec. */
export interface CommonFilter {
	programmes: Set<ProgrammeGroup>;
	levels: Set<NistLevel>;
	ngccLevels: Set<NgccLevel>;
	paramSets: 'recommended' | 'all';
	hideBroken: boolean;
	hideImplementation: boolean;
	hideWarnings: boolean;
	/** Main compare chart axes. View state, kept here so it shares the URL codec. */
	chart: ChartMode;
}
```

- [ ] **Step 4: Write `src/lib/compareFilters.ts`**

```ts
import { ALL_LEVELS, NGCC_LEVELS } from './constants';
import { STANDARDS_PROGRAMME } from './programmes';
import type { CommonFilter, NgccLevel, NistLevel, ProgrammeGroup, ProgrammeMembership } from './types';

/** Display order of the programme filter. */
export const PROGRAMME_GROUP_ORDER: readonly ProgrammeGroup[] = [
	'nist-standards',
	'nist-pqc-rounds',
	'nist-onramp',
	'china-ngcc',
	'kpqc',
	'iso',
	'research',
	'pre-quantum'
];

export const PROGRAMME_GROUP_LABELS: Record<ProgrammeGroup, string> = {
	'nist-standards': 'NIST standards',
	'nist-pqc-rounds': 'NIST PQC rounds 1–4',
	'nist-onramp': 'NIST on-ramp',
	'china-ngcc': 'China NGCC',
	kpqc: 'KpqC',
	iso: 'ISO/IEC',
	research: 'Research proposals',
	'pre-quantum': 'Pre-quantum reference'
};

/** NIST PQC stages that mean "a NIST standard, or about to be one". */
const STANDARD_STAGES = new Set(['selected', 'standardised']);

/** The row fields the shared filters read. ParameterSet and KemParameterSet both satisfy it. */
export interface FilterableRow {
	programmes: ProgrammeMembership[];
	classical: boolean;
	broken: false | string;
	warning: false | string;
	implementation: false | string;
	recommended: boolean;
	level: NistLevel | null;
	ngccLevel: NgccLevel | null;
}

export function isProgrammeGroup(v: string): v is ProgrammeGroup {
	return (PROGRAMME_GROUP_ORDER as readonly string[]).includes(v);
}

function isNistLevel(v: unknown): v is NistLevel {
	return (ALL_LEVELS as readonly unknown[]).includes(v);
}

function isNgccLevel(v: unknown): v is NgccLevel {
	return (NGCC_LEVELS as readonly unknown[]).includes(v);
}

export function rowProgrammeGroups(row: Pick<FilterableRow, 'programmes' | 'classical'>): ProgrammeGroup[] {
	const groups = new Set<ProgrammeGroup>();
	for (const m of row.programmes) {
		if (m.programme === STANDARDS_PROGRAMME) {
			groups.add(STANDARD_STAGES.has(m.stage) ? 'nist-standards' : 'nist-pqc-rounds');
		} else if (isProgrammeGroup(m.programme)) {
			groups.add(m.programme);
		}
	}
	if (row.classical) groups.add('pre-quantum');
	if (groups.size === 0) groups.add('research');
	return PROGRAMME_GROUP_ORDER.filter((g) => groups.has(g));
}

export function presentProgrammeGroups(rows: Pick<FilterableRow, 'programmes' | 'classical'>[]): ProgrammeGroup[] {
	const present = new Set(rows.flatMap(rowProgrammeGroups));
	return PROGRAMME_GROUP_ORDER.filter((g) => present.has(g));
}

export function defaultCommonFilter(groups: Iterable<ProgrammeGroup>): CommonFilter {
	return {
		programmes: new Set(groups),
		levels: new Set<NistLevel>(ALL_LEVELS),
		ngccLevels: new Set<NgccLevel>(NGCC_LEVELS),
		paramSets: 'recommended',
		hideBroken: false,
		hideImplementation: false,
		hideWarnings: false,
		chart: 'sizes'
	};
}

export function passesCommonFilters(row: FilterableRow, f: CommonFilter): boolean {
	if (!rowProgrammeGroups(row).some((g) => f.programmes.has(g))) return false;
	// Each scale is matched only against its own filter group; the scales are never converted.
	const levelOk =
		(row.level !== null && f.levels.has(row.level)) ||
		(row.ngccLevel !== null && f.ngccLevels.has(row.ngccLevel));
	if (!levelOk) return false;
	if (f.paramSets === 'recommended' && !row.recommended) return false;
	if (f.hideBroken && row.broken !== false && !row.classical) return false;
	if (f.hideImplementation && row.implementation !== false) return false;
	if (f.hideWarnings && row.warning !== false) return false;
	return true;
}

function sameSet<T>(a: Set<T>, b: Set<T>): boolean {
	return a.size === b.size && [...a].every((x) => b.has(x));
}

function list(v: string | null): string[] {
	return v ? v.split(',') : [];
}

const FLAGS = [
	['hb', 'hideBroken'],
	['hi', 'hideImplementation'],
	['hw', 'hideWarnings']
] as const;

/** Add the non-default common filter fields to `params`. */
export function encodeCommonFilters(state: CommonFilter, defaults: CommonFilter, params: URLSearchParams): void {
	if (!sameSet(state.programmes, defaults.programmes)) {
		params.set('p', PROGRAMME_GROUP_ORDER.filter((g) => state.programmes.has(g)).join(','));
	}
	const levelsChanged = !sameSet(state.levels, defaults.levels);
	if (levelsChanged) {
		params.set('l', [...state.levels].map((l) => (l === 'Pre-Quantum' ? 'PQ' : String(l))).join(','));
	}
	// decode reads `l` without `nl` as an old link (no NGCC targets), so `l` always travels with `nl`
	if (levelsChanged || !sameSet(state.ngccLevels, defaults.ngccLevels)) {
		params.set('nl', NGCC_LEVELS.filter((l) => state.ngccLevels.has(l)).join(','));
	}
	if (state.paramSets !== defaults.paramSets) params.set('ps', state.paramSets);
	for (const [key, field] of FLAGS) {
		if (state[field] !== defaults[field]) params.set(key, state[field] ? '1' : '0');
	}
	if (state.chart !== defaults.chart) params.set('v', state.chart);
}

/** Common filter fields from `params`, starting from `state`. Unknown values are ignored. */
export function decodeCommonFilters(state: CommonFilter, params: URLSearchParams): CommonFilter {
	const s: CommonFilter = {
		programmes: new Set(state.programmes),
		levels: new Set(state.levels),
		ngccLevels: new Set(state.ngccLevels),
		paramSets: state.paramSets,
		hideBroken: state.hideBroken,
		hideImplementation: state.hideImplementation,
		hideWarnings: state.hideWarnings,
		chart: state.chart
	};
	if (params.has('p')) s.programmes = new Set(list(params.get('p')).filter(isProgrammeGroup));
	if (params.has('l')) {
		s.levels = new Set(
			list(params.get('l'))
				.map((v) => (v === 'PQ' ? 'Pre-Quantum' : Number(v)))
				.filter(isNistLevel)
		);
		// Links from before NGCC targets existed: their level list was the whole level filter.
		if (!params.has('nl')) s.ngccLevels = new Set();
	}
	if (params.has('nl')) s.ngccLevels = new Set(list(params.get('nl')).map(Number).filter(isNgccLevel));
	const ps = params.get('ps');
	if (ps === 'recommended' || ps === 'all') s.paramSets = ps;
	for (const [key, field] of FLAGS) {
		const v = params.get(key);
		if (v === '1' || v === '0') s[field] = v === '1';
	}
	const v = params.get('v');
	if (v === 'sizes' || v === 'speed') s.chart = v;
	return s;
}

/** One-line summary for the collapsed mobile filter header, e.g. "4 programmes · 18 schemes · all levels". */
export function filterSummary(f: CommonFilter, schemeCount: number): string {
	const plural = (n: number, word: string) => `${n} ${word}${n === 1 ? '' : 's'}`;
	const allLevels = f.levels.size === ALL_LEVELS.length && f.ngccLevels.size === NGCC_LEVELS.length;
	const levels = allLevels ? 'all levels' : plural(f.levels.size + f.ngccLevels.size, 'level');
	return `${plural(f.programmes.size, 'programme')} · ${plural(schemeCount, 'scheme')} · ${levels}`;
}
```

- [ ] **Step 5: Run the tests to verify they pass**

Run: `npx vitest run src/lib/__tests__/compareFilters.test.ts src/lib/__tests__/dataFiles.test.ts && npm run check`
Expected: PASS, and svelte-check reports 0 errors.

- [ ] **Step 6: Commit**

```bash
git add src/lib/constants.ts src/lib/types.ts src/lib/compareFilters.ts src/lib/__tests__/compareFilters.test.ts src/lib/__tests__/dataFiles.test.ts
git commit -m "$(cat <<'EOF'
Add shared compare-page filter logic and URL codec

Programme groups, NIST and NGCC level filters that never mix,
recommended/all parameter sets, separate hide filters and the
chart mode, with one URL codec for both compare pages.

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
EOF
)"
```

---

### Task 2: Signatures filter store on the common filter

**Files:**
- Modify: `src/lib/types.ts` (`FilterState`), `src/lib/filterStore.ts`, `src/lib/legacy.ts`
- Test: `src/lib/__tests__/filterStore.test.ts`, `src/lib/__tests__/legacy.test.ts`

**Interfaces:**
- Consumes: `CommonFilter`, `defaultCommonFilter`, `encodeCommonFilters`, `decodeCommonFilters`, `passesCommonFilters`, `presentProgrammeGroups` (Task 1).
- Produces: `FilterState extends CommonFilter` (its own `levels` field is removed). `createFilterStore(allRows, ranges, urlParams)` keeps its signature; its defaults now select every programme group present in `allRows`. `buildUrlParams` also emits `p nl ps hb hi hw v`. `SIGNATURE_PARAMS` gains `'p', 'nl', 'ps', 'hb', 'hi', 'hw', 'v'`.

- [ ] **Step 1: Write the failing tests**

In `src/lib/__tests__/filterStore.test.ts`, change the imports and `makeDefaults()`:

```ts
import { describe, it, expect } from 'vitest';
import { buildUrlParams, sortRows, createFilterStore } from '$lib/filterStore';
import { get } from 'svelte/store';
import { processYamlSchemes } from '$lib/data';
import { defaultCommonFilter } from '$lib/compareFilters';
import type { FilterState, ParameterSetYaml, SchemeYaml } from '$lib/types';

function makeDefaults(): FilterState {
	return {
		...defaultCommonFilter(['nist-onramp', 'pre-quantum']),
		schemes: new Set(['SchemeA', 'SchemeB']),
		minPk: 100, maxPk: 50000,
		minSig: 200, maxSig: 80000,
		minPkPlusSig: 300, maxPkPlusSig: 130000,
		minSigningCycles: 10000, maxSigningCycles: 5000000,
		minVerificationCycles: 5000, maxVerificationCycles: 1000000,
		minSigningUs: 0, maxSigningUs: Infinity,
		minVerificationUs: 0, maxVerificationUs: Infinity,
		sortCol: 'scheme',
		sortDir: 'asc',
	};
}
```

(`DataRanges` was only imported for types the file no longer uses; drop it from the import if it becomes unused.)

Replace the test `'never match a NIST level, even with every level selected'` with:

```ts
	it('match NGCC targets only, never a NIST level', () => {
		const data: SchemeYaml[] = [
			{ name: 'Nist', website: '', category: 'Lattices', assumption: '', versions: [{ version: 'v1', date: '2024-01-01', status: 'On-ramp', parametersets: [{ name: 'L1', level: 1, pk: 100, sig: 200, signing_cycles: 1000, verification_cycles: 1000 }] }] },
			{ name: 'Ngcc', website: '', category: 'Lattices', assumption: '', versions: [{ version: 'R1', date: '2026-09-20', status: 'NGCC', parametersets: [{ name: 'X-128', ngcc_level: 128, pk: 100, sig: 200, signing_cycles: 1000, verification_cycles: 1000 }] }] }
		];
		const { parameterSets, ranges } = processYamlSchemes(data);
		const { store, filteredRows } = createFilterStore(parameterSets, ranges, new URLSearchParams());
		expect(get(filteredRows).map((r) => r.scheme)).toEqual(['Ngcc', 'Nist']);
		store.update((f) => ({ ...f, ngccLevels: new Set() }));
		expect(get(filteredRows).map((r) => r.scheme)).toEqual(['Nist']);
		store.update((f) => ({ ...f, levels: new Set(), ngccLevels: new Set([128 as const]) }));
		expect(get(filteredRows).map((r) => r.scheme)).toEqual(['Ngcc']);
	});
```

Append a new block at the end of the file:

```ts
describe('programme, parameter-set and status filters', () => {
	const ps = (name: string, extra: Partial<ParameterSetYaml> = {}): ParameterSetYaml => ({
		name, level: 1, pk: 10, sig: 20, signing_cycles: 100, verification_cycles: 100, ...extra
	});
	const DATA: SchemeYaml[] = [
		{ name: 'OnRamp', website: '', category: 'Lattices', assumption: '', versions: [{ version: 'v1', date: '2025-01-01', status: 'On-ramp', programmes: { 'nist-onramp': 'round-2' }, parametersets: [ps('fast', { recommended: true }), ps('small')] }] },
		{ name: 'Broken', website: '', category: 'Lattices', assumption: '', versions: [{ version: 'v1', date: '2025-01-01', status: 'KpqC', programmes: { kpqc: 'round-2' }, broken: 'key recovery', parametersets: [ps('b')] }] },
		{ name: 'Buggy', website: '', category: 'Lattices', assumption: '', versions: [{ version: 'v1', date: '2025-01-01', status: 'KpqC', programmes: { kpqc: 'round-1' }, implementation: 'buffer overflow', parametersets: [ps('x')] }] }
	];
	const { parameterSets, ranges } = processYamlSchemes(DATA);
	const shown = (params: string) =>
		get(createFilterStore(parameterSets, ranges, new URLSearchParams(params)).filteredRows).map((r) => `${r.scheme}/${r.parameterset}`);

	it('shows recommended parameter sets of every programme by default', () => {
		expect(shown('')).toEqual(['Broken/b', 'Buggy/x', 'OnRamp/fast']);
	});

	it('shows every parameter set with ps=all', () => {
		expect(shown('ps=all')).toContain('OnRamp/small');
	});

	it('filters by programme from the URL', () => {
		expect(shown('p=nist-onramp')).toEqual(['OnRamp/fast']);
	});

	it('hides design breaks and implementation findings independently', () => {
		expect(shown('hb=1')).toEqual(['Buggy/x', 'OnRamp/fast']);
		expect(shown('hi=1')).toEqual(['Broken/b', 'OnRamp/fast']);
	});

	it('round-trips the new keys through buildUrlParams', () => {
		const { store, defaults } = createFilterStore(parameterSets, ranges, new URLSearchParams('p=kpqc&ps=all&hw=1&v=speed'));
		const p = buildUrlParams(get(store), defaults);
		expect(p.get('p')).toBe('kpqc');
		expect(p.get('ps')).toBe('all');
		expect(p.get('hw')).toBe('1');
		expect(p.get('v')).toBe('speed');
	});
});
```

Re-read `src/lib/__tests__/legacy.test.ts` first (another agent added the `SIGNATURE_PARAMS` test). Add `import { defaultCommonFilter } from '$lib/compareFilters';` and change the type import to `import type { FilterState, ProgrammeGroup } from '$lib/types';`. Then replace the whole `describe('SIGNATURE_PARAMS', …)` block with:

```ts
describe('SIGNATURE_PARAMS', () => {
	it('covers every key buildUrlParams can emit', () => {
		const defaults: FilterState = {
			...defaultCommonFilter(['nist-onramp', 'kpqc']),
			schemes: new Set(['A', 'B']),
			minPk: 1, maxPk: 10, minSig: 1, maxSig: 10, minPkPlusSig: 1, maxPkPlusSig: 10,
			minSigningCycles: 1, maxSigningCycles: 10, minVerificationCycles: 1, maxVerificationCycles: 10,
			minSigningUs: 0, maxSigningUs: Infinity, minVerificationUs: 0, maxVerificationUs: Infinity,
			sortCol: 'scheme', sortDir: 'asc'
		};
		const state: FilterState = {
			programmes: new Set<ProgrammeGroup>(['kpqc']), levels: new Set([1] as const), ngccLevels: new Set([128] as const),
			paramSets: 'all', hideBroken: true, hideImplementation: true, hideWarnings: true, chart: 'speed',
			schemes: new Set(['A']),
			minPk: 2, maxPk: 9, minSig: 2, maxSig: 9, minPkPlusSig: 2, maxPkPlusSig: 9,
			minSigningCycles: 2, maxSigningCycles: 9, minVerificationCycles: 2, maxVerificationCycles: 9,
			minSigningUs: 2, maxSigningUs: 9, minVerificationUs: 2, maxVerificationUs: 9,
			sortCol: 'pk', sortDir: 'desc'
		};
		const keys = [...buildUrlParams(state, defaults).keys()];
		expect(keys.length).toBe(SIGNATURE_PARAMS.length - 1); // every key except the round param
		for (const k of keys) expect(SIGNATURE_PARAMS as readonly string[]).toContain(k);
	});

	it('forwards an old home link that carries a new filter key', () => {
		expect(legacyHomeRedirect('?p=kpqc', '', '')).toBe('/signatures/?p=kpqc');
	});
});
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `npx vitest run src/lib/__tests__/filterStore.test.ts src/lib/__tests__/legacy.test.ts`
Expected: FAIL. The programme/parameter-set tests fail because `createFilterStore` ignores `p`, `ps`, `hb`, `hi` (for example `'OnRamp/small'` shows by default and `p=nist-onramp` does not narrow). The legacy test fails because `keys.length` is 25 while `SIGNATURE_PARAMS.length - 1` is 18.

- [ ] **Step 3: Make `FilterState` extend `CommonFilter`**

In `src/lib/types.ts`, change the start of `FilterState` from

```ts
export interface FilterState {
	schemes: Set<string>;
	levels: Set<NistLevel>;
	minPk: number;
```

to

```ts
export interface FilterState extends CommonFilter {
	schemes: Set<string>;
	minPk: number;
```

- [ ] **Step 4: Use the common filter in `src/lib/filterStore.ts`**

Replace the import lines at the top with:

```ts
import { derived, writable } from 'svelte/store';
import type { Readable, Writable } from 'svelte/store';
import type { DataRanges, FilterState, ParameterSet, ProgrammeGroup, SortableColumn } from './types';
import { CYCLES_PER_US } from './constants';
import {
	decodeCommonFilters,
	defaultCommonFilter,
	encodeCommonFilters,
	passesCommonFilters,
	presentProgrammeGroups
} from './compareFilters';
```

Replace `defaultFilter` with:

```ts
function defaultFilter(ranges: DataRanges, allSchemes: Set<string>, groups: ProgrammeGroup[]): FilterState {
	return {
		...defaultCommonFilter(groups),
		schemes: new Set(allSchemes),
		minPk: ranges.pk[0],
		maxPk: ranges.pk[1],
		minSig: ranges.sig[0],
		maxSig: ranges.sig[1],
		minPkPlusSig: ranges.pkPlusSig[0],
		maxPkPlusSig: ranges.pkPlusSig[1],
		minSigningCycles: ranges.signingCycles[0],
		maxSigningCycles: ranges.signingCycles[1],
		minVerificationCycles: ranges.verificationCycles[0],
		maxVerificationCycles: ranges.verificationCycles[1],
		minSigningUs: ranges.signingUs?.[0] ?? 0,
		maxSigningUs: ranges.signingUs?.[1] ?? Infinity,
		minVerificationUs: ranges.verificationUs?.[0] ?? 0,
		maxVerificationUs: ranges.verificationUs?.[1] ?? Infinity,
		sortCol: DEFAULT_SORT_COL,
		sortDir: DEFAULT_SORT_DIR,
	};
}
```

In `applyUrlParams`, delete the whole `const levelsParam = params.get('l'); if (levelsParam) { … }` block, and replace the final `return s;` with:

```ts
	return { ...s, ...decodeCommonFilters(s, params) };
```

In `buildUrlParams`, replace the whole `const allLevels = defaults.levels; if ( … ) { params.set('l', …); }` block with:

```ts
	encodeCommonFilters(state, defaults, params);
```

In `createFilterStore`, change

```ts
	const defaults = defaultFilter(ranges, allSchemes);
```

to

```ts
	const defaults = defaultFilter(ranges, allSchemes, presentProgrammeGroups(allRows));
```

and in the `derived` filter replace

```ts
					if (row.level === null || !$state.levels.has(row.level)) return false;
```

with

```ts
					if (!passesCommonFilters(row, $state)) return false;
```

- [ ] **Step 5: Add the new keys to `SIGNATURE_PARAMS`**

In `src/lib/legacy.ts`, replace the constant with:

```ts
/** Query keys of the signature page's URL codec (filterStore.ts + compareFilters.ts) and round. */
export const SIGNATURE_PARAMS = [
	'r', 's', 'l', 'nl', 'p', 'ps', 'hb', 'hi', 'hw', 'v',
	'pkMin', 'pkMax', 'sigMin', 'sigMax', 'psMin', 'psMax',
	'scMin', 'scMax', 'vcMin', 'vcMax', 'suMin', 'suMax', 'vuMin', 'vuMax',
	'sort', 'dir'
] as const;
```

- [ ] **Step 6: Run the tests to verify they pass**

Run: `npm run test && npm run check`
Expected: all unit tests PASS; svelte-check 0 errors. (`FilterPanel.svelte` still compiles: `$store.levels` now comes from `CommonFilter`.)

- [ ] **Step 7: Commit**

```bash
git add src/lib/types.ts src/lib/filterStore.ts src/lib/legacy.ts src/lib/__tests__/filterStore.test.ts src/lib/__tests__/legacy.test.ts
git commit -m "$(cat <<'EOF'
Filter signatures by programme, NGCC target, parameter set and status

The signature filter store now uses the shared compare filters and
URL codec; the new keys are listed in SIGNATURE_PARAMS.

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
EOF
)"
```

---

### Task 3: The on-ramp round only scopes on-ramp entrants

The design nests the round select under "NIST on-ramp" in the programme filter, so the round must only decide which **on-ramp** entrants are shown. Schemes in another programme (KpqC, NGCC, the NIST standards) are always shown at their newest version, and so are schemes with no programme (pre-quantum, research). The programme filter, not the round, controls them. On the canvas this is visible as HAETAE (on-ramp round 1, KpqC selected) appearing in the Latest view.

**Files:**
- Modify: `src/lib/data.ts`
- Test: `src/lib/__tests__/data.test.ts`

**Interfaces:**
- Produces: `processYamlSchemes(data, filter, opts)`, same signature. With a filter, a scheme is included when (a) a version is at `filter.stage` in `filter.programme` (unchanged), or (b) the scheme has no memberships, or a membership in some other programme. In case (b) it is shown at its newest version. Schemes whose only memberships are in `filter.programme` and that never reached the stage are still excluded.

- [ ] **Step 1: Write the failing tests**

In `src/lib/__tests__/data.test.ts`, replace the test `'excludes schemes that are only in another competition (KpqC, NGCC)'` with:

```ts
	it('shows schemes from other programmes in every on-ramp round, at their newest version', () => {
		const data = [
			scheme({ name: 'KpqcOnly', versions: [
				{ version: 'v1', date: '2023-01-01', status: 'KpqC', programmes: { kpqc: 'round-1' }, parametersets: [{ ...BASE_PS, pk: 1 }] },
				{ version: 'v2', date: '2024-01-01', status: 'KpqC', programmes: { kpqc: 'selected' }, parametersets: [{ ...BASE_PS, pk: 2 }] }
			] }),
			scheme({ name: 'NgccOnly', versions: [{ version: 'v1', date: '2026-09-20', status: 'NGCC', programmes: { 'china-ngcc': 'round-1' }, parametersets: [BASE_PS] }] })
		];
		const { schemes, parameterSets } = processYamlSchemes(data, ONRAMP_R2);
		expect(schemes.map((s) => s.scheme)).toEqual(['KpqcOnly', 'NgccOnly']);
		expect(parameterSets.find((r) => r.scheme === 'KpqcOnly')!.pk).toBe(2);
	});

	it('keeps an entrant that left the on-ramp but is in another programme (HAETAE in KpqC)', () => {
		const data = [scheme({ name: 'HAETAE', versions: [{ version: 'Round 1', date: '2023-05-31', status: 'On-ramp', programmes: { 'nist-onramp': 'round-1', kpqc: 'selected' }, parametersets: [BASE_PS] }] })];
		expect(processYamlSchemes(data, ONRAMP_R2).schemes.map((s) => s.scheme)).toEqual(['HAETAE']);
	});

	it('still drops on-ramp-only entrants that did not reach the stage, even with an untagged newer version', () => {
		const data = [scheme({ name: 'Dropped', versions: [
			{ version: 'Round 1', date: '2023-05-31', status: 'On-ramp', programmes: { 'nist-onramp': 'round-1' }, parametersets: [BASE_PS] },
			{ version: 'v2', date: '2025-01-01', status: 'On-ramp', parametersets: [BASE_PS] }
		] })];
		expect(processYamlSchemes(data, ONRAMP_R2).schemes).toEqual([]);
	});

	it('shows pre-quantum schemes (no programme) in every round', () => {
		const data = [scheme({ name: 'ECDSA', versions: [{ version: 'FIPS 186-5', date: '2023-02-03', status: 'Classic cryptography', broken: 'classical', parametersets: [{ ...BASE_PS, level: 'Pre-Quantum' as const }] }] })];
		expect(processYamlSchemes(data, ONRAMP_R2).schemes.map((s) => s.scheme)).toEqual(['ECDSA']);
	});
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `npx vitest run src/lib/__tests__/data.test.ts`
Expected: FAIL. "shows schemes from other programmes…" gets `[]`, and "keeps an entrant that left the on-ramp…" gets `[]`.

- [ ] **Step 3: Change the inclusion rule in `src/lib/data.ts`**

Change the import line to:

```ts
import { schemeProgrammes, versionProgrammes } from './programmes';
```

In `processYamlSchemes`, replace this block

```ts
		let latest;
		if (filter) {
			const atStage = sorted.filter(
				(v) => versionProgrammes(v)[filter.programme]?.includes(filter.stage)
			);
			if (atStage.length > 0) {
				latest = useLatestVersion ? sorted[0] : atStage[0];
			} else if (isReferenceScheme(yaml.versions)) {
				latest = sorted[0];
			} else {
				continue; // a competition entrant that was not at this stage
			}
		} else {
			latest = sorted[0];
		}

		const programmes = schemeProgrammes(yaml.versions);
```

with

```ts
		const programmes = schemeProgrammes(yaml.versions);

		let latest;
		if (filter) {
			const atStage = sorted.filter(
				(v) => versionProgrammes(v)[filter.programme]?.includes(filter.stage)
			);
			if (atStage.length > 0) {
				latest = useLatestVersion ? sorted[0] : atStage[0];
			} else if (programmes.length === 0 || programmes.some((m) => m.programme !== filter.programme)) {
				// The stage filter only scopes this programme's entrants. Standards, other
				// programmes and pre-quantum schemes are shown at their newest spec; the
				// programme filter on the page decides whether they are visible.
				latest = sorted[0];
			} else {
				continue; // an entrant of this programme that was not at this stage
			}
		} else {
			latest = sorted[0];
		}
```

(`isReferenceScheme` stays exported from `programmes.ts`; it is still unit-tested there.)

- [ ] **Step 4: Run the tests to verify they pass**

Run: `npm run test && npm run check`
Expected: PASS; 0 errors.

- [ ] **Step 5: Commit**

```bash
git add src/lib/data.ts src/lib/__tests__/data.test.ts
git commit -m "$(cat <<'EOF'
Scope the on-ramp round to on-ramp entrants only

Schemes in other programmes (KpqC, NGCC, standards) and pre-quantum
schemes show their newest spec in every round; the programme filter
decides their visibility.

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
EOF
)"
```

---

### Task 4: KEM filters on the common filter, with µs ranges

**Files:**
- Create: `src/lib/timing.ts`
- Modify: `src/lib/types.ts` (`KemDataRanges`, `KemFilterState`), `src/lib/kemData.ts`, `src/lib/components/KemFilterPanel.svelte`, `src/routes/kems/+page.svelte`, `src/routes/kems/advanced/+page.svelte`
- Test: `src/lib/__tests__/timing.test.ts`, `src/lib/__tests__/kemData.test.ts`

**Interfaces:**
- Consumes: Task 1 exports.
- Produces (`timing.ts`):
  - `displayUs(us: number | null, cycles: number): number | null`: the measured µs, else cycles at `CYCLES_PER_US`, else null.
  - `sigTimes(r): { sign: number | null; verify: number | null }`
  - `kemTimes(r): { keygen: number | null; encaps: number | null; decaps: number | null }`
  - `timeRange(values: (number | null)[]): [number, number] | null`: min floored and max ceiled to 0.1 µs.
- Produces (types.ts): `KemDataRanges` gains `keygenUs`, `encapsUs` and `decapsUs` (`[number, number] | null`). `KemFilterState extends CommonFilter` (its own `levels` is removed) and gains `minKeygenUs`, `maxKeygenUs`, `minEncapsUs`, `maxEncapsUs`, `minDecapsUs` and `maxDecapsUs`.
- Produces (kemData.ts): `defaultKemFilter(ranges, schemeNames, groups: Iterable<ProgrammeGroup>)`, which takes a new **required** third argument. `buildKemUrlParams` and `applyKemUrlParams` also handle the common keys and `kuMin kuMax euMin euMax duMin duMax`. `filterKemRows` applies the common filters and the µs bounds (rows without a timing pass the µs bounds).
- Produces (KemFilterPanel): new required prop `groups: ProgrammeGroup[]`.

- [ ] **Step 1: Write the failing tests**

Create `src/lib/__tests__/timing.test.ts`:

```ts
import { describe, expect, it } from 'vitest';
import { displayUs, kemTimes, sigTimes, timeRange } from '$lib/timing';
import { CYCLES_PER_US } from '$lib/constants';

describe('displayUs', () => {
	it('prefers a measured time', () => {
		expect(displayUs(12.5, 99_999)).toBe(12.5);
	});
	it('extrapolates from cycles at the reference clock', () => {
		expect(displayUs(null, 25_000)).toBe(25_000 / CYCLES_PER_US);
	});
	it('is null when there is no timing at all', () => {
		expect(displayUs(null, 0)).toBeNull();
	});
});

describe('sigTimes / kemTimes', () => {
	it('derive every displayed time of a row', () => {
		expect(sigTimes({ signingUs: null, signingCycles: 2500, verificationUs: 3, verificationCycles: 0 })).toEqual({ sign: 1, verify: 3 });
		expect(kemTimes({ keygenUs: null, keygenCycles: 0, encapsUs: 2, encapsCycles: 0, decapsUs: null, decapsCycles: 5000 })).toEqual({ keygen: null, encaps: 2, decaps: 2 });
	});
});

describe('timeRange', () => {
	it('spans the values, rounded outwards to 0.1 µs', () => {
		expect(timeRange([13.42, null, 8.0064])).toEqual([8, 13.5]);
	});
	it('is null without any timing', () => {
		expect(timeRange([null, null])).toBeNull();
	});
});
```

In `src/lib/__tests__/kemData.test.ts`:

1. Add `import { presentProgrammeGroups } from '$lib/compareFilters';` to the imports.
2. In `describe('KEM filtering', …)` and `describe('KEM URL codec', …)`, every `defaultKemFilter(ranges, …names…)` call gains a third argument `presentProgrammeGroups(parameterSets)`. For example `defaultKemFilter(ranges, schemes.map((s) => s.scheme))` becomes `defaultKemFilter(ranges, schemes.map((s) => s.scheme), presentProgrammeGroups(parameterSets))`, and `defaultKemFilter(ranges, names)` becomes `defaultKemFilter(ranges, names, presentProgrammeGroups(parameterSets))`.
3. Replace `'computeKemRanges handles an empty dataset'` with:

```ts
	it('computeKemRanges handles an empty dataset', () => {
		expect(computeKemRanges([])).toEqual({
			pk: [0, 0], ct: [0, 0], pkPlusCt: [0, 0], keygenUs: null, encapsUs: null, decapsUs: null
		});
	});
```

4. Replace `'never lets an NGCC-scale KEM match a NIST level filter'` with:

```ts
	it('matches an NGCC-scale KEM against the NGCC targets only', () => {
		const ngcc = { ...mlkem, name: 'Aigis-Enc+', parametersets: [{ name: 'Aigis-enc1', ngcc_level: 128 as const, pk: 656, ct: 896 }] };
		const { schemes, parameterSets } = processKemSchemes([mlkem, ngcc]);
		const filter = defaultKemFilter(computeKemRanges(parameterSets), schemes.map((s) => s.scheme), presentProgrammeGroups(parameterSets));
		expect(filterKemRows(parameterSets, filter).map((r) => r.scheme)).toContain('Aigis-Enc+');
		expect(filterKemRows(parameterSets, { ...filter, ngccLevels: new Set() }).map((r) => r.scheme)).not.toContain('Aigis-Enc+');
		expect(filterKemRows(parameterSets, { ...filter, levels: new Set() }).map((r) => r.scheme)).toEqual(['Aigis-Enc+']);
	});
```

5. Append:

```ts
describe('KEM timings', () => {
	const timed: KemSchemeYaml = {
		...mlkem,
		parametersets: [
			{ name: 'ML-KEM-512', level: 1, pk: 800, ct: 768, encaps_us: 10 },
			{ name: 'ML-KEM-768', level: 3, pk: 1184, ct: 1088, encaps_cycles: 50_000 }
		]
	};
	const { schemes, parameterSets } = processKemSchemes([timed, ecdh]);
	const ranges = computeKemRanges(parameterSets);
	const defaults = defaultKemFilter(ranges, schemes.map((s) => s.scheme), presentProgrammeGroups(parameterSets));

	it('computes µs ranges over measured and extrapolated times', () => {
		expect(ranges.encapsUs).toEqual([10, 20]);
		expect(ranges.keygenUs).toBeNull();
		expect([defaults.minEncapsUs, defaults.maxEncapsUs]).toEqual([10, 20]);
		expect([defaults.minKeygenUs, defaults.maxKeygenUs]).toEqual([0, Infinity]);
	});

	it('filters by µs and keeps rows that have not been benchmarked', () => {
		const out = filterKemRows(parameterSets, { ...defaults, maxEncapsUs: 15 });
		expect(out.map((r) => r.parameterset)).toEqual(['ML-KEM-512', 'X25519']);
	});

	it('round-trips µs bounds and common keys through the URL', () => {
		const state = { ...defaults, maxEncapsUs: 15, minDecapsUs: 1, hideWarnings: true, chart: 'speed' as const };
		const p = buildKemUrlParams(state, defaults);
		expect(p.get('euMax')).toBe('15');
		expect(p.get('duMin')).toBe('1');
		expect(p.get('hw')).toBe('1');
		expect(p.get('v')).toBe('speed');
		const back = applyKemUrlParams(defaults, p);
		expect(back.maxEncapsUs).toBe(15);
		expect(back.minDecapsUs).toBe(1);
		expect(back.hideWarnings).toBe(true);
		expect(back.chart).toBe('speed');
	});

	it('shows only recommended parameter sets by default', () => {
		const flagged: KemSchemeYaml = { ...mlkem, parametersets: [{ ...mlkem.parametersets[0], recommended: true }, mlkem.parametersets[1]] };
		const rows = processKemSchemes([flagged]).parameterSets;
		const f = defaultKemFilter(computeKemRanges(rows), ['ML-KEM'], presentProgrammeGroups(rows));
		expect(filterKemRows(rows, f).map((r) => r.parameterset)).toEqual(['ML-KEM-512']);
		expect(filterKemRows(rows, { ...f, paramSets: 'all' })).toHaveLength(2);
	});
});
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `npx vitest run src/lib/__tests__/timing.test.ts src/lib/__tests__/kemData.test.ts`
Expected: FAIL with "Failed to resolve import "$lib/timing"" and, in kemData, missing `encapsUs` ranges.

- [ ] **Step 3: Write `src/lib/timing.ts`**

```ts
import { CYCLES_PER_US } from './constants';

/** Displayed time in µs: the measured value, else extrapolated from cycles, else null ("not yet benchmarked"). */
export function displayUs(us: number | null, cycles: number): number | null {
	if (us != null) return us;
	return cycles > 0 ? cycles / CYCLES_PER_US : null;
}

export function sigTimes(r: {
	signingUs: number | null;
	signingCycles: number;
	verificationUs: number | null;
	verificationCycles: number;
}): { sign: number | null; verify: number | null } {
	return {
		sign: displayUs(r.signingUs, r.signingCycles),
		verify: displayUs(r.verificationUs, r.verificationCycles)
	};
}

export function kemTimes(r: {
	keygenUs: number | null;
	keygenCycles: number;
	encapsUs: number | null;
	encapsCycles: number;
	decapsUs: number | null;
	decapsCycles: number;
}): { keygen: number | null; encaps: number | null; decaps: number | null } {
	return {
		keygen: displayUs(r.keygenUs, r.keygenCycles),
		encaps: displayUs(r.encapsUs, r.encapsCycles),
		decaps: displayUs(r.decapsUs, r.decapsCycles)
	};
}

/** Min/max of the known times, rounded outwards to 0.1 µs so filter inputs show short numbers. */
export function timeRange(values: (number | null)[]): [number, number] | null {
	const known = values.filter((v): v is number => v != null);
	if (known.length === 0) return null;
	return [Math.floor(Math.min(...known) * 10) / 10, Math.ceil(Math.max(...known) * 10) / 10];
}
```

- [ ] **Step 4: Update the KEM types**

In `src/lib/types.ts`, replace `KemDataRanges` and `KemFilterState` with:

```ts
export interface KemDataRanges {
	pk: [number, number];
	ct: [number, number];
	pkPlusCt: [number, number];
	/** Displayed times in µs (extrapolated from cycles where needed); null when no row has one. */
	keygenUs: [number, number] | null;
	encapsUs: [number, number] | null;
	decapsUs: [number, number] | null;
}

export interface KemFilterState extends CommonFilter {
	schemes: Set<string>;
	minPk: number;
	maxPk: number;
	minCt: number;
	maxCt: number;
	minPkPlusCt: number;
	maxPkPlusCt: number;
	minKeygenUs: number;
	maxKeygenUs: number;
	minEncapsUs: number;
	maxEncapsUs: number;
	minDecapsUs: number;
	maxDecapsUs: number;
}
```

- [ ] **Step 5: Update `src/lib/kemData.ts`**

Replace the imports with:

```ts
import type {
	KemDataRanges,
	KemFilterState,
	KemParameterSet,
	KemScheme,
	KemSchemeYaml,
	NistLevel,
	ProgrammeGroup
} from './types';
import { CPUSPEED } from './constants';
import { lastStage } from './programmes';
import { decodeCommonFilters, defaultCommonFilter, encodeCommonFilters, passesCommonFilters } from './compareFilters';
import { kemTimes, timeRange } from './timing';
```

Replace everything from `/** Min/max for each numeric size axis across the given rows. */` to the end of the file with:

```ts
/** Min/max for each size axis, and the displayed-µs range of each operation, across the given rows. */
export function computeKemRanges(rows: KemParameterSet[]): KemDataRanges {
	const span = (values: number[]): [number, number] =>
		values.length === 0 ? [0, 0] : [Math.min(...values), Math.max(...values)];
	const times = rows.map(kemTimes);
	return {
		pk: span(rows.map((r) => r.pk)),
		ct: span(rows.map((r) => r.ct)),
		pkPlusCt: span(rows.map((r) => r.pkPlusCt)),
		keygenUs: timeRange(times.map((t) => t.keygen)),
		encapsUs: timeRange(times.map((t) => t.encaps)),
		decapsUs: timeRange(times.map((t) => t.decaps))
	};
}

/** Default filter: every scheme and programme group, all levels, bounds at the data extremes. */
export function defaultKemFilter(
	ranges: KemDataRanges,
	schemeNames: Iterable<string>,
	groups: Iterable<ProgrammeGroup>
): KemFilterState {
	return {
		...defaultCommonFilter(groups),
		schemes: new Set(schemeNames),
		minPk: ranges.pk[0],
		maxPk: ranges.pk[1],
		minCt: ranges.ct[0],
		maxCt: ranges.ct[1],
		minPkPlusCt: ranges.pkPlusCt[0],
		maxPkPlusCt: ranges.pkPlusCt[1],
		minKeygenUs: ranges.keygenUs?.[0] ?? 0,
		maxKeygenUs: ranges.keygenUs?.[1] ?? Infinity,
		minEncapsUs: ranges.encapsUs?.[0] ?? 0,
		maxEncapsUs: ranges.encapsUs?.[1] ?? Infinity,
		minDecapsUs: ranges.decapsUs?.[0] ?? 0,
		maxDecapsUs: ranges.decapsUs?.[1] ?? Infinity
	};
}

/** Numeric bound fields and their URL keys. */
const BOUNDS = [
	['pkMin', 'minPk'], ['pkMax', 'maxPk'],
	['ctMin', 'minCt'], ['ctMax', 'maxCt'],
	['pcMin', 'minPkPlusCt'], ['pcMax', 'maxPkPlusCt'],
	['kuMin', 'minKeygenUs'], ['kuMax', 'maxKeygenUs'],
	['euMin', 'minEncapsUs'], ['euMax', 'maxEncapsUs'],
	['duMin', 'minDecapsUs'], ['duMax', 'maxDecapsUs']
] as const;

/** Encode non-default filter state as URL query params (shares the common codec with the signatures page). */
export function buildKemUrlParams(state: KemFilterState, defaults: KemFilterState): URLSearchParams {
	const p = new URLSearchParams();

	if (
		state.schemes.size !== defaults.schemes.size ||
		![...state.schemes].every((s) => defaults.schemes.has(s))
	) {
		p.set('s', [...state.schemes].map(encodeURIComponent).join(','));
	}

	encodeCommonFilters(state, defaults, p);

	for (const [key, field] of BOUNDS) {
		if (state[field] !== defaults[field]) p.set(key, String(state[field]));
	}

	return p;
}

/** Decode URL query params onto a copy of the default filter state. */
export function applyKemUrlParams(defaults: KemFilterState, params: URLSearchParams): KemFilterState {
	const s: KemFilterState = { ...defaults, schemes: new Set(defaults.schemes) };

	const schemesParam = params.get('s');
	if (schemesParam) s.schemes = new Set(schemesParam.split(',').map(decodeURIComponent));

	for (const [key, field] of BOUNDS) {
		if (params.has(key)) s[field] = Number(params.get(key));
	}

	return { ...s, ...decodeCommonFilters(s, params) };
}

function outside(v: number | null, min: number, max: number): boolean {
	return v != null && (v < min || v > max);
}

/** Apply scheme, common (programme/level/parameter-set/status), size and µs filters. */
export function filterKemRows(rows: KemParameterSet[], filter: KemFilterState): KemParameterSet[] {
	return rows.filter((row) => {
		if (!filter.schemes.has(row.scheme)) return false;
		if (!passesCommonFilters(row, filter)) return false;
		if (row.pk < filter.minPk || row.pk > filter.maxPk) return false;
		if (row.ct < filter.minCt || row.ct > filter.maxCt) return false;
		if (row.pkPlusCt < filter.minPkPlusCt || row.pkPlusCt > filter.maxPkPlusCt) return false;
		const t = kemTimes(row);
		if (outside(t.keygen, filter.minKeygenUs, filter.maxKeygenUs)) return false;
		if (outside(t.encaps, filter.minEncapsUs, filter.maxEncapsUs)) return false;
		if (outside(t.decaps, filter.minDecapsUs, filter.maxDecapsUs)) return false;
		return true;
	});
}
```

(`NistLevel` is still used by `processKemSchemes`.)

- [ ] **Step 6: Pass programme groups from the KEM pages and panel**

In `src/lib/components/KemFilterPanel.svelte`, change the type import to also import `ProgrammeGroup`, and change `Props` and the props destructuring to:

```ts
	interface Props {
		schemes: KemScheme[];
		categories: string[];
		ranges: KemDataRanges;
		groups: ProgrammeGroup[];
		filter: KemFilterState;
	}
	let { schemes, categories, ranges, groups, filter = $bindable() }: Props = $props();
```

and change `reset()` to:

```ts
	function reset() {
		filter = { ...defaultKemFilter(ranges, allSchemeNames, groups), chart: filter.chart };
	}
```

In both `src/routes/kems/+page.svelte` and `src/routes/kems/advanced/+page.svelte`:
- add `import { presentProgrammeGroups } from '$lib/compareFilters';`;
- after `const categories = …` add `const groups = presentProgrammeGroups(parameterSets);`;
- replace both occurrences of `defaultKemFilter(ranges, schemes.map((s) => s.scheme))` with `defaultKemFilter(ranges, schemes.map((s) => s.scheme), groups)`;
- replace both occurrences of `<KemFilterPanel {schemes} {categories} {ranges} bind:filter />` with `<KemFilterPanel {schemes} {categories} {ranges} {groups} bind:filter />`.

- [ ] **Step 7: Run the tests to verify they pass**

Run: `npm run test && npm run check`
Expected: PASS; 0 errors.

- [ ] **Step 8: Commit**

```bash
git add src/lib/timing.ts src/lib/types.ts src/lib/kemData.ts src/lib/components/KemFilterPanel.svelte src/routes/kems/+page.svelte src/routes/kems/advanced/+page.svelte src/lib/__tests__/timing.test.ts src/lib/__tests__/kemData.test.ts
git commit -m "$(cat <<'EOF'
Filter KEMs by programme, level scale, parameter set, status and µs

The KEM filter uses the shared compare filters and URL codec and
gains keygen/encaps/decaps µs bounds over displayed times.

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
EOF
)"
```

---

### Task 5: Security badges and programme chips (pure)

**Decision: the status chip is folded into the Programmes column.** The canvas drops the Status column. A standard's status says more than its `nist-pqc` membership label ("FIPS 204" against "NIST standard"), so the Programmes cell starts with the status chip (`FIPS 204`, `Std pending`) and leaves out the then-redundant `nist-pqc` membership. Other memberships follow as `membershipLabel` chips. A row with no chip gets "Classic" (pre-quantum) or "Research". Sorting by `status` stays valid in URLs (`sort=status`), even without a header button.

**Files:**
- Create: `src/lib/badges.ts`
- Test: `src/lib/__tests__/badges.test.ts`

**Interfaces:**
- Consumes: `membershipLabel`, `STANDARDS_PROGRAMME` (programmes.ts).
- Produces:
  ```ts
  export type BadgeKind = 'pre-quantum' | 'broken' | 'implementation' | 'warning' | 'note';
  export interface Badge { kind: BadgeKind; title: string; html: string; href: string | null }
  export const BADGE_LEGEND: Record<BadgeKind, string>;
  export function securityBadges(row: { classical: boolean; broken: false | string; implementation: false | string; warning: false | string; info: false | string; findingsUrl: string | null }): Badge[];
  export type ChipTone = 'standard' | 'new' | 'plain';
  export interface Chip { label: string; tone: ChipTone }
  export function programmeChips(row: { status: string; version?: string; programmes: ProgrammeMembership[] }, registry: ProgrammeYaml[]): Chip[];
  ```
  Badge order: pre-quantum **or** broken, then implementation, warning, note. A row may have several badges.

- [ ] **Step 1: Write the failing tests**

Create `src/lib/__tests__/badges.test.ts`:

```ts
import { describe, expect, it } from 'vitest';
import { programmeChips, securityBadges } from '$lib/badges';
import type { ProgrammeYaml } from '$lib/types';

const CLEAN = { classical: false, broken: false as const, implementation: false as const, warning: false as const, info: false as const, findingsUrl: null };

describe('securityBadges', () => {
	it('returns nothing for a row without flags', () => {
		expect(securityBadges(CLEAN)).toEqual([]);
	});

	it('keeps a design break and implementation findings as separate badges, in order', () => {
		const badges = securityBadges({ ...CLEAN, broken: 'forgery', implementation: 'stack write', findingsUrl: 'https://example.org/r.html' });
		expect(badges.map((b) => b.kind)).toEqual(['broken', 'implementation']);
		expect(badges[0]).toMatchObject({ title: 'Broken', html: 'forgery', href: null });
		expect(badges[1]).toMatchObject({ title: 'Implementation findings', html: 'stack write', href: 'https://example.org/r.html' });
	});

	it('links implementation findings only when a report URL exists', () => {
		expect(securityBadges({ ...CLEAN, implementation: 'bug' })[0].href).toBeNull();
	});

	it('shows pre-quantum schemes as pre-quantum, not broken', () => {
		const badges = securityBadges({ ...CLEAN, classical: true, broken: 'classical' });
		expect(badges.map((b) => b.kind)).toEqual(['pre-quantum']);
		expect(badges[0].html).not.toContain('classical<');
	});

	it('lists warning and note after the others', () => {
		expect(securityBadges({ ...CLEAN, warning: 'w', info: 'n' }).map((b) => b.kind)).toEqual(['warning', 'note']);
	});
});

const REGISTRY: ProgrammeYaml[] = [
	{ id: 'nist-pqc', name: 'NIST PQC', short: 'NIST', organiser: 'NIST', region: 'US', primitives: ['signature', 'kem'],
	  stages: [{ id: 'round-3', label: 'round 3' }, { id: 'selected', label: 'selected' }, { id: 'standardised', label: 'standard' }] },
	{ id: 'nist-onramp', name: 'On-ramp', short: 'On-ramp', organiser: 'NIST', region: 'US', primitives: ['signature'],
	  stages: [{ id: 'round-1', label: 'round 1' }, { id: 'round-3', label: 'round 3' }] },
	{ id: 'china-ngcc', name: 'NGCC', short: 'NGCC', organiser: 'ICCS', region: 'CN', primitives: ['signature', 'kem'],
	  stages: [{ id: 'round-1', label: 'round 1' }] },
	{ id: 'kpqc', name: 'KpqC', short: 'KpqC', organiser: 'KpqC', region: 'KR', primitives: ['signature', 'kem'],
	  stages: [{ id: 'selected', label: 'selected' }] },
	{ id: 'iso', name: 'ISO', short: 'ISO', organiser: 'ISO/IEC', region: 'Intl', primitives: ['kem'],
	  stages: [{ id: 'in-progress', label: 'in progress' }] }
];

describe('programmeChips', () => {
	it('shows a FIPS standard by its FIPS number instead of the NIST membership', () => {
		expect(programmeChips({ status: 'FIPS', version: 'FIPS 204', programmes: [{ programme: 'nist-pqc', stage: 'standardised' }] }, REGISTRY))
			.toEqual([{ label: 'FIPS 204', tone: 'standard' }]);
	});

	it('falls back to "FIPS" without a FIPS version (KEMs)', () => {
		expect(programmeChips({ status: 'FIPS', programmes: [{ programme: 'nist-pqc', stage: 'standardised' }] }, REGISTRY))
			.toEqual([{ label: 'FIPS', tone: 'standard' }]);
	});

	it('marks a selected scheme as Std pending', () => {
		expect(programmeChips({ status: 'To be standardized', programmes: [{ programme: 'nist-pqc', stage: 'selected' }] }, REGISTRY))
			.toEqual([{ label: 'Std pending', tone: 'standard' }]);
	});

	it('lists every other programme membership', () => {
		expect(programmeChips({ status: 'On-ramp', programmes: [{ programme: 'nist-onramp', stage: 'round-1' }, { programme: 'kpqc', stage: 'selected' }] }, REGISTRY))
			.toEqual([{ label: 'On-ramp round 1', tone: 'plain' }, { label: 'KpqC selected', tone: 'plain' }]);
		expect(programmeChips({ status: 'ISO standardisation', programmes: [{ programme: 'nist-pqc', stage: 'round-3' }, { programme: 'iso', stage: 'in-progress' }] }, REGISTRY))
			.toEqual([{ label: 'NIST round 3', tone: 'plain' }, { label: 'ISO in progress', tone: 'plain' }]);
	});

	it('highlights NGCC memberships as new', () => {
		expect(programmeChips({ status: 'NGCC', programmes: [{ programme: 'china-ngcc', stage: 'round-1' }] }, REGISTRY))
			.toEqual([{ label: 'NGCC round 1', tone: 'new' }]);
	});

	it('labels schemes outside every programme', () => {
		expect(programmeChips({ status: 'Classic cryptography', programmes: [] }, REGISTRY)).toEqual([{ label: 'Classic', tone: 'plain' }]);
		expect(programmeChips({ status: 'Academic', programmes: [] }, REGISTRY)).toEqual([{ label: 'Research', tone: 'plain' }]);
	});
});
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `npx vitest run src/lib/__tests__/badges.test.ts`
Expected: FAIL with "Failed to resolve import "$lib/badges"".

- [ ] **Step 3: Write `src/lib/badges.ts`**

```ts
import { membershipLabel, STANDARDS_PROGRAMME } from './programmes';
import type { ProgrammeMembership, ProgrammeYaml } from './types';

export type BadgeKind = 'pre-quantum' | 'broken' | 'implementation' | 'warning' | 'note';

export interface Badge {
	kind: BadgeKind;
	/** Button name prefix and detail-row heading, e.g. "Broken". */
	title: string;
	/** Detail text: trusted HTML from our own YAML files. */
	html: string;
	/** Full findings report (implementation findings only). */
	href: string | null;
}

/** Legend wording per badge kind. */
export const BADGE_LEGEND: Record<BadgeKind, string> = {
	'pre-quantum': 'pre-quantum',
	broken: 'broken by design',
	implementation: 'implementation findings',
	warning: 'security warning',
	note: 'note'
};

const PRE_QUANTUM_TEXT =
	'Not quantum-resistant: security rests on classical assumptions only. Listed as a reference point.';

export function securityBadges(row: {
	classical: boolean;
	broken: false | string;
	implementation: false | string;
	warning: false | string;
	info: false | string;
	findingsUrl: string | null;
}): Badge[] {
	const badges: Badge[] = [];
	if (row.classical) {
		badges.push({ kind: 'pre-quantum', title: 'Pre-quantum', html: PRE_QUANTUM_TEXT, href: null });
	} else if (row.broken) {
		badges.push({ kind: 'broken', title: 'Broken', html: row.broken, href: null });
	}
	if (row.implementation) {
		badges.push({ kind: 'implementation', title: 'Implementation findings', html: row.implementation, href: row.findingsUrl });
	}
	if (row.warning) badges.push({ kind: 'warning', title: 'Warning', html: row.warning, href: null });
	if (row.info) badges.push({ kind: 'note', title: 'Note', html: row.info, href: null });
	return badges;
}

export type ChipTone = 'standard' | 'new' | 'plain';

export interface Chip {
	label: string;
	tone: ChipTone;
}

/**
 * Programmes-column chips. A standard's status chip (FIPS number / Std pending) replaces its
 * NIST PQC membership; every other membership follows as "<short> <stage>".
 */
export function programmeChips(
	row: { status: string; version?: string; programmes: ProgrammeMembership[] },
	registry: ProgrammeYaml[]
): Chip[] {
	const chips: Chip[] = [];
	let standardShown = false;
	if (row.status === 'FIPS') {
		chips.push({ label: row.version?.startsWith('FIPS') ? row.version : 'FIPS', tone: 'standard' });
		standardShown = true;
	} else if (row.status === 'To be standardized') {
		chips.push({ label: 'Std pending', tone: 'standard' });
		standardShown = true;
	}
	for (const m of row.programmes) {
		if (standardShown && m.programme === STANDARDS_PROGRAMME) continue;
		chips.push({ label: membershipLabel(m, registry), tone: m.programme === 'china-ngcc' ? 'new' : 'plain' });
	}
	if (chips.length === 0) {
		chips.push({ label: row.status === 'Classic cryptography' ? 'Classic' : 'Research', tone: 'plain' });
	}
	return chips;
}
```

- [ ] **Step 4: Run the tests to verify they pass**

Run: `npx vitest run src/lib/__tests__/badges.test.ts && npm run check`
Expected: PASS; 0 errors.

- [ ] **Step 5: Commit**

```bash
git add src/lib/badges.ts src/lib/__tests__/badges.test.ts
git commit -m "$(cat <<'EOF'
Add security badge and programme chip helpers

Implementation findings get their own badge next to broken-by-design;
the Programmes column folds a standard's status into its chip.

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
EOF
)"
```

---

### Task 6: Tables with Programmes column, badge detail rows and unbenchmarked cells

**Files:**
- Create: `src/lib/components/BadgeIcon.svelte`, `src/lib/components/ProgrammeChips.svelte`, `src/lib/components/TableLegend.svelte`
- Modify (rewrite): `src/lib/components/SchemeTable.svelte`, `src/lib/components/KemTable.svelte`
- Modify: `src/routes/kems/+page.svelte`, `src/routes/kems/advanced/+page.svelte`
- Delete: `src/lib/components/SecurityBadge.svelte`
- Test: `e2e/signatures.spec.ts`, `e2e/kems.spec.ts`

**Interfaces:**
- Consumes: `securityBadges`, `programmeChips`, `BADGE_LEGEND`, `BadgeKind`, `Chip` (Task 5); `kemTimes` (Task 4); `allProgrammes` (`$lib/programmeData`).
- Produces: `BadgeIcon { kind: BadgeKind }`, `ProgrammeChips { chips: Chip[] }`, `TableLegend` (no props), and `KemTable { rows: KemParameterSet[]; recommendedOnly: boolean }`. The column order of both tables becomes Scheme, Category, Programmes, Parameter Set, Level, … (Parameter Set stays the 4th column and Level the 5th). The count footer keeps `<span>{n} parameter sets</span>` as its own element.

- [ ] **Step 1: Write the failing e2e tests**

Append inside `test.describe('Signatures page', …)` in `e2e/signatures.spec.ts` (re-read the file first; keep the other agent's `shared links` block):

```ts
	test('table has a Programmes column with membership chips', async ({ page }) => {
		await page.goto('/signatures/');
		await expect(page.getByRole('columnheader', { name: 'Programmes' })).toBeVisible();
		await expect(page.getByRole('columnheader', { name: /^Status/ })).toHaveCount(0);
		await expect(page.locator('tbody')).toContainText('On-ramp round 3');
		await expect(page.locator('tbody')).toContainText('FIPS 204');
	});

	test('a security badge expands a detail row under its parameter set', async ({ page }) => {
		await page.goto('/signatures/?ps=all');
		await page.waitForLoadState('networkidle');
		const badge = page.locator('tbody').getByRole('button', { name: /: show details$/ }).first();
		await expect(badge).toHaveAttribute('aria-expanded', 'false');
		await badge.click();
		await expect(badge).toHaveAttribute('aria-expanded', 'true');
		const id = await badge.getAttribute('aria-controls');
		expect(id).toBeTruthy();
		const detail = page.locator(`[id="${id}"]`);
		await expect(detail).toBeVisible();
		expect(await detail.evaluate((el) => el.tagName)).toBe('TR');
		await badge.click();
		await expect(badge).toHaveAttribute('aria-expanded', 'false');
		await expect(detail).toHaveCount(0);
	});

	test('table legend lists implementation findings separately from broken', async ({ page }) => {
		await page.goto('/signatures/');
		const legend = page.getByText('Select an icon for details').locator('..');
		await expect(legend).toContainText('broken by design');
		await expect(legend).toContainText('implementation findings');
		await expect(legend).toContainText('not yet benchmarked');
	});
```

Append inside `test.describe('KEMs page', …)` in `e2e/kems.spec.ts`:

```ts
	test('table has a Programmes column in place of Status', async ({ page }) => {
		await page.goto('/kems/');
		await expect(page.getByRole('columnheader', { name: 'Programmes' })).toBeVisible();
		await expect(page.locator('tbody')).toContainText('FIPS');
		await expect(page.locator('tbody')).toContainText('KpqC selected');
	});
```

- [ ] **Step 2: Run them to verify they fail**

Run: `npx playwright test e2e/signatures.spec.ts e2e/kems.spec.ts -g "Programmes|security badge|table legend"`
Expected: FAIL (no "Programmes" column header; badges have no "show details" names).

- [ ] **Step 3: Create the shared table pieces**

`src/lib/components/BadgeIcon.svelte`:

```svelte
<script lang="ts">
	import type { BadgeKind } from '$lib/badges';

	let { kind }: { kind: BadgeKind } = $props();
</script>

<svg width="16" height="16" viewBox="0 0 24 24" aria-hidden="true" class="inline-block align-middle">
	{#if kind === 'pre-quantum'}
		<rect x="2" y="2" width="20" height="20" rx="4" fill="#6B7280" />
		<path d="M7 17L17 7" stroke="#ffffff" stroke-width="2.5" stroke-linecap="round" />
	{:else if kind === 'broken'}
		<circle cx="12" cy="12" r="10" class="fill-[#B91C1C] dark:fill-[#EF4444]" />
		<path d="M8 8l8 8M16 8l-8 8" stroke="#ffffff" stroke-width="2.5" stroke-linecap="round" />
	{:else if kind === 'implementation'}
		<rect x="2" y="2" width="20" height="20" rx="5" fill="#C2410C" />
		<text x="12" y="16.5" text-anchor="middle" font-size="12" font-weight="700" fill="#ffffff" font-family="ui-monospace, monospace">{'{}'}</text>
	{:else if kind === 'warning'}
		<path d="M12 2.5L23 21.5H1Z" fill="#B45309" />
		<path d="M12 9v5.5M12 17.5v.5" stroke="#ffffff" stroke-width="2.5" stroke-linecap="round" />
	{:else}
		<circle cx="12" cy="12" r="10" class="fill-[#064058] dark:fill-[#5B9BB8]" />
		<path d="M12 11v6M12 7.5v.5" stroke="#ffffff" stroke-width="2.5" stroke-linecap="round" />
	{/if}
</svg>
```

`src/lib/components/ProgrammeChips.svelte`:

```svelte
<script lang="ts">
	import type { Chip, ChipTone } from '$lib/badges';

	let { chips }: { chips: Chip[] } = $props();

	const TONE: Record<ChipTone, string> = {
		standard: 'bg-[#fbe9d2] text-[#7a4606] dark:bg-pqs-apricot/20 dark:text-pqs-apricot',
		new: 'bg-[#fdf3e6] text-[#8a5208] dark:bg-pqs-apricot/10 dark:text-pqs-apricot-light',
		plain: 'bg-pqs-ashgray text-pqs-midnight dark:bg-pqs-steel/40 dark:text-pqs-smoke'
	};
</script>

<span class="flex flex-wrap gap-1">
	{#each chips as chip (chip.label)}
		<span class="whitespace-nowrap rounded px-1.5 py-px text-[11px] font-bold {TONE[chip.tone]}">{chip.label}</span>
	{/each}
</span>
```

`src/lib/components/TableLegend.svelte`:

```svelte
<script lang="ts">
	import { BADGE_LEGEND, type BadgeKind } from '$lib/badges';
	import { CPU_GHZ_LABEL } from '$lib/constants';
	import BadgeIcon from './BadgeIcon.svelte';

	const KINDS: BadgeKind[] = ['pre-quantum', 'broken', 'implementation', 'warning', 'note'];
</script>

<div class="flex flex-wrap items-center gap-x-4 gap-y-1.5 border-t border-pqs-ashgray bg-pqs-smoke px-3 py-2 text-xs text-pqs-steel/80 dark:border-pqs-steel dark:bg-pqs-midnight-mid dark:text-pqs-bluegray/80">
	<strong class="font-heading text-pqs-steel dark:text-pqs-bluegray">Legend</strong>
	{#each KINDS as kind (kind)}
		<span class="inline-flex items-center gap-1"><BadgeIcon {kind} />{BADGE_LEGEND[kind]}</span>
	{/each}
	<span><span class="underline decoration-wavy decoration-pqs-scarlet">value</span> estimated from cycle counts at {CPU_GHZ_LABEL}</span>
	<span>— not yet benchmarked</span>
	<span>Select an icon for details</span>
</div>
```

- [ ] **Step 4: Rewrite `src/lib/components/SchemeTable.svelte`**

```svelte
<script lang="ts">
	import { programmeChips, securityBadges, type BadgeKind } from '$lib/badges';
	import type { ParameterSet, SortableColumn } from '$lib/types';
	import { getFilterStore } from '$lib/filterStore';
	import { allProgrammes } from '$lib/programmeData';
	import { CYCLES_PER_US, CPU_GHZ_LABEL } from '$lib/constants';
	import { fmt, fmtCycles, fmtTime } from '$lib/format';
	import BadgeIcon from './BadgeIcon.svelte';
	import ProgrammeChips from './ProgrammeChips.svelte';
	import TableLegend from './TableLegend.svelte';

	const { store, filteredRows } = getFilterStore();
	const idPrefix = $props.id();

	/** Sortable columns; the Programmes column (not sortable) is inserted before index 2. */
	const COLUMNS: { key: SortableColumn; label: string; numeric?: boolean }[] = [
		{ key: 'scheme', label: 'Scheme' },
		{ key: 'category', label: 'Category' },
		{ key: 'parameterset', label: 'Parameter Set' },
		{ key: 'level', label: 'Level', numeric: true },
		{ key: 'pk', label: 'pk (B)', numeric: true },
		{ key: 'sig', label: 'sig (B)', numeric: true },
		{ key: 'pkPlusSig', label: 'pk+sig (B)', numeric: true },
		{ key: 'signingUs', label: 'Sign', numeric: true },
		{ key: 'verificationUs', label: 'Verify', numeric: true },
	];
	const COLSPAN = COLUMNS.length + 2; // + Programmes + Assumption

	const DETAIL_TONE: Record<BadgeKind, string> = {
		'pre-quantum': 'bg-pqs-smoke dark:bg-pqs-steel/20',
		broken: 'bg-[#fdf2f2] dark:bg-[#3b1219]',
		implementation: 'bg-[#fff4ec] dark:bg-[#3a1d0c]',
		warning: 'bg-[#fdf6e7] dark:bg-[#33270a]',
		note: 'bg-pqs-smoke dark:bg-pqs-steel/20',
	};

	/** Which badge's detail row is open, per parameter set. */
	let open = $state<Record<string, BadgeKind | undefined>>({});

	function rowKey(row: ParameterSet): string {
		return `${row.scheme}/${row.parameterset}`;
	}

	function toggle(key: string, kind: BadgeKind) {
		open[key] = open[key] === kind ? undefined : kind;
	}

	function setSort(col: SortableColumn) {
		store.update((f) => ({
			...f,
			sortCol: col,
			sortDir: f.sortCol === col && f.sortDir === 'asc' ? 'desc' : 'asc',
		}));
	}

	function sortIndicator(col: SortableColumn): string {
		if ($store.sortCol !== col) return '';
		return $store.sortDir === 'asc' ? ' ▲' : ' ▼';
	}

	function ariaSort(col: SortableColumn): 'ascending' | 'descending' | undefined {
		if ($store.sortCol !== col) return undefined;
		return $store.sortDir === 'asc' ? 'ascending' : 'descending';
	}
</script>

{#snippet timeCell(us: number | null, cyc: number)}
	<td class="px-3 py-1.5 text-right tabular-nums">
		{#if us != null}
			{fmtTime(us)}
		{:else if cyc > 0}
			<span
				class="underline decoration-wavy decoration-pqs-scarlet"
				title="Estimated from {fmtCycles(cyc)} cycles @ {CPU_GHZ_LABEL}"
			>{fmtTime(cyc / CYCLES_PER_US)}<span class="sr-only"> (estimated from {fmtCycles(cyc)} cycles @ {CPU_GHZ_LABEL})</span></span>
		{:else}
			<span class="text-pqs-bluegray" title="Not yet benchmarked">—<span class="sr-only"> not yet benchmarked</span></span>
		{/if}
	</td>
{/snippet}

<div class="overflow-hidden rounded border border-pqs-ashgray shadow-sm dark:border-pqs-steel">
	<div class="overflow-x-auto">
		<table class="min-w-full text-sm">
			<thead class="sticky top-0 z-10">
				<tr class="bg-pqs-steel font-heading text-xs text-white">
					{#each COLUMNS as col, i (col.key)}
						{#if i === 2}
							<th scope="col" class="whitespace-nowrap px-3 py-2.5 text-left font-semibold">Programmes</th>
						{/if}
						<th scope="col" aria-sort={ariaSort(col.key)} class="whitespace-nowrap p-0">
							<button
								type="button"
								onclick={() => setSort(col.key)}
								class="w-full cursor-pointer select-none px-3 py-2.5 font-semibold hover:bg-pqs-steel-light {col.numeric ? 'text-right' : 'text-left'}"
							>
								{col.label}{sortIndicator(col.key)}
							</button>
						</th>
					{/each}
					<th scope="col" class="px-3 py-2.5 text-left text-xs font-semibold">Assumption</th>
				</tr>
			</thead>
			<tbody class="divide-y divide-pqs-ashgray bg-white dark:divide-pqs-steel dark:bg-pqs-midnight-mid">
				{#each $filteredRows as row, i (rowKey(row))}
					{@const key = rowKey(row)}
					{@const badges = securityBadges(row)}
					{@const openBadge = badges.find((b) => b.kind === open[key])}
					{@const detailId = `${idPrefix}-detail-${i}`}
					<tr class="hover:bg-pqs-smoke dark:hover:bg-pqs-steel/30">
						<td class="whitespace-nowrap px-3 py-1.5">
							<a
								href={row.website}
								target="_blank"
								rel="noopener noreferrer"
								class="font-heading font-semibold text-pqs-steel hover:text-pqs-apricot dark:text-pqs-apricot dark:hover:text-pqs-apricot-light"
							>{row.scheme}</a>
							{#each badges as badge (badge.kind)}
								<button
									type="button"
									class="ml-1 inline-grid h-6 w-6 place-items-center rounded align-middle hover:bg-pqs-ashgray focus-visible:outline-2 focus-visible:outline-pqs-apricot dark:hover:bg-pqs-steel/40"
									aria-label="{badge.title}: show details"
									aria-expanded={open[key] === badge.kind}
									aria-controls={open[key] === badge.kind ? detailId : undefined}
									onclick={() => toggle(key, badge.kind)}
								><BadgeIcon kind={badge.kind} /></button>
							{/each}
						</td>
						<td class="whitespace-nowrap px-3 py-1.5 text-pqs-steel dark:text-pqs-bluegray">{row.category}</td>
						<td class="px-3 py-1.5"><ProgrammeChips chips={programmeChips(row, allProgrammes)} /></td>
						<td class="whitespace-nowrap px-3 py-1.5 font-mono text-xs">{row.parameterset}</td>
						<td class="px-3 py-1.5 text-right tabular-nums text-pqs-steel dark:text-pqs-bluegray">
							{#if row.level !== null}
								{row.level === 'Pre-Quantum' ? 'N/A' : row.level}
							{:else if row.ngccLevel !== null}
								<span class="font-semibold text-[#6b21a8] dark:text-[#d8b4fe]" title="China NGCC target in classical bits; not a NIST level">{row.ngccLevel}-bit</span>
							{:else}
								—
							{/if}
						</td>
						<td class="px-3 py-1.5 text-right tabular-nums">{fmt(row.pk)}</td>
						<td class="px-3 py-1.5 text-right tabular-nums">{fmt(row.sig)}</td>
						<td class="px-3 py-1.5 text-right tabular-nums">{fmt(row.pkPlusSig)}</td>
						{@render timeCell(row.signingUs, row.signingCycles)}
						{@render timeCell(row.verificationUs, row.verificationCycles)}
						<td class="px-3 py-1.5 text-pqs-steel/80 dark:text-pqs-bluegray">{row.assumption}</td>
					</tr>
					{#if openBadge}
						<tr id={detailId} class={DETAIL_TONE[openBadge.kind]}>
							<td colspan={COLSPAN} class="whitespace-normal px-3 py-2.5 pl-9 text-sm leading-relaxed text-pqs-midnight dark:text-pqs-smoke [&_a]:text-pqs-apricot [&_a]:underline">
								<strong class="font-heading">{openBadge.title}:</strong>
								{@html openBadge.html}
								{#if openBadge.href}
									<a href={openBadge.href} target="_blank" rel="noopener noreferrer">Full findings report</a>
								{/if}
							</td>
						</tr>
					{/if}
				{/each}
				{#if $filteredRows.length === 0}
					<tr>
						<td colspan={COLSPAN} class="px-3 py-10 text-center text-pqs-bluegray">
							No parameter sets match the current filters.
						</td>
					</tr>
				{/if}
			</tbody>
		</table>
	</div>
	<div class="border-t border-pqs-ashgray bg-pqs-smoke px-3 py-1.5 font-heading text-xs text-pqs-steel dark:border-pqs-steel dark:bg-pqs-midnight-mid dark:text-pqs-bluegray">
		<span>{$filteredRows.length} parameter set{$filteredRows.length === 1 ? '' : 's'}</span>{#if $store.paramSets === 'recommended'}<span> shown (recommended sets)</span>{/if}
	</div>
	<TableLegend />
</div>
```

- [ ] **Step 5: Rewrite `src/lib/components/KemTable.svelte`**

```svelte
<script lang="ts">
	import { programmeChips, securityBadges, type BadgeKind } from '$lib/badges';
	import type { KemParameterSet, KemSortableColumn } from '$lib/types';
	import { allProgrammes } from '$lib/programmeData';
	import { CYCLES_PER_US, CPU_GHZ_LABEL } from '$lib/constants';
	import { fmt, fmtCycles, fmtTime } from '$lib/format';
	import { kemTimes } from '$lib/timing';
	import BadgeIcon from './BadgeIcon.svelte';
	import ProgrammeChips from './ProgrammeChips.svelte';
	import TableLegend from './TableLegend.svelte';

	let { rows, recommendedOnly }: { rows: KemParameterSet[]; recommendedOnly: boolean } = $props();

	const idPrefix = $props.id();

	let sortCol = $state<KemSortableColumn>('pkPlusCt');
	let sortDir = $state<'asc' | 'desc'>('asc');

	function shortParam(row: KemParameterSet): string {
		const prefix = row.scheme + '-';
		return row.parameterset.startsWith(prefix) ? row.parameterset.slice(prefix.length) : row.parameterset;
	}

	/** Sortable columns; the Programmes column (not sortable) is inserted before index 2. */
	const COLUMNS: { key: KemSortableColumn; label: string; numeric?: boolean }[] = [
		{ key: 'scheme', label: 'Scheme' },
		{ key: 'category', label: 'Category' },
		{ key: 'parameterset', label: 'Parameter Set' },
		{ key: 'level', label: 'Level', numeric: true },
		{ key: 'pk', label: 'pk (B)', numeric: true },
		{ key: 'ct', label: 'ct (B)', numeric: true },
		{ key: 'pkPlusCt', label: 'pk+ct (B)', numeric: true },
		{ key: 'keygenUs', label: 'Keygen', numeric: true },
		{ key: 'encapsUs', label: 'Encaps', numeric: true },
		{ key: 'decapsUs', label: 'Decaps', numeric: true }
	];
	const COLSPAN = COLUMNS.length + 2; // + Programmes + Assumption

	const DETAIL_TONE: Record<BadgeKind, string> = {
		'pre-quantum': 'bg-pqs-smoke dark:bg-pqs-steel/20',
		broken: 'bg-[#fdf2f2] dark:bg-[#3b1219]',
		implementation: 'bg-[#fff4ec] dark:bg-[#3a1d0c]',
		warning: 'bg-[#fdf6e7] dark:bg-[#33270a]',
		note: 'bg-pqs-smoke dark:bg-pqs-steel/20'
	};

	const levelOrder: Record<string, number> = { 'Pre-Quantum': 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5 };

	// Displayed time in µs, or null when the row has no timing data (shown as "—").
	const TIME_VALUE: Partial<Record<KemSortableColumn, (r: KemParameterSet) => number | null>> = {
		keygenUs: (r) => kemTimes(r).keygen,
		encapsUs: (r) => kemTimes(r).encaps,
		decapsUs: (r) => kemTimes(r).decaps
	};

	function compare(a: KemParameterSet, b: KemParameterSet, col: KemSortableColumn): number {
		switch (col) {
			case 'level':
				return (levelOrder[String(a.level)] ?? 99) - (levelOrder[String(b.level)] ?? 99);
			case 'pk':
				return a.pk - b.pk;
			case 'ct':
				return a.ct - b.ct;
			case 'pkPlusCt':
				return a.pkPlusCt - b.pkPlusCt;
			default: {
				const av = String(a[col] ?? '').toLowerCase();
				const bv = String(b[col] ?? '').toLowerCase();
				return av < bv ? -1 : av > bv ? 1 : 0;
			}
		}
	}

	const sortedRows = $derived(
		[...rows].sort((a, b) => {
			// Time columns keep rows without data last regardless of direction.
			const timeValue = TIME_VALUE[sortCol];
			if (timeValue) {
				const av = timeValue(a);
				const bv = timeValue(b);
				if (av == null && bv == null) return 0;
				if (av == null) return 1;
				if (bv == null) return -1;
				return sortDir === 'asc' ? av - bv : bv - av;
			}
			const cmp = compare(a, b, sortCol);
			return sortDir === 'asc' ? cmp : -cmp;
		})
	);

	/** Which badge's detail row is open, per parameter set. */
	let open = $state<Record<string, BadgeKind | undefined>>({});

	function rowKey(row: KemParameterSet): string {
		return `${row.scheme}/${row.parameterset}`;
	}

	function toggle(key: string, kind: BadgeKind) {
		open[key] = open[key] === kind ? undefined : kind;
	}

	function setSort(col: KemSortableColumn) {
		if (sortCol === col) {
			sortDir = sortDir === 'asc' ? 'desc' : 'asc';
		} else {
			sortCol = col;
			sortDir = 'asc';
		}
	}

	function sortIndicator(col: KemSortableColumn): string {
		if (sortCol !== col) return '';
		return sortDir === 'asc' ? ' ▲' : ' ▼';
	}

	function ariaSort(col: KemSortableColumn): 'ascending' | 'descending' | undefined {
		if (sortCol !== col) return undefined;
		return sortDir === 'asc' ? 'ascending' : 'descending';
	}
</script>

{#snippet timeCell(us: number | null, cyc: number)}
	<td class="px-3 py-1.5 text-right tabular-nums">
		{#if us != null}
			{fmtTime(us)}
		{:else if cyc > 0}
			<span
				class="underline decoration-wavy decoration-pqs-scarlet"
				title="Estimated from {fmtCycles(cyc)} cycles @ {CPU_GHZ_LABEL}"
			>{fmtTime(cyc / CYCLES_PER_US)}<span class="sr-only"> (estimated from {fmtCycles(cyc)} cycles @ {CPU_GHZ_LABEL})</span></span>
		{:else}
			<span class="text-pqs-bluegray" title="Not yet benchmarked">—<span class="sr-only"> not yet benchmarked</span></span>
		{/if}
	</td>
{/snippet}

<div class="overflow-hidden rounded border border-pqs-ashgray shadow-sm dark:border-pqs-steel">
	<div class="overflow-x-auto">
		<table class="min-w-full text-sm">
			<thead class="sticky top-0 z-10">
				<tr class="bg-pqs-steel font-heading text-xs text-white">
					{#each COLUMNS as col, i (col.key)}
						{#if i === 2}
							<th scope="col" class="whitespace-nowrap px-3 py-2.5 text-left font-semibold">Programmes</th>
						{/if}
						<th scope="col" aria-sort={ariaSort(col.key)} class="whitespace-nowrap p-0">
							<button
								type="button"
								onclick={() => setSort(col.key)}
								class="w-full cursor-pointer select-none px-3 py-2.5 font-semibold hover:bg-pqs-steel-light {col.numeric ? 'text-right' : 'text-left'}"
							>
								{col.label}{sortIndicator(col.key)}
							</button>
						</th>
					{/each}
					<th scope="col" class="px-3 py-2.5 text-left text-xs font-semibold">Assumption</th>
				</tr>
			</thead>
			<tbody class="divide-y divide-pqs-ashgray bg-white dark:divide-pqs-steel dark:bg-pqs-midnight-mid">
				{#each sortedRows as row, i (rowKey(row))}
					{@const key = rowKey(row)}
					{@const badges = securityBadges(row)}
					{@const openBadge = badges.find((b) => b.kind === open[key])}
					{@const detailId = `${idPrefix}-detail-${i}`}
					<tr class="hover:bg-pqs-smoke dark:hover:bg-pqs-steel/30">
						<td class="whitespace-nowrap px-3 py-1.5">
							<a
								href={row.website}
								target="_blank"
								rel="noopener noreferrer"
								class="font-heading font-semibold text-pqs-steel hover:text-pqs-apricot dark:text-pqs-apricot dark:hover:text-pqs-apricot-light"
							>{row.scheme}</a>
							{#each badges as badge (badge.kind)}
								<button
									type="button"
									class="ml-1 inline-grid h-6 w-6 place-items-center rounded align-middle hover:bg-pqs-ashgray focus-visible:outline-2 focus-visible:outline-pqs-apricot dark:hover:bg-pqs-steel/40"
									aria-label="{badge.title}: show details"
									aria-expanded={open[key] === badge.kind}
									aria-controls={open[key] === badge.kind ? detailId : undefined}
									onclick={() => toggle(key, badge.kind)}
								><BadgeIcon kind={badge.kind} /></button>
							{/each}
						</td>
						<td class="whitespace-nowrap px-3 py-1.5 text-pqs-steel dark:text-pqs-bluegray">{row.category}</td>
						<td class="px-3 py-1.5"><ProgrammeChips chips={programmeChips(row, allProgrammes)} /></td>
						<td class="whitespace-nowrap px-3 py-1.5 font-mono text-xs" title={row.parameterset}>{shortParam(row)}</td>
						<td class="px-3 py-1.5 text-right tabular-nums text-pqs-steel dark:text-pqs-bluegray">
							{#if row.level !== null}
								{row.level === 'Pre-Quantum' ? 'N/A' : row.level}
							{:else if row.ngccLevel !== null}
								<span class="font-semibold text-[#6b21a8] dark:text-[#d8b4fe]" title="China NGCC target in classical bits; not a NIST level">{row.ngccLevel}-bit</span>
							{:else}
								—
							{/if}
						</td>
						<td class="px-3 py-1.5 text-right tabular-nums">{fmt(row.pk)}</td>
						<td class="px-3 py-1.5 text-right tabular-nums">{fmt(row.ct)}</td>
						<td class="px-3 py-1.5 text-right tabular-nums">{fmt(row.pkPlusCt)}</td>
						{@render timeCell(row.keygenUs, row.keygenCycles)}
						{@render timeCell(row.encapsUs, row.encapsCycles)}
						{@render timeCell(row.decapsUs, row.decapsCycles)}
						<td class="px-3 py-1.5 text-pqs-steel/80 dark:text-pqs-bluegray">{row.assumption}</td>
					</tr>
					{#if openBadge}
						<tr id={detailId} class={DETAIL_TONE[openBadge.kind]}>
							<td colspan={COLSPAN} class="whitespace-normal px-3 py-2.5 pl-9 text-sm leading-relaxed text-pqs-midnight dark:text-pqs-smoke [&_a]:text-pqs-apricot [&_a]:underline">
								<strong class="font-heading">{openBadge.title}:</strong>
								{@html openBadge.html}
								{#if openBadge.href}
									<a href={openBadge.href} target="_blank" rel="noopener noreferrer">Full findings report</a>
								{/if}
							</td>
						</tr>
					{/if}
				{/each}
				{#if sortedRows.length === 0}
					<tr>
						<td colspan={COLSPAN} class="px-3 py-10 text-center text-pqs-bluegray">No KEMs to display.</td>
					</tr>
				{/if}
			</tbody>
		</table>
	</div>
	<div class="border-t border-pqs-ashgray bg-pqs-smoke px-3 py-1.5 font-heading text-xs text-pqs-steel dark:border-pqs-steel dark:bg-pqs-midnight-mid dark:text-pqs-bluegray">
		<span>{sortedRows.length} parameter set{sortedRows.length === 1 ? '' : 's'}</span>{#if recommendedOnly}<span> shown (recommended sets)</span>{/if}
	</div>
	<TableLegend />
</div>
```

- [ ] **Step 6: Pass `recommendedOnly` from the KEM pages and delete the old badge**

In `src/routes/kems/+page.svelte` and `src/routes/kems/advanced/+page.svelte`, replace `<KemTable rows={filteredRows} />` with:

```svelte
<KemTable rows={filteredRows} recommendedOnly={filter.paramSets === 'recommended'} />
```

Then:

```bash
git rm src/lib/components/SecurityBadge.svelte
```

- [ ] **Step 7: Run all tests to verify they pass**

Run: `npm run check && npm run test && npm run test:e2e`
Expected: PASS. The other agent's `schemeCells` helper reads `td a` of each row, and detail rows are only rendered when open, so it is unaffected.

- [ ] **Step 8: Commit**

```bash
git add src/lib/components/BadgeIcon.svelte src/lib/components/ProgrammeChips.svelte src/lib/components/TableLegend.svelte src/lib/components/SchemeTable.svelte src/lib/components/KemTable.svelte src/routes/kems/+page.svelte src/routes/kems/advanced/+page.svelte e2e/signatures.spec.ts e2e/kems.spec.ts
git commit -m "$(cat <<'EOF'
Show programmes, badge detail rows and unbenchmarked cells in tables

Security badges are buttons that open a full-width detail row;
implementation findings have their own icon and link to the report.
Missing timings read "Not yet benchmarked".

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
EOF
)"
```

---

### Task 7: Shared filter sections and one collapsible filter column

**Files:**
- Create: `src/lib/components/filterStyles.ts`, `src/lib/components/FilterSidebar.svelte`, `src/lib/components/ProgrammeFilter.svelte`, `src/lib/components/LevelFilter.svelte`, `src/lib/components/ParamSetFilter.svelte`, `src/lib/components/StatusFilter.svelte`
- Modify (rewrite): `src/lib/components/FilterPanel.svelte`, `src/lib/components/KemFilterPanel.svelte`, `src/routes/kems/+page.svelte`
- Modify: `src/routes/signatures/+page.svelte`, `src/routes/signatures/advanced/+page.svelte`, `src/routes/kems/advanced/+page.svelte`
- Test: `e2e/signatures.spec.ts`, `e2e/kems.spec.ts`

**Interfaces:**
- Consumes: `CommonFilter`, `PROGRAMME_GROUP_LABELS`, `filterSummary`, `presentProgrammeGroups` (Task 1); `defaultKemFilter` (Task 4).
- Produces:
  - `FilterSidebar { summary: string; children: Snippet }`: on screens below `lg`, a "Filters" button (`aria-expanded`, `aria-controls`) that starts collapsed; on `lg` and up the body is always shown in a sticky 16 rem column. There is exactly **one** filter panel per page.
  - `ProgrammeFilter { filter: CommonFilter; groups: ProgrammeGroup[]; onchange: (patch: Partial<CommonFilter>) => void; onramp?: Snippet<[boolean]> }`: `onramp` is rendered under the "NIST on-ramp" checkbox and gets `true` while that checkbox is ticked.
  - `LevelFilter`, `ParamSetFilter`, `StatusFilter` with `{ filter: CommonFilter; onchange }`.
  - `FilterPanel { schemes; categories; ranges; groups: ProgrammeGroup[]; onramp?: Snippet<[boolean]> }`
  - `KemFilterPanel { schemes; categories; ranges; groups; filter (bindable) }`, now with Performance (µs) and Security status sections.
- Labels (e2e relies on them): programme labels from `PROGRAMME_GROUP_LABELS`; level checkboxes "Level 1"…"Level 5", "Pre-quantum", "128-bit"…"512-bit"; fieldsets "Programme", "NIST category", "NGCC target (classical bits)", "Parameter sets", "Security status"; radios "Recommended (one per scheme and level)" and "All parameter sets"; checkboxes "Hide broken by design", "Hide schemes with implementation findings" and "Hide warnings"; a "Reset filters" button; KEM range fields "Min keygen" … "Max decaps".

- [ ] **Step 1: Write the failing e2e tests**

Append to `e2e/signatures.spec.ts` (a new top-level block after the existing one):

```ts
test.describe('Signatures filters', () => {
	test('programme filter narrows the table and survives a reload', async ({ page }) => {
		await page.goto('/signatures/');
		await page.waitForLoadState('networkidle');
		const panel = page.locator('aside').first();
		await panel.getByLabel('NIST standards').uncheck();
		await expect(page).toHaveURL(/[?&]p=/);
		await expect(page.locator('tbody')).not.toContainText('ML-DSA');
		await page.reload();
		await expect(page.locator('tbody')).not.toContainText('ML-DSA');
		await expect(panel.getByLabel('NIST standards')).not.toBeChecked();
	});

	test('security levels come in separate NIST and NGCC groups', async ({ page }) => {
		await page.goto('/signatures/');
		const nist = page.getByRole('group', { name: 'NIST category' });
		const ngcc = page.getByRole('group', { name: 'NGCC target (classical bits)' });
		await expect(nist.getByLabel('Level 1')).toBeChecked();
		await expect(nist.getByLabel('Pre-quantum', { exact: true })).toBeChecked();
		await expect(ngcc.getByLabel('128-bit')).toBeChecked();
		await expect(ngcc.getByLabel('512-bit')).toBeChecked();
	});

	test('parameter-set toggle defaults to recommended and is kept in the URL', async ({ page }) => {
		await page.goto('/signatures/');
		await page.waitForLoadState('networkidle');
		await expect(page.getByLabel('Recommended (one per scheme and level)')).toBeChecked();
		await page.getByLabel('All parameter sets').check();
		await expect(page).toHaveURL(/[?&]ps=all/);
	});

	test('status filters are three separate checkboxes', async ({ page }) => {
		await page.goto('/signatures/');
		await page.waitForLoadState('networkidle');
		const status = page.getByRole('group', { name: 'Security status' });
		await expect(status.getByLabel('Hide broken by design')).not.toBeChecked();
		await expect(status.getByLabel('Hide schemes with implementation findings')).not.toBeChecked();
		await status.getByLabel('Hide warnings').check();
		await expect(page).toHaveURL(/[?&]hw=1/);
	});

	test('filters start collapsed on a phone and open from the Filters button', async ({ page }) => {
		await page.setViewportSize({ width: 390, height: 844 });
		await page.goto('/signatures/');
		await page.waitForLoadState('networkidle');
		const toggle = page.getByRole('button', { name: /^Filters/ });
		await expect(toggle).toHaveAttribute('aria-expanded', 'false');
		await expect(page.getByLabel('NIST standards')).toBeHidden();
		await toggle.click();
		await expect(toggle).toHaveAttribute('aria-expanded', 'true');
		await expect(page.getByLabel('NIST standards')).toBeVisible();
	});

	test('desktop shows the filter column without a toggle', async ({ page }) => {
		await page.goto('/signatures/');
		await expect(page.getByRole('button', { name: /^Filters/ })).toBeHidden();
		await expect(page.getByLabel('NIST standards')).toBeVisible();
		await expect(page.locator('aside')).toHaveCount(1);
	});
});
```

In `e2e/kems.spec.ts`, replace the YAML loader block (from `function loadSourceParameterSets()` through `const expectedNaCount = …;`) with:

```ts
interface SourceKem {
	programmes?: Record<string, unknown>;
	broken?: string;
	parametersets: {
		level: unknown;
		recommended?: boolean;
		encaps_us?: number;
		encaps_cycles?: number;
	}[];
}

function loadSourceKems(): SourceKem[] {
	return fs
		.readdirSync(KEMS_DIR)
		.filter((f) => f.endsWith('.yaml'))
		.map((file) => yaml.load(fs.readFileSync(path.join(KEMS_DIR, file), 'utf8')) as SourceKem);
}

const sourceKems = loadSourceKems();
// The default view shows recommended parameter sets: if a KEM flags any, only those.
const shownSets = (k: SourceKem) =>
	k.parametersets.some((p) => p.recommended) ? k.parametersets.filter((p) => p.recommended) : k.parametersets;
const shownParameterSets = sourceKems.flatMap(shownSets);
const expectedTotal = shownParameterSets.length;
const expectedNaCount = shownParameterSets.filter((ps) => ps.level === 'Pre-Quantum').length;
const researchCount = sourceKems
	.filter((k) => !k.programmes && k.broken !== 'classical')
	.reduce((n, k) => n + shownSets(k).length, 0);
const unbenchmarkedEncaps = shownParameterSets.filter((ps) => ps.encaps_us == null && !(ps.encaps_cycles! > 0)).length;
```

Replace both uses of `panel.getByLabel('N/A').uncheck()` with `panel.getByLabel('Pre-quantum', { exact: true }).uncheck()`. Then append inside `test.describe('KEMs page', …)`:

```ts
	test('programme filter hides research proposals', async ({ page }) => {
		expect(researchCount).toBeGreaterThan(0);
		await page.goto('/kems/');
		await page.waitForLoadState('networkidle');
		const panel = page.locator('aside').first();
		await panel.getByLabel('Research proposals').uncheck();
		await expect.poll(() => paramSetCount(page)).toBe(expectedTotal - researchCount);
		await expect(page).toHaveURL(/[?&]p=/);
	});

	test('performance filter bounds encapsulation time and keeps unbenchmarked rows', async ({ page }) => {
		await page.goto('/kems/');
		await page.waitForLoadState('networkidle');
		const panel = page.locator('aside').first();
		await panel.getByLabel('Max encaps').fill('0.0001');
		await expect.poll(() => paramSetCount(page)).toBe(unbenchmarkedEncaps);
		await expect(page).toHaveURL(/[?&]euMax=/);
	});

	test('KEM filters have the same sections as the signatures page', async ({ page }) => {
		await page.goto('/kems/');
		for (const name of ['Programme', 'NIST category', 'NGCC target (classical bits)', 'Parameter sets', 'Security status']) {
			await expect(page.getByRole('group', { name })).toBeVisible();
		}
		await expect(page.getByRole('button', { name: 'Reset filters' })).toBeVisible();
	});
```

- [ ] **Step 2: Run them to verify they fail**

Run: `npx playwright test e2e/signatures.spec.ts e2e/kems.spec.ts -g "filter|Filters|security levels|parameter-set|status"`
Expected: FAIL (no "NIST standards" checkbox, no fieldsets; KEM `Pre-quantum` label missing).

- [ ] **Step 3: Create the shared filter pieces**

`src/lib/components/filterStyles.ts`:

```ts
/** Section heading (fieldset legend) in the filter column. */
export const FILTER_HEADING =
	'mb-2 font-heading text-xs font-semibold uppercase tracking-wider text-pqs-steel dark:text-pqs-apricot';

/** Sub-group heading inside a filter section. */
export const FILTER_SUBHEADING = 'mb-1 mt-1 text-xs text-pqs-steel/80 dark:text-pqs-bluegray';

export const FILTER_OPTION = 'flex cursor-pointer items-center gap-1.5';

export const FILTER_OPTION_TEXT = 'text-pqs-steel dark:text-pqs-bluegray';
```

`src/lib/components/FilterSidebar.svelte`:

```svelte
<script lang="ts">
	import type { Snippet } from 'svelte';

	let { summary, children }: { summary: string; children: Snippet } = $props();

	let open = $state(false);
	const bodyId = $props.id();
</script>

<div class="w-full shrink-0 lg:sticky lg:top-4 lg:max-h-[calc(100vh-2rem)] lg:w-64 lg:self-start lg:overflow-y-auto">
	<button
		type="button"
		class="flex min-h-11 w-full items-center gap-3 rounded border border-pqs-ashgray bg-white px-3 text-left dark:border-pqs-steel dark:bg-pqs-midnight-mid lg:hidden"
		aria-expanded={open}
		aria-controls={bodyId}
		onclick={() => (open = !open)}
	>
		<span class="font-heading font-bold text-pqs-midnight dark:text-white">Filters</span>
		<span class="flex-1 text-sm text-pqs-steel dark:text-pqs-bluegray">{summary}</span>
		<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true" class="transition-transform {open ? 'rotate-180' : ''}"><path d="M6 9l6 6 6-6" /></svg>
	</button>
	<div id={bodyId} class={open ? 'mt-3 lg:mt-0' : 'hidden lg:block'}>
		{@render children()}
	</div>
</div>
```

`src/lib/components/ProgrammeFilter.svelte`:

```svelte
<script lang="ts">
	import type { Snippet } from 'svelte';
	import { PROGRAMME_GROUP_LABELS } from '$lib/compareFilters';
	import type { CommonFilter, ProgrammeGroup } from '$lib/types';
	import { FILTER_HEADING, FILTER_OPTION, FILTER_OPTION_TEXT } from './filterStyles';

	interface Props {
		filter: CommonFilter;
		groups: ProgrammeGroup[];
		onchange: (patch: Partial<CommonFilter>) => void;
		/** Rendered under "NIST on-ramp"; gets whether that programme is ticked. */
		onramp?: Snippet<[boolean]>;
	}
	let { filter, groups, onchange, onramp }: Props = $props();

	function toggle(group: ProgrammeGroup) {
		const next = new Set(filter.programmes);
		if (next.has(group)) next.delete(group);
		else next.add(group);
		onchange({ programmes: next });
	}
</script>

<fieldset class="mb-5">
	<legend class={FILTER_HEADING}>Programme</legend>
	<div class="space-y-0.5">
		{#each groups as group (group)}
			<label class={FILTER_OPTION}>
				<input type="checkbox" class="accent-pqs-apricot" checked={filter.programmes.has(group)} onchange={() => toggle(group)} />
				<span class={FILTER_OPTION_TEXT}>{PROGRAMME_GROUP_LABELS[group]}</span>
				{#if group === 'china-ngcc'}
					<span class="rounded bg-[#fdf3e6] px-1.5 text-[11px] font-bold text-[#8a5208]">New</span>
				{/if}
			</label>
			{#if group === 'nist-onramp' && onramp}
				<div class="ml-5">{@render onramp(filter.programmes.has('nist-onramp'))}</div>
			{/if}
		{/each}
	</div>
</fieldset>
```

`src/lib/components/LevelFilter.svelte`:

```svelte
<script lang="ts">
	import { NGCC_LEVELS } from '$lib/constants';
	import type { CommonFilter, NgccLevel, NistLevel } from '$lib/types';
	import { FILTER_HEADING, FILTER_OPTION, FILTER_OPTION_TEXT, FILTER_SUBHEADING } from './filterStyles';

	let { filter, onchange }: { filter: CommonFilter; onchange: (patch: Partial<CommonFilter>) => void } = $props();

	const NIST_ORDER: NistLevel[] = [1, 2, 3, 4, 5, 'Pre-Quantum'];

	function toggleNist(level: NistLevel) {
		const next = new Set(filter.levels);
		if (next.has(level)) next.delete(level);
		else next.add(level);
		onchange({ levels: next });
	}

	function toggleNgcc(level: NgccLevel) {
		const next = new Set(filter.ngccLevels);
		if (next.has(level)) next.delete(level);
		else next.add(level);
		onchange({ ngccLevels: next });
	}
</script>

<fieldset class="mb-5">
	<legend class={FILTER_HEADING}>Security level</legend>
	<fieldset class="space-y-0.5">
		<legend class={FILTER_SUBHEADING}>NIST category</legend>
		{#each NIST_ORDER as level (level)}
			<label class={FILTER_OPTION}>
				<input type="checkbox" class="accent-pqs-apricot" checked={filter.levels.has(level)} onchange={() => toggleNist(level)} />
				<span class={FILTER_OPTION_TEXT}>{level === 'Pre-Quantum' ? 'Pre-quantum' : `Level ${level}`}</span>
			</label>
		{/each}
	</fieldset>
	<fieldset class="mt-2 space-y-0.5">
		<legend class={FILTER_SUBHEADING}>NGCC target (classical bits)</legend>
		{#each NGCC_LEVELS as level (level)}
			<label class={FILTER_OPTION}>
				<input type="checkbox" class="accent-pqs-apricot" checked={filter.ngccLevels.has(level)} onchange={() => toggleNgcc(level)} />
				<span class={FILTER_OPTION_TEXT}>{level}-bit</span>
			</label>
		{/each}
	</fieldset>
</fieldset>
```

`src/lib/components/ParamSetFilter.svelte`:

```svelte
<script lang="ts">
	import type { CommonFilter } from '$lib/types';
	import { FILTER_HEADING, FILTER_OPTION, FILTER_OPTION_TEXT } from './filterStyles';

	let { filter, onchange }: { filter: CommonFilter; onchange: (patch: Partial<CommonFilter>) => void } = $props();

	const name = $props.id();
</script>

<fieldset class="mb-5 space-y-0.5">
	<legend class={FILTER_HEADING}>Parameter sets</legend>
	<label class={FILTER_OPTION}>
		<input type="radio" {name} class="accent-pqs-apricot" checked={filter.paramSets === 'recommended'} onchange={() => onchange({ paramSets: 'recommended' })} />
		<span class={FILTER_OPTION_TEXT}>Recommended (one per scheme and level)</span>
	</label>
	<label class={FILTER_OPTION}>
		<input type="radio" {name} class="accent-pqs-apricot" checked={filter.paramSets === 'all'} onchange={() => onchange({ paramSets: 'all' })} />
		<span class={FILTER_OPTION_TEXT}>All parameter sets</span>
	</label>
</fieldset>
```

`src/lib/components/StatusFilter.svelte`:

```svelte
<script lang="ts">
	import type { CommonFilter } from '$lib/types';
	import { FILTER_HEADING, FILTER_OPTION, FILTER_OPTION_TEXT } from './filterStyles';

	let { filter, onchange }: { filter: CommonFilter; onchange: (patch: Partial<CommonFilter>) => void } = $props();
</script>

<fieldset class="mb-5 space-y-0.5">
	<legend class={FILTER_HEADING}>Security status</legend>
	<label class={FILTER_OPTION}>
		<input type="checkbox" class="accent-pqs-apricot" checked={filter.hideBroken} onchange={(e) => onchange({ hideBroken: e.currentTarget.checked })} />
		<span class={FILTER_OPTION_TEXT}>Hide broken by design</span>
	</label>
	<label class={FILTER_OPTION}>
		<input type="checkbox" class="accent-pqs-apricot" checked={filter.hideImplementation} onchange={(e) => onchange({ hideImplementation: e.currentTarget.checked })} />
		<span class={FILTER_OPTION_TEXT}>Hide schemes with implementation findings</span>
	</label>
	<label class={FILTER_OPTION}>
		<input type="checkbox" class="accent-pqs-apricot" checked={filter.hideWarnings} onchange={(e) => onchange({ hideWarnings: e.currentTarget.checked })} />
		<span class={FILTER_OPTION_TEXT}>Hide warnings</span>
	</label>
</fieldset>
```

- [ ] **Step 4: Rewrite `src/lib/components/FilterPanel.svelte`**

```svelte
<script lang="ts">
	import type { Snippet } from 'svelte';
	import type { CommonFilter, DataRanges, ProgrammeGroup, Scheme } from '$lib/types';
	import { getFilterStore } from '$lib/filterStore';
	import RangeField from './RangeField.svelte';
	import ProgrammeFilter from './ProgrammeFilter.svelte';
	import LevelFilter from './LevelFilter.svelte';
	import ParamSetFilter from './ParamSetFilter.svelte';
	import StatusFilter from './StatusFilter.svelte';
	import { FILTER_HEADING } from './filterStyles';

	interface Props {
		schemes: Scheme[];
		categories: string[];
		ranges: DataRanges;
		groups: ProgrammeGroup[];
		/** Rendered under "NIST on-ramp" (the on-ramp round select). */
		onramp?: Snippet<[boolean]>;
	}
	let { schemes, categories, ranges, groups, onramp }: Props = $props();

	const { store } = getFilterStore();

	function patch(p: Partial<CommonFilter>) {
		store.update((f) => ({ ...f, ...p }));
	}

	const schemesByCategory = $derived(
		Object.fromEntries(categories.map((cat) => [cat, schemes.filter((s) => s.category === cat)]))
	);

	function categoryState(cat: string): 'all' | 'some' | 'none' {
		const catSchemes = schemesByCategory[cat];
		const n = catSchemes.filter((s) => $store.schemes.has(s.scheme)).length;
		if (n === catSchemes.length) return 'all';
		if (n === 0) return 'none';
		return 'some';
	}

	function toggleCategory(cat: string) {
		const catSchemes = schemesByCategory[cat].map((s) => s.scheme);
		const next = new Set($store.schemes);
		if (categoryState(cat) === 'all') catSchemes.forEach((s) => next.delete(s));
		else catSchemes.forEach((s) => next.add(s));
		store.update((f) => ({ ...f, schemes: next }));
	}

	function toggleScheme(scheme: string) {
		const next = new Set($store.schemes);
		if (next.has(scheme)) next.delete(scheme);
		else next.add(scheme);
		store.update((f) => ({ ...f, schemes: next }));
	}

	// Defaults are read at call time: they change when the round changes.
	function selectAll() {
		store.update((f) => ({ ...f, schemes: new Set(getFilterStore().defaults.schemes) }));
	}

	function selectNone() {
		store.update((f) => ({ ...f, schemes: new Set() }));
	}

	function reset() {
		store.update((f) => ({ ...getFilterStore().defaults, sortCol: f.sortCol, sortDir: f.sortDir, chart: f.chart }));
	}

	function indeterminate(el: HTMLInputElement, value: boolean) {
		el.indeterminate = value;
		return {
			update(v: boolean) {
				el.indeterminate = v;
			},
		};
	}
</script>

<aside class="rounded border border-pqs-ashgray bg-white p-4 text-sm shadow-sm dark:border-pqs-steel dark:bg-pqs-midnight-mid">
	<h2 class="mb-4 hidden font-heading text-base font-bold text-pqs-midnight dark:text-white lg:block">Filters</h2>

	<ProgrammeFilter filter={$store} {groups} onchange={patch} {onramp} />

	<hr class="mb-4 border-pqs-ashgray dark:border-pqs-steel" />

	<section class="mb-5">
		<div class="mb-2 flex items-center justify-between">
			<h3 class={FILTER_HEADING}>Schemes</h3>
			<span class="space-x-2 font-heading text-xs">
				<button type="button" onclick={selectAll} class="text-pqs-apricot underline hover:no-underline">All</button>
				<button type="button" onclick={selectNone} class="text-pqs-apricot underline hover:no-underline">None</button>
			</span>
		</div>
		{#each categories as cat (cat)}
			<details class="mb-1" open>
				<summary class="flex cursor-pointer list-none items-center gap-1.5 py-0.5">
					<input
						type="checkbox"
						aria-label="All {cat} schemes"
						checked={categoryState(cat) !== 'none'}
						use:indeterminate={categoryState(cat) === 'some'}
						onchange={() => toggleCategory(cat)}
						class="shrink-0 accent-pqs-apricot"
					/>
					<span class="font-heading font-semibold text-pqs-midnight dark:text-pqs-smoke">{cat}</span>
				</summary>
				<div class="ml-5 mt-0.5 space-y-0.5">
					{#each schemesByCategory[cat] as scheme (scheme.scheme)}
						<label class="flex cursor-pointer items-center gap-1.5">
							<input
								type="checkbox"
								checked={$store.schemes.has(scheme.scheme)}
								onchange={() => toggleScheme(scheme.scheme)}
								class="shrink-0 accent-pqs-apricot"
							/>
							<span class="text-pqs-steel dark:text-pqs-bluegray">{scheme.scheme}</span>
						</label>
					{/each}
				</div>
			</details>
		{/each}
	</section>

	<hr class="mb-4 border-pqs-ashgray dark:border-pqs-steel" />

	<LevelFilter filter={$store} onchange={patch} />
	<ParamSetFilter filter={$store} onchange={patch} />

	<hr class="mb-4 border-pqs-ashgray dark:border-pqs-steel" />

	<section class="mb-5">
		<h3 class={FILTER_HEADING}>Sizes (bytes)</h3>
		<div class="space-y-2">
			<RangeField label="Min pk" value={$store.minPk} onchange={(v) => store.update((f) => ({ ...f, minPk: v }))} />
			<RangeField label="Max pk" value={$store.maxPk} onchange={(v) => store.update((f) => ({ ...f, maxPk: v }))} />
			<RangeField label="Min sig" value={$store.minSig} onchange={(v) => store.update((f) => ({ ...f, minSig: v }))} />
			<RangeField label="Max sig" value={$store.maxSig} onchange={(v) => store.update((f) => ({ ...f, maxSig: v }))} />
			<RangeField label="Min pk+sig" value={$store.minPkPlusSig} onchange={(v) => store.update((f) => ({ ...f, minPkPlusSig: v }))} />
			<RangeField label="Max pk+sig" value={$store.maxPkPlusSig} onchange={(v) => store.update((f) => ({ ...f, maxPkPlusSig: v }))} />
		</div>
	</section>

	<section class="mb-5">
		<h3 class={FILTER_HEADING}>Performance (cycles)</h3>
		<div class="space-y-2">
			<RangeField label="Min signing" value={$store.minSigningCycles} onchange={(v) => store.update((f) => ({ ...f, minSigningCycles: v }))} />
			<RangeField label="Max signing" value={$store.maxSigningCycles} onchange={(v) => store.update((f) => ({ ...f, maxSigningCycles: v }))} />
			<RangeField label="Min verification" value={$store.minVerificationCycles} onchange={(v) => store.update((f) => ({ ...f, minVerificationCycles: v }))} />
			<RangeField label="Max verification" value={$store.maxVerificationCycles} onchange={(v) => store.update((f) => ({ ...f, maxVerificationCycles: v }))} />
		</div>
	</section>

	{#if ranges.signingUs || ranges.verificationUs}
		<section class="mb-5">
			<h3 class={FILTER_HEADING}>Performance (µs)</h3>
			<div class="space-y-2">
				{#if ranges.signingUs}
					<RangeField label="Min signing µs" value={$store.minSigningUs} onchange={(v) => store.update((f) => ({ ...f, minSigningUs: v }))} />
					<RangeField label="Max signing µs" value={$store.maxSigningUs} onchange={(v) => store.update((f) => ({ ...f, maxSigningUs: v }))} />
				{/if}
				{#if ranges.verificationUs}
					<RangeField label="Min verification µs" value={$store.minVerificationUs} onchange={(v) => store.update((f) => ({ ...f, minVerificationUs: v }))} />
					<RangeField label="Max verification µs" value={$store.maxVerificationUs} onchange={(v) => store.update((f) => ({ ...f, maxVerificationUs: v }))} />
				{/if}
			</div>
		</section>
	{/if}

	<hr class="mb-4 border-pqs-ashgray dark:border-pqs-steel" />

	<StatusFilter filter={$store} onchange={patch} />

	<button
		type="button"
		onclick={reset}
		class="h-10 w-full rounded border border-pqs-bluegray font-heading font-semibold text-pqs-steel hover:bg-pqs-smoke dark:border-pqs-steel dark:text-pqs-apricot dark:hover:bg-pqs-steel/30"
	>Reset filters</button>
</aside>
```

(The µs range labels gain a "µs" suffix so they no longer clash with the cycles fields of the same name. No test used the old duplicate labels.)

- [ ] **Step 5: Rewrite `src/lib/components/KemFilterPanel.svelte`**

```svelte
<script lang="ts">
	import type { CommonFilter, KemDataRanges, KemFilterState, KemScheme, ProgrammeGroup } from '$lib/types';
	import { defaultKemFilter } from '$lib/kemData';
	import RangeField from './RangeField.svelte';
	import ProgrammeFilter from './ProgrammeFilter.svelte';
	import LevelFilter from './LevelFilter.svelte';
	import ParamSetFilter from './ParamSetFilter.svelte';
	import StatusFilter from './StatusFilter.svelte';
	import { FILTER_HEADING } from './filterStyles';

	interface Props {
		schemes: KemScheme[];
		categories: string[];
		ranges: KemDataRanges;
		groups: ProgrammeGroup[];
		filter: KemFilterState;
	}
	let { schemes, categories, ranges, groups, filter = $bindable() }: Props = $props();

	const allSchemeNames = $derived(schemes.map((s) => s.scheme));

	const schemesByCategory = $derived(
		Object.fromEntries(categories.map((cat) => [cat, schemes.filter((s) => s.category === cat)]))
	);

	function patch(p: Partial<CommonFilter>) {
		filter = { ...filter, ...p };
	}

	function categoryState(cat: string): 'all' | 'some' | 'none' {
		const catSchemes = schemesByCategory[cat];
		const n = catSchemes.filter((s) => filter.schemes.has(s.scheme)).length;
		if (n === catSchemes.length) return 'all';
		if (n === 0) return 'none';
		return 'some';
	}

	function toggleCategory(cat: string) {
		const catSchemes = schemesByCategory[cat].map((s) => s.scheme);
		const next = new Set(filter.schemes);
		if (categoryState(cat) === 'all') catSchemes.forEach((s) => next.delete(s));
		else catSchemes.forEach((s) => next.add(s));
		filter = { ...filter, schemes: next };
	}

	function toggleScheme(scheme: string) {
		const next = new Set(filter.schemes);
		if (next.has(scheme)) next.delete(scheme);
		else next.add(scheme);
		filter = { ...filter, schemes: next };
	}

	function selectAll() {
		filter = { ...filter, schemes: new Set(allSchemeNames) };
	}

	function selectNone() {
		filter = { ...filter, schemes: new Set() };
	}

	function reset() {
		filter = { ...defaultKemFilter(ranges, allSchemeNames, groups), chart: filter.chart };
	}

	function indeterminate(el: HTMLInputElement, value: boolean) {
		el.indeterminate = value;
		return {
			update(v: boolean) {
				el.indeterminate = v;
			}
		};
	}
</script>

<aside class="rounded border border-pqs-ashgray bg-white p-4 text-sm shadow-sm dark:border-pqs-steel dark:bg-pqs-midnight-mid">
	<h2 class="mb-4 hidden font-heading text-base font-bold text-pqs-midnight dark:text-white lg:block">Filters</h2>

	<ProgrammeFilter {filter} {groups} onchange={patch} />

	<hr class="mb-4 border-pqs-ashgray dark:border-pqs-steel" />

	<section class="mb-5">
		<div class="mb-2 flex items-center justify-between">
			<h3 class={FILTER_HEADING}>Schemes</h3>
			<span class="space-x-2 font-heading text-xs">
				<button type="button" onclick={selectAll} class="text-pqs-apricot underline hover:no-underline">All</button>
				<button type="button" onclick={selectNone} class="text-pqs-apricot underline hover:no-underline">None</button>
			</span>
		</div>
		{#each categories as cat (cat)}
			<details class="mb-1" open>
				<summary class="flex cursor-pointer list-none items-center gap-1.5 py-0.5">
					<input
						type="checkbox"
						aria-label="All {cat} schemes"
						checked={categoryState(cat) !== 'none'}
						use:indeterminate={categoryState(cat) === 'some'}
						onchange={() => toggleCategory(cat)}
						class="shrink-0 accent-pqs-apricot"
					/>
					<span class="font-heading font-semibold text-pqs-midnight dark:text-pqs-smoke">{cat}</span>
				</summary>
				<div class="ml-5 mt-0.5 space-y-0.5">
					{#each schemesByCategory[cat] as scheme (scheme.scheme)}
						<label class="flex cursor-pointer items-center gap-1.5">
							<input
								type="checkbox"
								checked={filter.schemes.has(scheme.scheme)}
								onchange={() => toggleScheme(scheme.scheme)}
								class="shrink-0 accent-pqs-apricot"
							/>
							<span class="text-pqs-steel dark:text-pqs-bluegray">{scheme.scheme}</span>
						</label>
					{/each}
				</div>
			</details>
		{/each}
	</section>

	<hr class="mb-4 border-pqs-ashgray dark:border-pqs-steel" />

	<LevelFilter {filter} onchange={patch} />
	<ParamSetFilter {filter} onchange={patch} />

	<hr class="mb-4 border-pqs-ashgray dark:border-pqs-steel" />

	<section class="mb-5">
		<h3 class={FILTER_HEADING}>Sizes (bytes)</h3>
		<div class="space-y-2">
			<RangeField label="Min pk" value={filter.minPk} onchange={(v) => (filter = { ...filter, minPk: v })} />
			<RangeField label="Max pk" value={filter.maxPk} onchange={(v) => (filter = { ...filter, maxPk: v })} />
			<RangeField label="Min ct" value={filter.minCt} onchange={(v) => (filter = { ...filter, minCt: v })} />
			<RangeField label="Max ct" value={filter.maxCt} onchange={(v) => (filter = { ...filter, maxCt: v })} />
			<RangeField label="Min pk+ct" value={filter.minPkPlusCt} onchange={(v) => (filter = { ...filter, minPkPlusCt: v })} />
			<RangeField label="Max pk+ct" value={filter.maxPkPlusCt} onchange={(v) => (filter = { ...filter, maxPkPlusCt: v })} />
		</div>
	</section>

	{#if ranges.keygenUs || ranges.encapsUs || ranges.decapsUs}
		<section class="mb-5">
			<h3 class={FILTER_HEADING}>Performance (µs)</h3>
			<div class="space-y-2">
				{#if ranges.keygenUs}
					<RangeField label="Min keygen" value={filter.minKeygenUs} onchange={(v) => (filter = { ...filter, minKeygenUs: v })} />
					<RangeField label="Max keygen" value={filter.maxKeygenUs} onchange={(v) => (filter = { ...filter, maxKeygenUs: v })} />
				{/if}
				{#if ranges.encapsUs}
					<RangeField label="Min encaps" value={filter.minEncapsUs} onchange={(v) => (filter = { ...filter, minEncapsUs: v })} />
					<RangeField label="Max encaps" value={filter.maxEncapsUs} onchange={(v) => (filter = { ...filter, maxEncapsUs: v })} />
				{/if}
				{#if ranges.decapsUs}
					<RangeField label="Min decaps" value={filter.minDecapsUs} onchange={(v) => (filter = { ...filter, minDecapsUs: v })} />
					<RangeField label="Max decaps" value={filter.maxDecapsUs} onchange={(v) => (filter = { ...filter, maxDecapsUs: v })} />
				{/if}
			</div>
		</section>
	{/if}

	<hr class="mb-4 border-pqs-ashgray dark:border-pqs-steel" />

	<StatusFilter {filter} onchange={patch} />

	<button
		type="button"
		onclick={reset}
		class="h-10 w-full rounded border border-pqs-bluegray font-heading font-semibold text-pqs-steel hover:bg-pqs-smoke dark:border-pqs-steel dark:text-pqs-apricot dark:hover:bg-pqs-steel/30"
	>Reset filters</button>
</aside>
```

- [ ] **Step 6: Use one `FilterSidebar` on the signatures pages**

In `src/routes/signatures/+page.svelte` (re-read it first):
- add imports `import FilterSidebar from '$lib/components/FilterSidebar.svelte';` and `import { filterSummary, presentProgrammeGroups } from '$lib/compareFilters';`;
- after `let ranges = $state(_init.ranges);` add:

```ts
	let groups = $state(presentProgrammeGroups(_init.parameterSets));
	const { store } = getFilterStore();
```

- in `applyRound`, after `ranges = result.ranges;` add `groups = presentProgrammeGroups(result.parameterSets);`.

Replace the markup from `<div class="flex gap-8">` through the closing `</details>` of the mobile disclosure with:

```svelte
	<div class="flex flex-col gap-6 lg:flex-row lg:gap-8">
		<FilterSidebar summary={filterSummary($store, $store.schemes.size)}>
			<FilterPanel {schemes} {categories} {ranges} {groups} />
		</FilterSidebar>

		<!-- Main content -->
		<div class="min-w-0 flex-1 space-y-6">
```

In `src/routes/signatures/advanced/+page.svelte`, make the same change: the same two imports; `const groups = presentProgrammeGroups(_init.parameterSets);` after `let ranges = …`; `const { store } = getFilterStore();` after the existing `const { applyUrl } = getFilterStore();`. Then replace the markup from `<div class="flex gap-8">` through the closing `</details>` with:

```svelte
	<div class="flex flex-col gap-6 lg:flex-row lg:gap-8">
		<FilterSidebar summary={filterSummary($store, $store.schemes.size)}>
			<FilterPanel {schemes} {categories} {ranges} {groups} />
		</FilterSidebar>

		<!-- Main content -->
		<div class="min-w-0 flex-1 space-y-6">
```

- [ ] **Step 7: Rewrite `src/routes/kems/+page.svelte` and update the KEM advanced page**

`src/routes/kems/+page.svelte`:

```svelte
<script lang="ts">
	import { onMount } from 'svelte';
	import { goto } from '$app/navigation';
	import { base } from '$app/paths';
	import FilterSidebar from '$lib/components/FilterSidebar.svelte';
	import KemFilterPanel from '$lib/components/KemFilterPanel.svelte';
	import KemScatterPlot from '$lib/components/KemScatterPlot.svelte';
	import KemTable from '$lib/components/KemTable.svelte';
	import BenchmarkEnvInfo from '$lib/components/BenchmarkEnvInfo.svelte';
	import { filterSummary, presentProgrammeGroups } from '$lib/compareFilters';
	import {
		applyKemUrlParams,
		buildKemUrlParams,
		computeKemRanges,
		defaultKemFilter,
		filterKemRows,
		processKemSchemes
	} from '$lib/kemData';
	import { allKemData, kemBenchmarkEnv } from '$lib/kemSchemeData';
	import type { PageData } from './$types';

	// load (+page.ts) drives prerendering; data is read from the module glob here
	// (matching the signatures page) so the one-time filter init stays warning-free.
	let { data: _ }: { data: PageData } = $props();

	const { schemes, parameterSets } = processKemSchemes(allKemData);
	const ranges = computeKemRanges(parameterSets);
	const categories = [...new Set(schemes.map((s) => s.category))].sort();
	const groups = presentProgrammeGroups(parameterSets);

	const defaults = defaultKemFilter(ranges, schemes.map((s) => s.scheme), groups);
	let filter = $state(defaultKemFilter(ranges, schemes.map((s) => s.scheme), groups));

	const filteredRows = $derived(filterKemRows(parameterSets, filter));

	// URL state: filter params encoded as query params (same common codec as the signatures page).
	// Applied client-side in onMount (the page is prerendered); synced back, debounced.
	let urlReady = $state(false);

	onMount(() => {
		const params = new URLSearchParams(window.location.search);
		if (params.toString()) filter = applyKemUrlParams(defaults, params);
		urlReady = true;
	});

	$effect(() => {
		filter; // track filter changes
		if (!urlReady) return;
		const timer = setTimeout(() => {
			const qs = buildKemUrlParams(filter, defaults).toString();
			goto((qs ? `?${qs}` : window.location.pathname) + window.location.hash, {
				replaceState: true,
				keepFocus: true,
				noScroll: true
			});
		}, 300);
		return () => clearTimeout(timer);
	});
</script>

<div class="mx-auto max-w-screen-2xl px-6 py-8">
	<!-- Hero header -->
	<div class="mb-8 border-l-4 border-pqs-apricot pl-4">
		<h1 class="font-heading text-3xl font-bold text-pqs-midnight dark:text-white">
			Key Encapsulation Mechanisms
		</h1>
		<p class="mt-2 text-sm text-pqs-steel dark:text-pqs-bluegray">
			Standardised KEMs, competition candidates and research proposals, with pre-quantum ECDH for
			reference. Select column headers to sort; use the filters to narrow by programme, scheme,
			security level, size or speed.
		</p>
		<p class="mt-1.5 text-xs text-pqs-steel/70 dark:text-pqs-bluegray/70">
			Sizes reflect the latest known specifications (ML-KEM: FIPS 203; HQC: 2025-08-22 draft). For
			ECDH the “ciphertext” is the ephemeral public key and keygen/encaps/decaps map to the
			static/ephemeral key generation and the two ECDH derivations.
			See the <a href="{base}/signatures/" class="text-pqs-apricot hover:underline">signature comparison</a>
			for signature schemes.
		</p>
	</div>

	<div class="flex flex-col gap-6 lg:flex-row lg:gap-8">
		<FilterSidebar summary={filterSummary(filter, filter.schemes.size)}>
			<KemFilterPanel {schemes} {categories} {ranges} {groups} bind:filter />
		</FilterSidebar>

		<!-- Main content -->
		<div class="min-w-0 flex-1 space-y-6">
			<!-- Scatter plot -->
			<section class="rounded border border-pqs-ashgray bg-white p-4 shadow-sm dark:border-pqs-steel dark:bg-pqs-midnight-mid">
				<h2 class="mb-3 font-heading text-base font-semibold text-pqs-steel dark:text-pqs-apricot">
					pk size vs. ciphertext size <span class="font-normal text-pqs-bluegray">(log–log scale)</span>
				</h2>
				<KemScatterPlot rows={filteredRows} />
				<div class="mt-2 flex justify-end">
					<a href="{base}/kems/advanced/" class="text-xs text-pqs-bluegray hover:text-pqs-apricot dark:text-pqs-steel dark:hover:text-pqs-apricot transition-colors">
						Advanced graph →
					</a>
				</div>
			</section>

			<!-- Performance note -->
			{#if kemBenchmarkEnv}
				<div class="rounded border border-pqs-apricot/40 bg-pqs-apricot/10 px-4 py-3 text-xs text-pqs-midnight dark:border-pqs-apricot/30 dark:bg-pqs-apricot/5 dark:text-pqs-smoke">
					<strong class="font-heading font-semibold text-pqs-apricot">Performance note:</strong>
					Keygen / encapsulation / decapsulation timings are from our own benchmarks on a
					{kemBenchmarkEnv.cpu.model} (ML-KEM and HQC use AVX2; ECDH uses OpenSSL); see the
					environment details below. Schemes not yet in our harness show sizes only.
				</div>
			{/if}

			<!-- Table -->
			<section>
				<KemTable rows={filteredRows} recommendedOnly={filter.paramSets === 'recommended'} />
			</section>

			<!-- Benchmark environment -->
			{#if kemBenchmarkEnv}
				<BenchmarkEnvInfo env={kemBenchmarkEnv} />
			{/if}
		</div>
	</div>
</div>
```

In `src/routes/kems/advanced/+page.svelte`:
- add `import FilterSidebar from '$lib/components/FilterSidebar.svelte';` and extend the compareFilters import to `import { filterSummary, presentProgrammeGroups } from '$lib/compareFilters';`;
- replace the markup from `<div class="flex gap-8">` through the closing `</details>` with:

```svelte
	<div class="flex flex-col gap-6 lg:flex-row lg:gap-8">
		<FilterSidebar summary={filterSummary(filter, filter.schemes.size)}>
			<KemFilterPanel {schemes} {categories} {ranges} {groups} bind:filter />
		</FilterSidebar>

		<!-- Main content -->
		<div class="min-w-0 flex-1 space-y-6">
```

- in its URL-sync `goto(...)`, append `+ window.location.hash` to the URL expression, as on the main page.

- [ ] **Step 8: Run all tests to verify they pass**

Run: `npm run check && npm run test && npm run test:e2e`
Expected: PASS. Existing KEM tests still find the panel via `page.locator('aside').first()`; there is now exactly one `aside`.

- [ ] **Step 9: Commit**

```bash
git add src/lib/components/filterStyles.ts src/lib/components/FilterSidebar.svelte src/lib/components/ProgrammeFilter.svelte src/lib/components/LevelFilter.svelte src/lib/components/ParamSetFilter.svelte src/lib/components/StatusFilter.svelte src/lib/components/FilterPanel.svelte src/lib/components/KemFilterPanel.svelte src/routes/signatures/+page.svelte src/routes/signatures/advanced/+page.svelte src/routes/kems/+page.svelte src/routes/kems/advanced/+page.svelte e2e/signatures.spec.ts e2e/kems.spec.ts
git commit -m "$(cat <<'EOF'
Add programme, level, parameter-set and status filters to both pages

Shared filter sections, one filter column per page that is
collapsed by default on phones, and KEM µs range filters.

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
EOF
)"
```

---

### Task 8: On-ramp round nested under the programme filter

**Files:**
- Modify (rewrite): `src/lib/components/RoundSelect.svelte`
- Modify: `src/routes/signatures/+page.svelte`
- Test: `e2e/signatures.spec.ts`

**Interfaces:**
- Consumes: `ProgrammeFilter`'s `onramp` snippet and `FilterPanel`'s `onramp` prop (Task 7); `encodeCommonFilters` (Task 1).
- Produces: `RoundSelect { disabled?: boolean }`, labelled "On-ramp round" (visually hidden, as on the canvas). It sits under the "NIST on-ramp" checkbox and is disabled while that checkbox is unticked. On a round change the common filters and the sort are carried over; scheme and range filters reset to the new round's data.

- [ ] **Step 1: Write the failing e2e tests**

In `e2e/signatures.spec.ts`, replace the two existing tests `'dataset selector offers Latest and rounds 3–1'` and `'choosing Round 2 updates the URL'` with:

```ts
	test('on-ramp round selector offers Latest and rounds 3–1 under NIST on-ramp', async ({ page }) => {
		await page.goto('/signatures/');
		const select = page.getByLabel('On-ramp round');
		await expect(select).toBeVisible();
		await expect(select.locator('option')).toHaveText(['Latest specs', 'Round 3', 'Round 2', 'Round 1']);
		await expect(page.getByRole('group', { name: 'Programme' }).getByLabel('On-ramp round')).toBeVisible();
	});

	test('choosing Round 2 updates the URL', async ({ page }) => {
		await page.goto('/signatures/');
		await page.waitForLoadState('networkidle');
		await page.getByLabel('On-ramp round').selectOption('round-2');
		await page.waitForURL(/[?&]r=2/, { timeout: 15_000 });
	});

	test('changing the round keeps status and programme filters', async ({ page }) => {
		await page.goto('/signatures/?hw=1');
		await page.waitForLoadState('networkidle');
		await page.getByLabel('NIST standards').uncheck();
		await expect(page).toHaveURL(/[?&]p=/);
		await page.getByLabel('On-ramp round').selectOption('round-2');
		await page.waitForURL(/[?&]r=2/, { timeout: 15_000 });
		await expect(page).toHaveURL(/[?&]hw=1/);
		await expect(page).toHaveURL(/[?&]p=/);
		await expect(page.getByLabel('Hide warnings')).toBeChecked();
		await expect(page.getByLabel('NIST standards')).not.toBeChecked();
	});

	test('the round select is disabled while NIST on-ramp is unticked', async ({ page }) => {
		await page.goto('/signatures/');
		await page.waitForLoadState('networkidle');
		await page.getByLabel('NIST on-ramp').uncheck();
		await expect(page.getByLabel('On-ramp round')).toBeDisabled();
	});
```

- [ ] **Step 2: Run them to verify they fail**

Run: `npx playwright test e2e/signatures.spec.ts -g "round"`
Expected: FAIL (no "On-ramp round" label; the round change resets `hw`).

- [ ] **Step 3: Rewrite `src/lib/components/RoundSelect.svelte`**

```svelte
<script lang="ts">
	import { roundStore, type Round } from '$lib/roundStore';

	let { disabled = false }: { disabled?: boolean } = $props();

	const id = $props.id();

	const OPTIONS: { value: Round; label: string }[] = [
		{ value: 'latest', label: 'Latest specs' },
		{ value: 'round-3', label: 'Round 3' },
		{ value: 'round-2', label: 'Round 2' },
		{ value: 'round-1', label: 'Round 1' }
	];
</script>

<label for={id} class="sr-only">On-ramp round</label>
<select
	{id}
	{disabled}
	class="mb-1.5 mt-0.5 h-8 w-full rounded border border-pqs-bluegray bg-white px-2 text-sm text-pqs-midnight disabled:opacity-50 dark:border-pqs-steel dark:bg-pqs-midnight dark:text-pqs-smoke"
	value={$roundStore}
	onchange={(e) => roundStore.set(e.currentTarget.value as Round)}
>
	{#each OPTIONS as opt (opt.value)}
		<option value={opt.value}>{opt.label}</option>
	{/each}
</select>
```

- [ ] **Step 4: Nest the select and carry filters over in `src/routes/signatures/+page.svelte`**

Re-read the file first. Then:

1. Add `import { encodeCommonFilters } from '$lib/compareFilters';`. If the file already imports from `$lib/compareFilters`, merge it into that import.
2. Replace the round subscription

```ts
		const unsubRound = roundStore.subscribe((round) => {
			if (round === currentRound) return;
			currentRound = round;
			applyRound(round);
		});
```

with

```ts
		const unsubRound = roundStore.subscribe((round) => {
			if (round === currentRound) return;
			currentRound = round;
			// Keep the data-independent filters (programmes, levels, parameter sets, status,
			// chart) and the sort; scheme and range filters reset to the new round's data.
			const { store: s, defaults } = getFilterStore();
			const current = get(s);
			const carry = new URLSearchParams();
			encodeCommonFilters(current, defaults, carry);
			applyRound(round, carry);
			getFilterStore().store.update((f) => ({ ...f, sortCol: current.sortCol, sortDir: current.sortDir }));
		});
```

and update the comment above it to: `// Round changes after load keep the common filters (see below). subscribe() fires synchronously with the current value; the round-equality guard skips that call so it cannot wipe the URL-applied filters.`

3. In the hero, delete `<div class="mt-3"><RoundSelect /></div>`, and replace the first paragraph

```svelte
		<p class="mt-2 text-sm text-pqs-steel dark:text-pqs-bluegray">
			Comparing NIST on-ramp candidates and standardized schemes.
			Click column headers to sort. Use the filters to narrow down by category, security level, or size constraints.
		</p>
```

with

```svelte
		<p class="mt-2 text-sm text-pqs-steel dark:text-pqs-bluegray">
			Standardised schemes and candidates from the NIST on-ramp and other standardisation programmes,
			with pre-quantum schemes for reference. Select column headers to sort; use the filters to narrow
			by programme, scheme, security level, size or speed.
		</p>
```

4. Directly after the closing `</p>` of the round-dependent `<p class="mt-1.5 …">` paragraph (the one with the `{#if $roundStore === 'latest'}` chain), add a new paragraph:

```svelte
		<p class="mt-1 text-xs text-pqs-steel/70 dark:text-pqs-bluegray/70">
			The on-ramp round (under Programme in the filters) only selects on-ramp submissions; schemes from
			other programmes always show their latest specification.
		</p>
```

5. Add the snippet just before the `<div class="flex flex-col gap-6 lg:flex-row lg:gap-8">` line, and pass it to `FilterPanel`:

```svelte
	{#snippet onrampRound(enabled: boolean)}
		<RoundSelect disabled={!enabled} />
	{/snippet}
```

```svelte
			<FilterPanel {schemes} {categories} {ranges} {groups} onramp={onrampRound} />
```

- [ ] **Step 5: Run all tests to verify they pass**

Run: `npm run check && npm run test && npm run test:e2e`
Expected: PASS, including the other agent's shared-link tests (`?r=2&s=MAYO,UOV&sort=sig`).

- [ ] **Step 6: Commit**

```bash
git add src/lib/components/RoundSelect.svelte src/routes/signatures/+page.svelte e2e/signatures.spec.ts
git commit -m "$(cat <<'EOF'
Nest the on-ramp round under the programme filter

The round select sits under "NIST on-ramp" and is disabled while it
is unticked; changing round keeps the programme, level, parameter
set, status and chart filters.

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
EOF
)"
```

---

### Task 9: HTML chart legend with a separate NGCC colour scale

**Files:**
- Create: `src/lib/chartLegend.ts`, `src/lib/components/ChartLegend.svelte`
- Modify (rewrite): `src/lib/components/ScatterPlot.svelte`, `src/lib/components/KemScatterPlot.svelte`
- Test: `src/lib/__tests__/chartLegend.test.ts`, `e2e/signatures.spec.ts`

**Interfaces:**
- Produces (`chartLegend.ts`):
  - `SHAPES` (SVG paths in a unit square), `STANDARDISED = 'Standardised'`, `FAMILY_SHAPES: Record<string, string>` (legend order)
  - `familyKey(row: { status: string; category: string }): string`: returns `'Standardised'` for `FIPS` and `To be standardized`, the category when it has a shape, else `'Other'`.
  - `levelKey(level, ngccLevel): string`: `'Level 1'…'Level 5'`, `'Pre-quantum'`, `'128-bit'…'512-bit'`, `'No level'`.
  - `levelDescription(level, ngccLevel): string`, used in tooltips.
  - `levelColourScale(keys: Iterable<string>, dark: boolean): { domain: string[]; range: string[] }`: NIST keys first, then NGCC keys, present keys only.
  - `type Security = 'broken' | 'warning' | 'ok'`; `securityStatus(row): Security`; `RINGS`.
  - `interface LegendItem { label: string; colour?: string; shape?: string; ring?: string }`, `interface LegendGroup { title: string; items: LegendItem[]; note?: string }`
  - `interface ChartPoint { family: string; level: string; security: Security }`; `chartLegend(points: ChartPoint[], dark: boolean): LegendGroup[]`
- Produces: `ChartLegend { groups: LegendGroup[]; footnote?: string }` (`role="group"`, `aria-label="Legend"`). It is a side column (`lg:w-44`) on desktop and wraps below the plot on mobile.
- Colours: NIST light `#6B7280 #1D4ED8 #059669 #D97706 #EA580C #DC2626` (dark `#9CA3AF #60A5FA #34D399 #FBBF24 #F97316 #F87171`), as today. NGCC light `#A855F7 #7E22CE #6B21A8 #4C1D95` (dark `#D8B4FE #C084FC #A855F7 #9333EA`).

- [ ] **Step 1: Write the failing tests**

Create `src/lib/__tests__/chartLegend.test.ts`:

```ts
import { describe, expect, it } from 'vitest';
import { chartLegend, familyKey, FAMILY_SHAPES, levelColourScale, levelDescription, levelKey, STANDARDISED } from '$lib/chartLegend';

describe('levelKey', () => {
	it('names NIST levels and NGCC targets on their own scales', () => {
		expect(levelKey(3, null)).toBe('Level 3');
		expect(levelKey('Pre-Quantum', null)).toBe('Pre-quantum');
		expect(levelKey(null, 128)).toBe('128-bit');
		expect(levelKey(null, 128)).not.toMatch(/Level/);
	});
	it('describes levels for tooltips', () => {
		expect(levelDescription(1, null)).toBe('NIST level 1');
		expect(levelDescription(null, 256)).toBe('NGCC target: 256-bit classical');
	});
});

describe('levelColourScale', () => {
	it('keeps only present keys, NIST first', () => {
		expect(levelColourScale(['512-bit', 'Level 3', 'Level 1'], false)).toEqual({
			domain: ['Level 1', 'Level 3', '512-bit'],
			range: ['#1D4ED8', '#D97706', '#4C1D95']
		});
	});
	it('never gives an NGCC target a NIST level colour', () => {
		const nist = levelColourScale(['Pre-quantum', 'Level 1', 'Level 2', 'Level 3', 'Level 4', 'Level 5'], false).range;
		const ngcc = levelColourScale(['128-bit', '256-bit', '384-bit', '512-bit'], false).range;
		expect(ngcc.filter((c) => nist.includes(c))).toEqual([]);
		const nistDark = levelColourScale(['Level 1', 'Level 5'], true).range;
		const ngccDark = levelColourScale(['128-bit', '512-bit'], true).range;
		expect(ngccDark.filter((c) => nistDark.includes(c))).toEqual([]);
	});
});

describe('familyKey', () => {
	it('stars standards and pending standards', () => {
		expect(familyKey({ status: 'FIPS', category: 'Lattices' })).toBe(STANDARDISED);
		expect(familyKey({ status: 'To be standardized', category: 'Code-based' })).toBe(STANDARDISED);
	});
	it('maps unknown categories to Other', () => {
		expect(familyKey({ status: 'On-ramp', category: 'Weird' })).toBe('Other');
		expect(FAMILY_SHAPES[familyKey({ status: 'On-ramp', category: 'Lattices' })]).toBeDefined();
	});
});

describe('chartLegend', () => {
	it('adds an NGCC target group with its note only when NGCC points are shown', () => {
		const groups = chartLegend([
			{ family: 'Lattices', level: 'Level 1', security: 'ok' },
			{ family: 'Lattices', level: '128-bit', security: 'broken' }
		], false);
		expect(groups.map((g) => g.title)).toEqual(['Family', 'NIST category', 'NGCC target', 'Security']);
		const ngcc = groups.find((g) => g.title === 'NGCC target')!;
		expect(ngcc.items.map((i) => i.label)).toEqual(['128-bit classical']);
		expect(ngcc.note).toBe('Not comparable to NIST levels.');
		expect(groups.find((g) => g.title === 'Security')!.items.map((i) => i.label)).toEqual(['Broken by design']);
	});
	it('has no NGCC group without NGCC points', () => {
		const groups = chartLegend([{ family: STANDARDISED, level: 'Level 2', security: 'warning' }], false);
		expect(groups.map((g) => g.title)).toEqual(['Family', 'NIST category', 'Security']);
	});
	it('is empty for an empty chart', () => {
		expect(chartLegend([], true)).toEqual([]);
	});
});
```

Append to `e2e/signatures.spec.ts` (a new top-level block):

```ts
test.describe('Signatures chart legend', () => {
	async function waitForChart(page: import('@playwright/test').Page) {
		await page.waitForFunction(
			() => [...document.querySelectorAll('svg')].some((s) => s.querySelector('g')),
			{ timeout: 15_000 }
		);
	}

	test('sits beside the plot on desktop and below it on a phone', async ({ page }) => {
		await page.goto('/signatures/');
		await waitForChart(page);
		const legend = page.getByRole('group', { name: 'Legend' });
		await expect(legend).toContainText('Family');
		await expect(legend).toContainText('NIST category');
		const chart = page.locator('section svg').filter({ has: page.locator('g') }).first();
		const l = (await legend.boundingBox())!;
		const c = (await chart.boundingBox())!;
		expect(l.x).toBeGreaterThanOrEqual(c.x + c.width - 1);

		await page.setViewportSize({ width: 390, height: 844 });
		await expect
			.poll(async () => (await legend.boundingBox())!.y - ((await chart.boundingBox())!.y + (await chart.boundingBox())!.height))
			.toBeGreaterThanOrEqual(-1);
	});
});
```

- [ ] **Step 2: Run them to verify they fail**

Run: `npx vitest run src/lib/__tests__/chartLegend.test.ts`
Expected: FAIL with "Failed to resolve import "$lib/chartLegend"".

- [ ] **Step 3: Write `src/lib/chartLegend.ts`**

```ts
import type { NgccLevel, NistLevel } from './types';

/** SVG paths in [-1, 1]²: Vega-Lite shape values and HTML legend symbols use the same strings. */
export const SHAPES = {
	star: 'M0,-1L0.224,-0.309L0.951,-0.309L0.363,0.118L0.588,0.809L0,0.382L-0.588,0.809L-0.363,0.118L-0.951,-0.309L-0.224,-0.309Z',
	square: 'M-0.75,-0.75H0.75V0.75H-0.75Z',
	plus: 'M-0.3,-1H0.3V-0.3H1V0.3H0.3V1H-0.3V0.3H-1V-0.3H-0.3Z',
	diamond: 'M0,-1L0.9,0L0,1L-0.9,0Z',
	triangleUp: 'M0,-0.9L0.9,0.7L-0.9,0.7Z',
	triangleDown: 'M0,0.9L0.9,-0.7L-0.9,-0.7Z',
	circle: 'M0.8,0A0.8,0.8 0 1,1 -0.8,0A0.8,0.8 0 1,1 0.8,0Z',
	triangleLeft: 'M-0.9,0L0.7,-0.9L0.7,0.9Z',
	triangleRight: 'M0.9,0L-0.7,-0.9L-0.7,0.9Z'
} as const;

export const STANDARDISED = 'Standardised';

/** Family → shape, in legend order. Standardised schemes get a star whatever their family. */
export const FAMILY_SHAPES: Record<string, string> = {
	[STANDARDISED]: SHAPES.star,
	Lattices: SHAPES.square,
	'Code-based': SHAPES.plus,
	MPCitH: SHAPES.diamond,
	Multivariate: SHAPES.triangleUp,
	Symmetric: SHAPES.triangleDown,
	Isogenies: SHAPES.circle,
	'Pre-Quantum': SHAPES.triangleLeft,
	Other: SHAPES.triangleRight
};

export function familyKey(row: { status: string; category: string }): string {
	if (row.status === 'FIPS' || row.status === 'To be standardized') return STANDARDISED;
	return row.category in FAMILY_SHAPES ? row.category : 'Other';
}

const NIST_KEYS = ['Pre-quantum', 'Level 1', 'Level 2', 'Level 3', 'Level 4', 'Level 5'] as const;
const NIST_COLOURS = {
	light: ['#6B7280', '#1D4ED8', '#059669', '#D97706', '#EA580C', '#DC2626'],
	dark: ['#9CA3AF', '#60A5FA', '#34D399', '#FBBF24', '#F97316', '#F87171']
};
const NGCC_KEYS = ['128-bit', '256-bit', '384-bit', '512-bit'] as const;
const NGCC_COLOURS = {
	light: ['#A855F7', '#7E22CE', '#6B21A8', '#4C1D95'],
	dark: ['#D8B4FE', '#C084FC', '#A855F7', '#9333EA']
};

/** Colour key of a row: its level on its own scale. NGCC targets never become NIST levels. */
export function levelKey(level: NistLevel | null, ngccLevel: NgccLevel | null): string {
	if (level === 'Pre-Quantum') return 'Pre-quantum';
	if (level !== null) return `Level ${level}`;
	if (ngccLevel !== null) return `${ngccLevel}-bit`;
	return 'No level';
}

export function levelDescription(level: NistLevel | null, ngccLevel: NgccLevel | null): string {
	if (level === 'Pre-Quantum') return 'Pre-quantum';
	if (level !== null) return `NIST level ${level}`;
	if (ngccLevel !== null) return `NGCC target: ${ngccLevel}-bit classical`;
	return 'Not stated';
}

export interface ColourScale {
	domain: string[];
	range: string[];
}

export function levelColourScale(keys: Iterable<string>, dark: boolean): ColourScale {
	const present = new Set(keys);
	const scale: ColourScale = { domain: [], range: [] };
	const add = (ks: readonly string[], colours: string[]) =>
		ks.forEach((k, i) => {
			if (present.has(k)) {
				scale.domain.push(k);
				scale.range.push(colours[i]);
			}
		});
	add(NIST_KEYS, dark ? NIST_COLOURS.dark : NIST_COLOURS.light);
	add(NGCC_KEYS, dark ? NGCC_COLOURS.dark : NGCC_COLOURS.light);
	return scale;
}

export type Security = 'broken' | 'warning' | 'ok';

export function securityStatus(row: { broken: false | string; classical: boolean; warning: false | string }): Security {
	if (row.broken && !row.classical) return 'broken';
	if (row.warning) return 'warning';
	return 'ok';
}

/** Ring colours of the broken/warning overlay layers. */
export const RINGS = {
	broken: { light: '#B91C1C', dark: '#FCA5A5' },
	warning: { light: '#B45309', dark: '#FDE68A' }
} as const;

export interface LegendItem {
	label: string;
	colour?: string;
	shape?: string;
	ring?: string;
}

export interface LegendGroup {
	title: string;
	items: LegendItem[];
	note?: string;
}

export interface ChartPoint {
	family: string;
	level: string;
	security: Security;
}

/** Legend groups for the points actually drawn: Family, NIST category, NGCC target, Security. */
export function chartLegend(points: ChartPoint[], dark: boolean): LegendGroup[] {
	const families = new Set(points.map((p) => p.family));
	const levels = new Set(points.map((p) => p.level));
	const scale = levelColourScale(levels, dark);
	const colourOf = (k: string) => scale.range[scale.domain.indexOf(k)];
	const groups: LegendGroup[] = [];

	const family = Object.keys(FAMILY_SHAPES)
		.filter((f) => families.has(f))
		.map((f) => ({ label: f === 'Pre-Quantum' ? 'Pre-quantum' : f, shape: FAMILY_SHAPES[f] }));
	if (family.length) groups.push({ title: 'Family', items: family });

	const nist = NIST_KEYS.filter((k) => levels.has(k)).map((k) => ({ label: k, colour: colourOf(k) }));
	if (nist.length) groups.push({ title: 'NIST category', items: nist });

	const ngcc = NGCC_KEYS.filter((k) => levels.has(k)).map((k) => ({ label: `${k} classical`, colour: colourOf(k) }));
	if (ngcc.length) groups.push({ title: 'NGCC target', items: ngcc, note: 'Not comparable to NIST levels.' });

	const tone = dark ? 'dark' : 'light';
	const security: LegendItem[] = [];
	if (points.some((p) => p.security === 'broken')) security.push({ label: 'Broken by design', ring: RINGS.broken[tone] });
	if (points.some((p) => p.security === 'warning')) security.push({ label: 'Warning', ring: RINGS.warning[tone] });
	if (security.length) groups.push({ title: 'Security', items: security });

	return groups;
}
```

- [ ] **Step 4: Write `src/lib/components/ChartLegend.svelte`**

```svelte
<script lang="ts">
	import type { LegendGroup } from '$lib/chartLegend';

	let { groups, footnote }: { groups: LegendGroup[]; footnote?: string } = $props();
</script>

<div
	role="group"
	aria-label="Legend"
	class="flex flex-wrap gap-x-8 gap-y-3 text-xs text-pqs-midnight dark:text-pqs-smoke lg:w-44 lg:shrink-0 lg:flex-col lg:flex-nowrap lg:gap-5"
>
	{#each groups as group (group.title)}
		<div>
			<h3 class="mb-1.5 font-heading text-xs font-bold">{group.title}</h3>
			<ul class="flex flex-wrap gap-x-3.5 gap-y-1.5 lg:flex-col">
				{#each group.items as item (item.label)}
					<li class="flex items-center gap-1.5">
						{#if item.shape}
							<svg width="12" height="12" viewBox="-1.1 -1.1 2.2 2.2" aria-hidden="true">
								<path d={item.shape} class="fill-pqs-steel dark:fill-pqs-bluegray" />
							</svg>
						{:else if item.colour}
							<span class="inline-block h-2.5 w-2.5 rounded-sm" style:background={item.colour} aria-hidden="true"></span>
						{:else if item.ring}
							<span class="inline-block h-3 w-3 rounded-full border-2" style:border-color={item.ring} aria-hidden="true"></span>
						{/if}
						{item.label}
					</li>
				{/each}
			</ul>
			{#if group.note}
				<p class="mt-1 text-pqs-steel dark:text-pqs-bluegray">{group.note}</p>
			{/if}
		</div>
	{/each}
	{#if footnote}
		<p class="text-pqs-steel dark:text-pqs-bluegray">{footnote}</p>
	{/if}
</div>
```

- [ ] **Step 5: Rewrite `src/lib/components/ScatterPlot.svelte`**

```svelte
<script lang="ts">
	import { onMount } from 'svelte';
	import { getFilterStore } from '$lib/filterStore';
	import { themeStore } from '$lib/themeStore';
	import { stripHtml } from '$lib/html';
	import {
		chartLegend,
		familyKey,
		FAMILY_SHAPES,
		levelColourScale,
		levelDescription,
		levelKey,
		RINGS,
		securityStatus,
		STANDARDISED,
		type LegendGroup
	} from '$lib/chartLegend';
	import type { AxisField, ScaleType } from '$lib/types';
	import ChartLegend from './ChartLegend.svelte';

	let {
		xField = 'pk' as AxisField,
		yField = 'sig' as AxisField,
		xScale = 'log' as ScaleType,
		yScale = 'log' as ScaleType,
	}: { xField?: AxisField; yField?: AxisField; xScale?: ScaleType; yScale?: ScaleType } = $props();

	const AXIS_TITLES: Record<AxisField, string> = {
		pk: 'Public key (bytes)',
		sig: 'Signature (bytes)',
		pkPlusSig: 'pk + sig (bytes)',
		signingCycles: 'Signing (cycles)',
		verificationCycles: 'Verification (cycles)',
		signingUs: 'Signing (µs)',
		verificationUs: 'Verification (µs)',
	};

	const { filteredRows } = getFilterStore();

	let container = $state<HTMLDivElement | null>(null);
	let viewHandle: { finalize(): void } | null = null;
	let renderSeq = 0;
	let renderQueue: Promise<void> = Promise.resolve();
	let renderError = $state(false);
	let legend = $state<LegendGroup[]>([]);

	// Serialize renders: concurrent vega-embed calls on the same container can
	// leave a stale, finalized chart in the DOM when they resolve out of order.
	function render() {
		const seq = ++renderSeq;
		renderQueue = renderQueue.then(async () => {
			try {
				await doRender(seq);
				renderError = false;
			} catch (err) {
				console.error('scatter plot render failed', err);
				renderError = true;
			}
		});
	}

	async function doRender(seq: number) {
		if (!container || seq !== renderSeq) return;

		const { default: embed } = await import('vega-embed');
		const data = $filteredRows;
		const isDark = document.documentElement.classList.contains('dark');

		const values = data.map((d) => {
			const notes = [
				d.broken && !d.classical ? `⚠ ${stripHtml(d.broken)}` : null,
				d.warning ? `⚠ ${stripHtml(d.warning)}` : null
			]
				.filter(Boolean)
				.join(' | ');

			const tooltip: Record<string, string | number> = {
				Scheme: d.scheme,
				'Parameter set': d.parameterset,
				...(d.version ? { Version: d.version } : {}),
				Family: d.category,
				'Security level': levelDescription(d.level, d.ngccLevel),
				'pk (bytes)': d.pk.toLocaleString(),
				'sig (bytes)': d.sig.toLocaleString(),
				'pk+sig (bytes)': d.pkPlusSig.toLocaleString(),
				...(d.signingCycles > 0 ? { 'Signing (cycles)': d.signingCycles.toLocaleString() } : {}),
				...(d.verificationCycles > 0 ? { 'Verification (cycles)': d.verificationCycles.toLocaleString() } : {}),
				...(d.signingUs != null ? { 'Signing (µs)': d.signingUs.toLocaleString() } : {}),
				...(d.verificationUs != null ? { 'Verification (µs)': d.verificationUs.toLocaleString() } : {}),
				...(notes ? { Notes: notes } : {})
			};

			return {
				scheme: d.scheme,
				parameterset: d.parameterset,
				pk: d.pk,
				sig: d.sig,
				pkPlusSig: d.pkPlusSig,
				signingCycles: d.signingCycles > 0 ? d.signingCycles : null,
				verificationCycles: d.verificationCycles > 0 ? d.verificationCycles : null,
				signingUs: d.signingUs,
				verificationUs: d.verificationUs,
				family: familyKey(d),
				level: levelKey(d.level, d.ngccLevel),
				security: securityStatus(d),
				tooltip
			};
		});

		const colour = levelColourScale(values.map((v) => v.level), isDark);
		const families = Object.keys(FAMILY_SHAPES).filter((f) => values.some((v) => v.family === f));
		legend = chartLegend(values, isDark);
		const tone = isDark ? 'dark' : 'light';

		// eslint-disable-next-line @typescript-eslint/no-explicit-any
		const spec: any = {
			$schema: 'https://vega.github.io/schema/vega-lite/v6.json',
			width: 'container',
			height: 350,
			autosize: { type: 'fit-x', resize: true },
			data: { values },
			resolve: { scale: { x: 'shared', y: 'shared', color: 'independent', shape: 'independent' } },
			layer: [
				{
					params: [
						{
							name: 'grid',
							select: { type: 'interval', encodings: ['x', 'y'] },
							bind: 'scales'
						}
					],
					mark: { type: 'point', filled: true, size: 100 },
					encoding: {
						size: {
							condition: [
								{ test: `datum.family === '${STANDARDISED}'`, value: 180 },
								{ test: "datum.family === 'Lattices'", value: 70 }
							],
							value: 100
						},
						color: { field: 'level', type: 'nominal', scale: colour, legend: null },
						shape: {
							field: 'family',
							type: 'nominal',
							scale: { domain: families, range: families.map((f) => FAMILY_SHAPES[f]) },
							legend: null
						}
					}
				},
				{
					transform: [{ filter: "datum.security === 'broken'" }],
					mark: { type: 'point', filled: false, shape: 'circle', size: 120, strokeWidth: 2 },
					encoding: { color: { value: RINGS.broken[tone] } }
				},
				{
					transform: [{ filter: "datum.security === 'warning'" }],
					mark: { type: 'point', filled: false, shape: 'circle', size: 120, strokeWidth: 2 },
					encoding: { color: { value: RINGS.warning[tone] } }
				}
			],
			encoding: {
				x: {
					field: xField,
					type: 'quantitative',
					scale: { type: xScale },
					axis: { title: AXIS_TITLES[xField], grid: true, format: 's' }
				},
				y: {
					field: yField,
					type: 'quantitative',
					scale: { type: yScale },
					axis: { title: AXIS_TITLES[yField], grid: true, format: 's' }
				},
				tooltip: { field: 'tooltip', type: 'nominal' }
			},
			config: {
				background: 'transparent',
				axis: {
					gridColor: isDark ? '#334155' : '#E2E8F0',
					tickColor: isDark ? '#475569' : '#94A3B8',
					labelColor: isDark ? '#94A3B8' : '#475569',
					titleColor: isDark ? '#C5CADA' : '#061128',
					domainColor: isDark ? '#475569' : '#94A3B8'
				},
				view: { stroke: 'transparent' }
			}
		};

		viewHandle?.finalize();
		viewHandle = null;
		viewHandle = await embed(container, spec, { actions: false, renderer: 'svg' });
	}

	onMount(() => {
		return () => viewHandle?.finalize();
	});

	$effect(() => {
		$filteredRows;
		$themeStore;
		xField; yField; xScale; yScale;
		render();
	});
</script>

<div class="flex flex-col gap-4 lg:flex-row lg:items-start">
	<div class="min-w-0 flex-1">
		<div bind:this={container} class="w-full"></div>
		{#if renderError}
			<p class="mt-1 text-xs text-pqs-scarlet" role="alert">Chart failed to render.</p>
		{/if}
		<div class="mt-1 flex items-center justify-between">
			<p class="text-xs text-pqs-bluegray dark:text-pqs-steel">Scroll to zoom · drag to pan<span class="sm:hidden"> · tap a point for details</span></p>
			<button
				type="button"
				onclick={render}
				class="text-xs text-pqs-bluegray hover:text-pqs-apricot dark:text-pqs-steel dark:hover:text-pqs-apricot transition-colors"
			>Reset zoom</button>
		</div>
	</div>
	<ChartLegend groups={legend} />
</div>
```

- [ ] **Step 6: Rewrite `src/lib/components/KemScatterPlot.svelte`**

```svelte
<script lang="ts">
	import { onMount } from 'svelte';
	import { themeStore } from '$lib/themeStore';
	import { stripHtml } from '$lib/html';
	import {
		chartLegend,
		familyKey,
		FAMILY_SHAPES,
		levelColourScale,
		levelDescription,
		levelKey,
		RINGS,
		securityStatus,
		STANDARDISED,
		type LegendGroup
	} from '$lib/chartLegend';
	import type { KemAxisField, KemParameterSet, ScaleType } from '$lib/types';
	import ChartLegend from './ChartLegend.svelte';

	let {
		rows,
		xField = 'pk' as KemAxisField,
		yField = 'ct' as KemAxisField,
		xScale = 'log' as ScaleType,
		yScale = 'log' as ScaleType,
	}: {
		rows: KemParameterSet[];
		xField?: KemAxisField;
		yField?: KemAxisField;
		xScale?: ScaleType;
		yScale?: ScaleType;
	} = $props();

	const AXIS_TITLES: Record<KemAxisField, string> = {
		pk: 'Public key (bytes)',
		ct: 'Ciphertext (bytes)',
		pkPlusCt: 'pk + ct (bytes)',
		keygenCycles: 'Keygen (cycles)',
		encapsCycles: 'Encapsulation (cycles)',
		decapsCycles: 'Decapsulation (cycles)',
		keygenUs: 'Keygen (µs)',
		encapsUs: 'Encapsulation (µs)',
		decapsUs: 'Decapsulation (µs)',
	};

	let container = $state<HTMLDivElement | null>(null);
	let viewHandle: { finalize(): void } | null = null;
	let renderSeq = 0;
	let renderQueue: Promise<void> = Promise.resolve();
	let renderError = $state(false);
	let legend = $state<LegendGroup[]>([]);

	// Serialize renders: concurrent vega-embed calls on the same container can
	// leave a stale, finalized chart in the DOM when they resolve out of order.
	function render() {
		const seq = ++renderSeq;
		renderQueue = renderQueue.then(async () => {
			try {
				await doRender(seq);
				renderError = false;
			} catch (err) {
				console.error('scatter plot render failed', err);
				renderError = true;
			}
		});
	}

	async function doRender(seq: number) {
		if (!container || seq !== renderSeq) return;

		const { default: embed } = await import('vega-embed');
		const data = rows;
		const isDark = document.documentElement.classList.contains('dark');

		const values = data.map((d) => {
			const notes = [
				d.broken && !d.classical ? `⚠ ${stripHtml(d.broken)}` : null,
				d.warning ? `⚠ ${stripHtml(d.warning)}` : null,
				d.notes ?? null
			]
				.filter(Boolean)
				.join(' | ');

			const tooltip: Record<string, string | number> = {
				Scheme: d.scheme,
				'Parameter set': d.parameterset,
				Family: d.category,
				'Security level': levelDescription(d.level, d.ngccLevel),
				'pk (bytes)': d.pk.toLocaleString(),
				'ct (bytes)': d.ct.toLocaleString(),
				'pk+ct (bytes)': d.pkPlusCt.toLocaleString(),
				...(d.keygenCycles > 0 ? { 'Keygen (cycles)': d.keygenCycles.toLocaleString() } : {}),
				...(d.encapsCycles > 0 ? { 'Encapsulation (cycles)': d.encapsCycles.toLocaleString() } : {}),
				...(d.decapsCycles > 0 ? { 'Decapsulation (cycles)': d.decapsCycles.toLocaleString() } : {}),
				...(d.keygenUs != null ? { 'Keygen (µs)': d.keygenUs.toLocaleString() } : {}),
				...(d.encapsUs != null ? { 'Encapsulation (µs)': d.encapsUs.toLocaleString() } : {}),
				...(d.decapsUs != null ? { 'Decapsulation (µs)': d.decapsUs.toLocaleString() } : {}),
				...(notes ? { Notes: notes } : {})
			};

			return {
				scheme: d.scheme,
				parameterset: d.parameterset,
				pk: d.pk,
				ct: d.ct,
				pkPlusCt: d.pkPlusCt,
				keygenCycles: d.keygenCycles > 0 ? d.keygenCycles : null,
				encapsCycles: d.encapsCycles > 0 ? d.encapsCycles : null,
				decapsCycles: d.decapsCycles > 0 ? d.decapsCycles : null,
				keygenUs: d.keygenUs,
				encapsUs: d.encapsUs,
				decapsUs: d.decapsUs,
				family: familyKey(d),
				level: levelKey(d.level, d.ngccLevel),
				security: securityStatus(d),
				tooltip
			};
		});

		const colour = levelColourScale(values.map((v) => v.level), isDark);
		const families = Object.keys(FAMILY_SHAPES).filter((f) => values.some((v) => v.family === f));
		legend = chartLegend(values, isDark);
		const tone = isDark ? 'dark' : 'light';

		// eslint-disable-next-line @typescript-eslint/no-explicit-any
		const spec: any = {
			$schema: 'https://vega.github.io/schema/vega-lite/v6.json',
			width: 'container',
			height: 350,
			autosize: { type: 'fit-x', resize: true },
			data: { values },
			resolve: { scale: { x: 'shared', y: 'shared', color: 'independent', shape: 'independent' } },
			layer: [
				{
					params: [
						{
							name: 'grid',
							select: { type: 'interval', encodings: ['x', 'y'] },
							bind: 'scales'
						}
					],
					mark: { type: 'point', filled: true, size: 120 },
					encoding: {
						size: {
							condition: { test: `datum.family === '${STANDARDISED}'`, value: 200 },
							value: 120
						},
						color: { field: 'level', type: 'nominal', scale: colour, legend: null },
						shape: {
							field: 'family',
							type: 'nominal',
							scale: { domain: families, range: families.map((f) => FAMILY_SHAPES[f]) },
							legend: null
						}
					}
				},
				{
					transform: [{ filter: "datum.security === 'broken'" }],
					mark: { type: 'point', filled: false, shape: 'circle', size: 140, strokeWidth: 2 },
					encoding: { color: { value: RINGS.broken[tone] } }
				},
				{
					transform: [{ filter: "datum.security === 'warning'" }],
					mark: { type: 'point', filled: false, shape: 'circle', size: 140, strokeWidth: 2 },
					encoding: { color: { value: RINGS.warning[tone] } }
				}
			],
			encoding: {
				x: {
					field: xField,
					type: 'quantitative',
					scale: { type: xScale },
					axis: { title: AXIS_TITLES[xField], grid: true, format: 's' }
				},
				y: {
					field: yField,
					type: 'quantitative',
					scale: { type: yScale },
					axis: { title: AXIS_TITLES[yField], grid: true, format: 's' }
				},
				tooltip: { field: 'tooltip', type: 'nominal' }
			},
			config: {
				background: 'transparent',
				axis: {
					gridColor: isDark ? '#334155' : '#E2E8F0',
					tickColor: isDark ? '#475569' : '#94A3B8',
					labelColor: isDark ? '#94A3B8' : '#475569',
					titleColor: isDark ? '#C5CADA' : '#061128',
					domainColor: isDark ? '#475569' : '#94A3B8'
				},
				view: { stroke: 'transparent' }
			}
		};

		viewHandle?.finalize();
		viewHandle = null;
		viewHandle = await embed(container, spec, { actions: false, renderer: 'svg' });
	}

	onMount(() => {
		return () => viewHandle?.finalize();
	});

	$effect(() => {
		rows;
		$themeStore;
		xField; yField; xScale; yScale;
		render();
	});
</script>

<div class="flex flex-col gap-4 lg:flex-row lg:items-start">
	<div class="min-w-0 flex-1">
		<div bind:this={container} class="w-full"></div>
		{#if renderError}
			<p class="mt-1 text-xs text-pqs-scarlet" role="alert">Chart failed to render.</p>
		{/if}
		<div class="mt-1 flex items-center justify-between">
			<p class="text-xs text-pqs-bluegray dark:text-pqs-steel">Scroll to zoom · drag to pan<span class="sm:hidden"> · tap a point for details</span></p>
			<button
				type="button"
				onclick={render}
				class="text-xs text-pqs-bluegray hover:text-pqs-apricot dark:text-pqs-steel dark:hover:text-pqs-apricot transition-colors"
			>Reset zoom</button>
		</div>
	</div>
	<ChartLegend groups={legend} />
</div>
```

(KEMs now star `To be standardized` as well as `FIPS`, matching the canvas's "Standardised" family; this affects HQC.)

- [ ] **Step 7: Run all tests to verify they pass**

Run: `npm run check && npm run test && npm run test:e2e`
Expected: PASS.

- [ ] **Step 8: Commit**

```bash
git add src/lib/chartLegend.ts src/lib/components/ChartLegend.svelte src/lib/components/ScatterPlot.svelte src/lib/components/KemScatterPlot.svelte src/lib/__tests__/chartLegend.test.ts e2e/signatures.spec.ts
git commit -m "$(cat <<'EOF'
Move chart legends into an HTML side column with an NGCC scale

NGCC targets get their own purple scale and legend group, marked
as not comparable to NIST levels; the legend wraps below the plot
on phones.

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
EOF
)"
```

---

### Task 10: Sizes / Speed chart toggle

**Files:**
- Create: `src/lib/chartData.ts`, `src/lib/components/ChartModeToggle.svelte`
- Modify: `src/lib/types.ts` (`AxisField`, `KemAxisField`), `src/lib/components/ScatterPlot.svelte`, `src/lib/components/KemScatterPlot.svelte`, `src/routes/signatures/+page.svelte`, `src/routes/kems/+page.svelte`, `src/routes/signatures/advanced/+page.svelte`, `src/routes/kems/advanced/+page.svelte`
- Test: `src/lib/__tests__/chartData.test.ts`, `e2e/signatures.spec.ts`, `e2e/kems.spec.ts`

**Interfaces:**
- Consumes: `sigTimes`, `kemTimes` (Task 4); `CommonFilter.chart` (Task 1); scatter plots and `ChartLegend` `footnote` (Task 9).
- Produces:
  - `AxisField` gains `'signingTime' | 'verificationTime'`, and `KemAxisField` gains `'keygenTime' | 'encapsTime' | 'decapsTime'`. These are displayed µs, measured or extrapolated.
  - `chartData.ts`: `sigAxisValue(row: ParameterSet, field: AxisField): number | null`, `kemAxisValue(row: KemParameterSet, field: KemAxisField): number | null` (cycles of 0 count as missing), `partitionPlottable<T, F>(rows: T[], valueOf: (row: T, field: F) => number | null, x: F, y: F): { plotted: T[]; omitted: T[] }`, and `omittedNote(n: number): string`.
  - `ChartModeToggle { mode: ChartMode; onchange: (mode: ChartMode) => void }`: a `role="group"` named "Chart axes" with two `aria-pressed` buttons, "Sizes" and "Speed".
  - Main pages: Speed means signing vs. verification time (signatures) or encapsulation vs. decapsulation time (KEMs), log–log. Rows without both values are left out, and a visible note says how many.

- [ ] **Step 1: Write the failing tests**

Create `src/lib/__tests__/chartData.test.ts`:

```ts
import { describe, expect, it } from 'vitest';
import { kemAxisValue, omittedNote, partitionPlottable, sigAxisValue } from '$lib/chartData';
import { processYamlSchemes } from '$lib/data';
import { processKemSchemes } from '$lib/kemData';
import { CYCLES_PER_US } from '$lib/constants';
import type { SchemeYaml } from '$lib/types';

const SIGS: SchemeYaml[] = [
	{ name: 'Fast', website: '', category: 'Lattices', assumption: '', versions: [{ version: 'v', date: '2025-01-01', status: 'On-ramp', parametersets: [{ name: 'f', level: 1, pk: 1, sig: 2, signing_cycles: 2500, verification_cycles: 5000 }] }] },
	{ name: 'Sized', website: '', category: 'Lattices', assumption: '', versions: [{ version: 'v', date: '2026-09-20', status: 'NGCC', parametersets: [{ name: 's', ngcc_level: 128, pk: 3, sig: 4 }] }] }
];
const rows = processYamlSchemes(SIGS).parameterSets;

describe('sigAxisValue', () => {
	it('returns sizes, and displayed times extrapolated from cycles', () => {
		expect(sigAxisValue(rows[0], 'pk')).toBe(1);
		expect(sigAxisValue(rows[0], 'signingTime')).toBe(2500 / CYCLES_PER_US);
		expect(sigAxisValue(rows[0], 'verificationTime')).toBe(5000 / CYCLES_PER_US);
	});
	it('treats a missing benchmark as no value', () => {
		expect(sigAxisValue(rows[1], 'signingTime')).toBeNull();
		expect(sigAxisValue(rows[1], 'signingCycles')).toBeNull();
	});
});

describe('kemAxisValue', () => {
	it('uses measured µs, else cycles', () => {
		const [k] = processKemSchemes([{ name: 'K', website: '', category: 'Lattices', assumption: '', status: 'x',
			parametersets: [{ name: 'k', level: 1, pk: 1, ct: 1, encaps_us: 7, decaps_cycles: 2500 }] }]).parameterSets;
		expect(kemAxisValue(k, 'encapsTime')).toBe(7);
		expect(kemAxisValue(k, 'decapsTime')).toBe(1);
		expect(kemAxisValue(k, 'keygenTime')).toBeNull();
		expect(kemAxisValue(k, 'ct')).toBe(1);
	});
});

describe('partitionPlottable', () => {
	it('leaves out rows without a value on either axis in the Speed view', () => {
		const { plotted, omitted } = partitionPlottable(rows, sigAxisValue, 'signingTime', 'verificationTime');
		expect(plotted.map((r) => r.scheme)).toEqual(['Fast']);
		expect(omitted.map((r) => r.scheme)).toEqual(['Sized']);
	});
	it('plots every row in the Sizes view', () => {
		expect(partitionPlottable(rows, sigAxisValue, 'pk', 'sig').omitted).toEqual([]);
	});
	it('copes with nothing plottable', () => {
		const { plotted, omitted } = partitionPlottable([rows[1]], sigAxisValue, 'signingTime', 'verificationTime');
		expect(plotted).toEqual([]);
		expect(omitted).toHaveLength(1);
	});
});

describe('omittedNote', () => {
	it('says how many parameter sets are missing and why', () => {
		expect(omittedNote(1)).toBe('1 parameter set is not shown: not yet benchmarked.');
		expect(omittedNote(4)).toBe('4 parameter sets are not shown: not yet benchmarked.');
	});
});
```

Append inside `test.describe('Signatures page', …)` in `e2e/signatures.spec.ts`:

```ts
	test('Speed toggle switches the chart to signing vs. verification time', async ({ page }) => {
		await page.goto('/signatures/');
		await page.waitForLoadState('networkidle');
		const axes = page.getByRole('group', { name: 'Chart axes' });
		await expect(axes.getByRole('button', { name: 'Sizes' })).toHaveAttribute('aria-pressed', 'true');
		await axes.getByRole('button', { name: 'Speed' }).click();
		await expect(axes.getByRole('button', { name: 'Speed' })).toHaveAttribute('aria-pressed', 'true');
		await expect(page.locator('section h2').first()).toContainText('Signing vs. verification time');
		await expect(page).toHaveURL(/[?&]v=speed/);
		await page.reload();
		await expect(page.locator('section h2').first()).toContainText('Signing vs. verification time');
	});
```

Append inside `test.describe('KEMs page', …)` in `e2e/kems.spec.ts`:

```ts
	test('Speed toggle switches the chart to encapsulation vs. decapsulation time', async ({ page }) => {
		await page.goto('/kems/');
		await page.waitForLoadState('networkidle');
		await page.getByRole('group', { name: 'Chart axes' }).getByRole('button', { name: 'Speed' }).click();
		await expect(page.locator('section h2').first()).toContainText('Encapsulation vs. decapsulation time');
		await expect(page).toHaveURL(/[?&]v=speed/);
	});
```

- [ ] **Step 2: Run them to verify they fail**

Run: `npx vitest run src/lib/__tests__/chartData.test.ts`
Expected: FAIL with "Failed to resolve import "$lib/chartData"".

- [ ] **Step 3: Extend the axis types**

In `src/lib/types.ts`, change

```ts
export type AxisField = 'pk' | 'sig' | 'pkPlusSig' | 'signingCycles' | 'verificationCycles' | 'signingUs' | 'verificationUs';
```

to

```ts
/** `signingTime` / `verificationTime`: displayed µs (measured, else extrapolated from cycles). */
export type AxisField =
	| 'pk' | 'sig' | 'pkPlusSig' | 'signingCycles' | 'verificationCycles' | 'signingUs' | 'verificationUs'
	| 'signingTime' | 'verificationTime';
```

and add three members to `KemAxisField`, after `'decapsUs'`:

```ts
    | 'keygenTime'
    | 'encapsTime'
    | 'decapsTime';
```

(Remove the `;` after `'decapsUs'`.)

- [ ] **Step 4: Write `src/lib/chartData.ts`**

```ts
import { kemTimes, sigTimes } from './timing';
import type { AxisField, KemAxisField, KemParameterSet, ParameterSet } from './types';

/** Chart value of a signature row on an axis; null when the row has no value there. */
export function sigAxisValue(row: ParameterSet, field: AxisField): number | null {
	switch (field) {
		case 'signingTime':
			return sigTimes(row).sign;
		case 'verificationTime':
			return sigTimes(row).verify;
		case 'signingCycles':
		case 'verificationCycles':
			return row[field] > 0 ? row[field] : null;
		default:
			return row[field];
	}
}

/** Chart value of a KEM row on an axis; null when the row has no value there. */
export function kemAxisValue(row: KemParameterSet, field: KemAxisField): number | null {
	switch (field) {
		case 'keygenTime':
			return kemTimes(row).keygen;
		case 'encapsTime':
			return kemTimes(row).encaps;
		case 'decapsTime':
			return kemTimes(row).decaps;
		case 'keygenCycles':
		case 'encapsCycles':
		case 'decapsCycles':
			return row[field] > 0 ? row[field] : null;
		default:
			return row[field];
	}
}

/** Split rows into those with a value on both axes and those left out of the chart. */
export function partitionPlottable<T, F>(
	rows: T[],
	valueOf: (row: T, field: F) => number | null,
	x: F,
	y: F
): { plotted: T[]; omitted: T[] } {
	const plotted: T[] = [];
	const omitted: T[] = [];
	for (const row of rows) {
		(valueOf(row, x) != null && valueOf(row, y) != null ? plotted : omitted).push(row);
	}
	return { plotted, omitted };
}

export function omittedNote(n: number): string {
	return n === 1
		? '1 parameter set is not shown: not yet benchmarked.'
		: `${n} parameter sets are not shown: not yet benchmarked.`;
}
```

- [ ] **Step 5: Write `src/lib/components/ChartModeToggle.svelte`**

```svelte
<script lang="ts">
	import type { ChartMode } from '$lib/types';

	let { mode, onchange }: { mode: ChartMode; onchange: (mode: ChartMode) => void } = $props();

	const MODES: { value: ChartMode; label: string }[] = [
		{ value: 'sizes', label: 'Sizes' },
		{ value: 'speed', label: 'Speed' }
	];
</script>

<div role="group" aria-label="Chart axes" class="inline-flex overflow-hidden rounded border border-pqs-bluegray dark:border-pqs-steel">
	{#each MODES as m (m.value)}
		<button
			type="button"
			aria-pressed={mode === m.value}
			onclick={() => onchange(m.value)}
			class="h-9 px-4 font-heading text-sm font-semibold {mode === m.value
				? 'bg-pqs-steel text-white'
				: 'bg-white text-pqs-midnight hover:bg-pqs-smoke dark:bg-pqs-midnight-mid dark:text-pqs-smoke dark:hover:bg-pqs-steel/30'}"
		>{m.label}</button>
	{/each}
</div>
```

- [ ] **Step 6: Leave out unplottable rows in `ScatterPlot.svelte`**

In `src/lib/components/ScatterPlot.svelte` (the Task 9 version):

1. Add imports:

```ts
	import { omittedNote, partitionPlottable, sigAxisValue } from '$lib/chartData';
	import { CPU_GHZ_LABEL } from '$lib/constants';
```

2. Add two entries at the end of `AXIS_TITLES`:

```ts
		signingTime: 'Signing time (µs)',
		verificationTime: 'Verification time (µs)',
```

3. After `let legend = $state<LegendGroup[]>([]);` add:

```ts
	let omitted = $state(0);

	const TIME_FIELDS = new Set<AxisField>(['signingTime', 'verificationTime']);
	const timeAxes = $derived(TIME_FIELDS.has(xField) || TIME_FIELDS.has(yField));
	const TIME_FOOTNOTE = `Timings in µs. Where only cycle counts exist, time is extrapolated at ${CPU_GHZ_LABEL}.`;
```

4. Replace `const data = $filteredRows;` with:

```ts
		const { plotted: data, omitted: missing } = partitionPlottable($filteredRows, sigAxisValue, xField, yField);
		omitted = missing.length;
```

5. In the object returned from `data.map`, after `verificationUs: d.verificationUs,` add:

```ts
				signingTime: sigAxisValue(d, 'signingTime'),
				verificationTime: sigAxisValue(d, 'verificationTime'),
```

6. In the markup, after the `{#if renderError} … {/if}` block add:

```svelte
		{#if omitted > 0}
			<p role="note" class="mt-2 rounded border border-dashed border-[#A855F7] bg-[#faf5ff] px-2.5 py-1.5 text-xs text-[#4c1d95] dark:bg-[#2e1065]/40 dark:text-[#e9d5ff]">
				{omittedNote(omitted)} They will appear once they are added to our benchmark harness.
			</p>
		{/if}
```

and replace `<ChartLegend groups={legend} />` with:

```svelte
	<ChartLegend groups={legend} footnote={timeAxes ? TIME_FOOTNOTE : undefined} />
```

- [ ] **Step 7: The same in `KemScatterPlot.svelte`**

In `src/lib/components/KemScatterPlot.svelte` (the Task 9 version):

1. Add imports:

```ts
	import { kemAxisValue, omittedNote, partitionPlottable } from '$lib/chartData';
	import { CPU_GHZ_LABEL } from '$lib/constants';
```

2. Add at the end of `AXIS_TITLES`:

```ts
		keygenTime: 'Keygen time (µs)',
		encapsTime: 'Encapsulation time (µs)',
		decapsTime: 'Decapsulation time (µs)',
```

3. After `let legend = $state<LegendGroup[]>([]);` add:

```ts
	let omitted = $state(0);

	const TIME_FIELDS = new Set<KemAxisField>(['keygenTime', 'encapsTime', 'decapsTime']);
	const timeAxes = $derived(TIME_FIELDS.has(xField) || TIME_FIELDS.has(yField));
	const TIME_FOOTNOTE = `Timings in µs. Where only cycle counts exist, time is extrapolated at ${CPU_GHZ_LABEL}.`;
```

4. Replace `const data = rows;` with:

```ts
		const { plotted: data, omitted: missing } = partitionPlottable(rows, kemAxisValue, xField, yField);
		omitted = missing.length;
```

5. After `decapsUs: d.decapsUs,` in the returned object add:

```ts
				keygenTime: kemAxisValue(d, 'keygenTime'),
				encapsTime: kemAxisValue(d, 'encapsTime'),
				decapsTime: kemAxisValue(d, 'decapsTime'),
```

6. Add the same `{#if omitted > 0} … {/if}` note after the `renderError` block, and replace `<ChartLegend groups={legend} />` with `<ChartLegend groups={legend} footnote={timeAxes ? TIME_FOOTNOTE : undefined} />`.

- [ ] **Step 8: Add the toggle to the main pages; keep the advanced pages compiling**

In `src/routes/signatures/+page.svelte` (re-read first), add `import ChartModeToggle from '$lib/components/ChartModeToggle.svelte';` and replace

```svelte
				<h2 class="mb-3 font-heading text-base font-semibold text-pqs-steel dark:text-pqs-apricot">
					pk size vs. sig size <span class="font-normal text-pqs-bluegray">(log–log scale)</span>
				</h2>
				<ScatterPlot />
```

with

```svelte
				<div class="mb-3 flex flex-wrap items-center justify-between gap-3">
					<h2 class="font-heading text-base font-semibold text-pqs-steel dark:text-pqs-apricot">
						{$store.chart === 'speed' ? 'Signing vs. verification time' : 'pk size vs. sig size'}
						<span class="font-normal text-pqs-bluegray">(log–log scale)</span>
					</h2>
					<ChartModeToggle mode={$store.chart} onchange={(chart) => store.update((f) => ({ ...f, chart }))} />
				</div>
				<ScatterPlot
					xField={$store.chart === 'speed' ? 'signingTime' : 'pk'}
					yField={$store.chart === 'speed' ? 'verificationTime' : 'sig'}
				/>
```

In `src/routes/kems/+page.svelte`, add `import ChartModeToggle from '$lib/components/ChartModeToggle.svelte';` and replace

```svelte
				<h2 class="mb-3 font-heading text-base font-semibold text-pqs-steel dark:text-pqs-apricot">
					pk size vs. ciphertext size <span class="font-normal text-pqs-bluegray">(log–log scale)</span>
				</h2>
				<KemScatterPlot rows={filteredRows} />
```

with

```svelte
				<div class="mb-3 flex flex-wrap items-center justify-between gap-3">
					<h2 class="font-heading text-base font-semibold text-pqs-steel dark:text-pqs-apricot">
						{filter.chart === 'speed' ? 'Encapsulation vs. decapsulation time' : 'pk size vs. ciphertext size'}
						<span class="font-normal text-pqs-bluegray">(log–log scale)</span>
					</h2>
					<ChartModeToggle mode={filter.chart} onchange={(chart) => (filter = { ...filter, chart })} />
				</div>
				<KemScatterPlot
					rows={filteredRows}
					xField={filter.chart === 'speed' ? 'encapsTime' : 'pk'}
					yField={filter.chart === 'speed' ? 'decapsTime' : 'ct'}
				/>
```

The advanced pages keep free axes and don't offer the new fields in their selects. Their `Record<…>` maps must still cover them, so:
- in `src/routes/signatures/advanced/+page.svelte`, add to `AXIS_SHORT`: `signingTime: 'signing time',` and `verificationTime: 'verification time',`;
- in `src/routes/kems/advanced/+page.svelte`, add to `AXIS_SHORT`: `keygenTime: 'keygen time',`, `encapsTime: 'encaps time',` and `decapsTime: 'decaps time'`.

- [ ] **Step 9: Run all tests to verify they pass**

Run: `npm run check && npm run test && npm run test:e2e`
Expected: PASS. The existing "scatter plot section heading" tests still see "pk size vs. sig size" and "pk size vs. ciphertext size" in the default Sizes view.

- [ ] **Step 10: Commit**

```bash
git add src/lib/chartData.ts src/lib/components/ChartModeToggle.svelte src/lib/types.ts src/lib/components/ScatterPlot.svelte src/lib/components/KemScatterPlot.svelte src/routes/signatures/+page.svelte src/routes/kems/+page.svelte src/routes/signatures/advanced/+page.svelte src/routes/kems/advanced/+page.svelte src/lib/__tests__/chartData.test.ts e2e/signatures.spec.ts e2e/kems.spec.ts
git commit -m "$(cat <<'EOF'
Add a Sizes/Speed toggle to the compare charts

Speed plots signing vs. verification (encaps vs. decaps for KEMs)
in µs; parameter sets without timings are left out with a visible
"not yet benchmarked" note.

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
EOF
)"
```

---

### Task 11: Documentation and full verification

**Files:**
- Modify: `CLAUDE.md`

**Interfaces:**
- Consumes: everything above. No new code.

- [ ] **Step 1: Update `CLAUDE.md`**

1. In the `## Architecture` tree under `src/lib/`, add these lines after `kemData.ts`:

```
│   ├── compareFilters.ts # programme groups, CommonFilter predicate + URL codec (both compare pages)
│   ├── badges.ts         # security badges (incl. implementation findings) + Programmes-column chips
│   ├── chartLegend.ts    # family shapes, NIST + NGCC colour scales, HTML legend groups
│   ├── chartData.ts      # axis values, leaving out unbenchmarked rows (Speed view)
│   ├── timing.ts         # displayed µs (measured, else extrapolated from cycles)
```

   Under `components/`, replace the `SecurityBadge.svelte` line with:

```
│       ├── FilterSidebar.svelte  # single filter column; collapsed on phones, sticky on desktop
│       ├── ProgrammeFilter / LevelFilter / ParamSetFilter / StatusFilter.svelte  # shared filter sections
│       ├── BadgeIcon / ProgrammeChips / TableLegend.svelte  # shared table pieces
│       ├── ChartLegend.svelte    # HTML legend beside the chart (below on phones)
│       ├── ChartModeToggle.svelte # Sizes / Speed
```

2. In `### Data Processing`, replace the "Fallback" and "Schemes that have other memberships…" bullets with:

```
- The stage filter only scopes that programme's entrants. A scheme with no memberships, or with
  a membership in any other programme (standards, KpqC, NGCC), is always included at its newest
  version; the page's programme filter decides whether it is shown.
- Schemes whose only memberships are in `filter.programme` and that never reached the stage are excluded.
```

3. Replace the URL-state paragraph in `### Filter Store` with:

```
URL state: filter params encoded as query params, applied client-side in `onMount`.
Round encoded as `?r=1|2|3`. No param = latest (default). Common keys (both compare pages,
`encodeCommonFilters`/`decodeCommonFilters` in `compareFilters.ts`): `p` programme groups,
`l` NIST levels (`PQ` = pre-quantum), `nl` NGCC targets (`l` without `nl` = old link, no NGCC
targets), `ps=all`, `hb`/`hi`/`hw` hide broken / implementation findings / warnings, `v=speed`.
Every signature key must be in `SIGNATURE_PARAMS` (`src/lib/legacy.ts`). KEM-only:
`pkMin…pcMax` sizes and `kuMin kuMax euMin euMax duMin duMax` µs.
Changing the on-ramp round keeps the common filters and the sort.
```

4. Replace the `### Round Selector` section body with:

```
`RoundSelect.svelte` ("On-ramp round", visually hidden label) sits under "NIST on-ramp" in the
programme filter on `/signatures/` and is disabled while that is unticked. Choosing a value updates
`roundStore`, which triggers `applyRound()` in `signatures/+page.svelte`.
```

5. In `### KEMs`, after the paragraph about `processKemSchemes()`, add:

```
Both compare pages share programme, NIST/NGCC level, Recommended/All and "Hide …" filters
(`CommonFilter`), the table layout (Programmes column, badge detail rows, `TableLegend`) and the
chart (HTML legend, Sizes/Speed toggle). NGCC targets have their own filter group and purple
colour scale and are never mapped onto NIST levels.
```

- [ ] **Step 2: Run the full verification**

Run: `npm run check && npm run test && npm run test:e2e && BASE_PATH=/nist-sigs-zoo npm run build`
Expected: svelte-check 0 errors; all Vitest and Playwright tests PASS; the build succeeds.

- [ ] **Step 3: Manual spot check**

Run `npm run preview` and open `/signatures/` and `/kems/` at 1536 px, 1024 px and 390 px wide, in light and dark themes. Check that:
- the content is at most 1536 px wide;
- the filter column is sticky on desktop and collapsed behind "Filters" on a phone;
- the legend sits beside the chart on desktop and below it on a phone;
- badge icons are readable in dark mode;
- the Speed view shows the note when you tick only research KEMs with no timings (if any exist).

- [ ] **Step 4: Commit**

```bash
git add CLAUDE.md
git commit -m "$(cat <<'EOF'
Document the compare-page filters, URL params and modules

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
EOF
)"
```
