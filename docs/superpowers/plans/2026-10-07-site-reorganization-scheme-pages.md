# Site Reorganization: Scheme Pages Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Give every signature scheme and every KEM its own static page at `/schemes/<slug>/`. The page has a header (family, assumption, website, programme memberships, latest version, security flags), a parameter-set table, a version comparison (signatures with two or more versions) and the scheme's slice of `data/history.yaml`. Every scheme name in the compare tables and every point in the compare charts links to it.

**Architecture:** One shared route, `src/routes/schemes/[slug]/`, serves both primitives. The load data carries a `primitive: 'signature' | 'kem'` discriminator. All logic is in pure, unit-tested modules:
- `slugs.ts`: slug derivation, uniqueness across both primitives, hrefs;
- `versionCompare.ts`: matches parameter sets by name, works out added/removed sets, percentage changes, the flag per version and the benchmark-source note;
- `schemeHistory.ts`: the history slice, merged with version releases;
- `schemePage.ts`: shapes YAML into the page model.

The Svelte components only render. `schemeIndex.ts` is a thin runtime loader that builds the slug index from the bundled YAML once. Tables and charts use it to turn a scheme name into a link. `entries()` lists every slug, so every page is prerendered, including schemes that no compare view shows by default (round-1-only schemes, for example).

**Tech Stack:** SvelteKit 2 + Svelte 5 runes, adapter-static, Tailwind v4 (`pqs-*` tokens), TypeScript, Vega-Lite via vega-embed, Vitest, Playwright.

**Spec:** The "Scheme page" board of the design canvas, copied as `.superpowers/spec/Scheme.dc.html` (MAYO mockup; an HTML mockup, so use it for layout, wording and behaviour, don't copy its code). The "Sitemap and data model" board (`.superpowers/spec/Sitemap.dc.html`: "`/schemes/<name>/`: one page per YAML file. Lists every programme the scheme is in, every version with dates, parameter sets, security flags and its slice of the timeline"). The plan-1 roadmap entry 5 in `docs/superpowers/plans/2026-10-07-site-reorganization-foundation.md`. Thom's brief for plan 5 (2026-10-07), quoted in the requirements below.

## Global Constraints

- Everything stays on branch `feat/programmes-foundation` (single-branch project). Never switch branches.
- Small, focused commits, one per task. Stage specific files and never use `git add -A` / `git add .`. End every commit message with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>` on its own line.
- **Never import third-party benchmark numbers.** This plan only displays timings that are already in `data/`.
- Slugs are lowercase ASCII (`^[a-z0-9]+(?:-[a-z0-9]+)*$`), stable, and unique across signatures **and** KEMs. Scheme names are unique across both primitives too, because `history.yaml` refers to schemes by name only.
- Every internal link is prefixed with `base` from `$app/paths`. Production builds with `BASE_PATH=/nist-sigs-zoo`.
- Static prerender: the route exports `prerender = true`, `trailingSlash = 'always'` and `entries()`. The prerenderer throws on any broken internal link (`svelte.config.js`), so never link to a route that does not exist yet (for example plan 4's `/programmes/<id>/`).
- Level scales are never converted. A parameter set shows a NIST level (`1`–`5`, `Pre-quantum`) **or** an NGCC target (`128-bit`), never both.
- `broken` means a design break. `implementation` (bugs in submitted code) is separate and never implies `broken`.
- Pure logic lives in `src/lib/*.ts` with Vitest tests in `src/lib/__tests__/`. Unit tests never import `$app/*`, `$lib/schemeData`, `$lib/kemSchemeData`, `$lib/programmeData`, `$lib/historyData` or `$lib/schemeIndex`. Build fixtures from YAML-shaped literals.
- Accessibility: every form control has a label; toggle buttons use `aria-pressed`; ▲/▼ never carry meaning alone (add screen-reader text); tables have a caption and `scope` on header cells; links that open a new tab say so to screen readers.
- Copy uses British spelling ("programme", "Standardisation", "colour").
- Old-URL compatibility is low priority: renaming a scheme may change its slug (pin it with `slug:` if that matters).
- Run `npm run check` and `npm run test` before every commit, and `npm run test:e2e` in tasks that touch components or routes.
- Plan 3 (NGCC import) commits concurrently to this branch, adding `data/schemes/*.yaml` and `data/kems/*.yaml`. Before each commit, `git pull --rebase` is **not** needed (same branch, same checkout), but re-run `npm run test`: a newly imported NGCC file can fail the slug/name uniqueness test. Fix that by adding `slug:` to the YAML (see Task 1), never by weakening the test.

## Review Focus

1. **MAYO has two versions both labelled "Round 2"** (2025-02-05 and 2025-03-26): the From/To selects must tell them apart ("Round 2 · 5 Feb 2025" vs "Round 2 · 26 Mar 2025"), and picking one must not select the other. Pinned by Task 2's `versionOptions` duplicate-date test and Task 6's e2e check of the selected option text.
2. **A tiny or undefined change** (pk 10 000 → 10 004, or a cycle count going from 0/absent to a value): the change cell must read "▲ <0.1%" or "—", never "▲ 0.0%", "Infinity%" or "NaN%". Pinned by Task 2's `formatChange` / `percentChange` tests.
3. **An NGCC name that slugifies to nothing or collides** (a Chinese-only name, or a signature and a KEM that share a name or slug): `npm run test` must fail with a message that names both schemes and says to add `slug:`. Two pages must never share a URL. Pinned by Task 1's `buildSchemeIndex` tests and the `dataFiles.test.ts` check.
4. **Panning the chart by dragging, with the drag ending on a point:** the drag must not navigate away; a plain click on a point must. Pinned by Task 8's `chartClickHref` tests.
5. **Dates in September and around midnight UTC:** "1 Sep 2026", not "1 Sept 2026" (en-GB ICU) and not "31 Aug 2026" (time-zone shift). Pinned by Task 3's `fmtDate` tests.

---

## Decisions (for Thom to confirm)

### D1. One shared route `/schemes/<slug>/` for both primitives

Chosen over `/signatures/<slug>/` + `/kems/<slug>/` because:
- The sitemap board and the plan-1 roadmap both specify `/schemes/<name>/`, "one page per YAML file".
- `data/history.yaml` names schemes without a primitive (`schemes: [SMAUG-T]`, `[NTRU+]`, `[MAYO]`), so names, and therefore slugs, have to be unambiguous across both primitives anyway.
- One `entries()` generator, one `schemeHref(base, slug)` for plans 4 and 6, and no clash with `/kems/advanced/` or `/signatures/advanced/`.
- The primitive still shows on the page: breadcrumb "Signatures / MAYO" or "KEMs / ML-KEM", and signature-only sections are guarded by `page.primitive`.

Cost: a signature and a KEM can never share a name or slug. `buildSchemeIndex` enforces this, and an optional `slug:` field in the YAML resolves a slug clash.

### D2. Slugs come from the scheme name, not the file name

`slugify(name)`: NFKD, strip diacritics, `+` → `plus`, lowercase, runs of other characters → `-`. Examples: MAYO → `mayo`, NTRU+ → `ntru-plus`, `EHTv3 / EHTv4` → `ehtv3-ehtv4`, `eMLE-Sig 2.0` → `emle-sig-2-0`, Aigis-Sig+ → `aigis-sig-plus`. File basenames are not normalised (`EHTv3_EHTv4.yaml`, `NTRUPlus.yaml`), and plan 3 generates its own file names, so names give more predictable URLs. An optional `slug:` in the YAML overrides the derived slug. It is needed for names with no ASCII letters or digits, for clashes, or to pin a URL across a rename. All 59 current names give unique slugs (checked).

### D3. Flags stay on versions for now, read as "today's assessment" (model change deferred)

Thom's concern: `broken`/`warning` live on versions even when the attack came later. MAYO "Round 2" (Feb/Mar 2025) carries the Wedge `broken` flag from an attack published on 9 May 2026. Options considered:
- **(a) `*_since: <date>` next to each flag** (or flags as objects `{text, since}`). This touches `types.ts`, `data.ts`, `kemData.ts`, `badges.ts`, the plan-3 generator `ngcc_data.py` (in flight) and ~30 YAML files. It answers "was this known at release?", which no page asks today.
- **(b) Flags derived from dated `attack` entries in `history.yaml`** (an entry gains `flags: {MAYO: {two: broken}}`). This inverts the source of truth, makes history entries load-bearing, and needs a migration of every flag.
- **(c) Keep the model and label it.** A flag on a version means "what we know today about these parameters". This is the fair reading for a comparison: "Round 2 → Round 3: flag cleared" is true today. The history slice directly below the comparison shows *when* each attack was published. The compare column is headed "Security flag (today's assessment)" with a one-line footnote.

**Proposal: (c) now, and revisit (a) after plans 3 and 6.** Plan 6 (timeline filters) is the natural home for dated flag events, and by then the NGCC flag text (plan 3, Task 14) will have settled, so the migration happens once. Known data gap to confirm separately: older versions whose parameter sets were also affected are not consistently flagged. MAYO "Round 1" `two` has different sizes and carries no flag. Auditing that is data work, out of scope here.

### D4. Cycle comparisons always carry a source caveat

In "Cycles" mode, the comparison prints each version's `perf_source` (or "source not recorded"). It always says that machines and implementations can differ between versions, so a change mixes scheme and platform effects. Two versions both labelled "submission document" are not treated as the same machine. Cycles come from `*_cycles`. When only µs exists they are estimated at 2.5 GHz and shown with the usual wavy underline.

### D5. `findings_url` credit

`findings_url` is rendered as "Third-party findings: full report on <host>", where the host comes from the URL (for example `ngcc.dev`). Only `http(s)` URLs are linked. The credit wording itself is inside the flag text (plan 3, Task 14 enforces "… (<credit>)." in every NGCC summary), and those texts appear in full under "Security flags". No new `findings_credit` field. If Thom wants the credit next to the link too, add `findings_credit?: string` later; it is a one-line change in `schemePage.ts`.

### D6. Version events in the history slice

As in the mockup, each version shows up as a "New version" event. If `history.yaml` has an `update` entry for the scheme on the same date, the two are merged into one event (title = version label, text = the update's description). This adds no data. KEMs have no versions, so they only show `history.yaml` entries.

---

## File Structure

| File | Responsibility |
|---|---|
| `src/lib/types.ts` (modify) | optional `slug?: string` on `SchemeYaml` and `KemSchemeYaml` |
| `src/lib/slugs.ts` (create) | `slugify`, `schemeSlug`, `buildSchemeIndex`, `schemeHref`, `Primitive`, `SchemeRef` (pure) |
| `src/lib/schemeIndex.ts` (create) | Runtime: `schemeIndex` built from bundled YAML; `slugOf(name)` |
| `src/lib/versionCompare.ts` (create) | `versionOptions`, `parameterSetFlag`, `metricValue`, `percentChange`, `compareVersions`, `formatChange`, `levelText`, labels (pure) |
| `src/lib/dates.ts` (create) | `fmtDate(iso)`: "31 Aug 2026" |
| `src/lib/schemeHistory.ts` (create) | `schemeEvents()`, `EVENT_LABELS` (pure) |
| `src/lib/historyData.ts` (create) | Runtime loader: `allHistory` from `data/history.yaml` |
| `src/lib/schemePage.ts` (create) | Page model: `signaturePage`, `kemPage`, table model, flag notes, flag summary, `hostOf` (pure) |
| `src/lib/chartNav.ts` (create) | `chartClickHref()`: click vs. drag, href from datum (pure) |
| `src/routes/schemes/[slug]/+page.ts` (create) | `entries()`, `load()` |
| `src/routes/schemes/[slug]/+page.svelte` (create) | Page composition |
| `src/lib/components/scheme/{SchemeHeader,ParamSetTable,FlagNotes,SchemeEvents,VersionCompare}.svelte` (create) | Page sections |
| `src/lib/components/{SchemeTable,KemTable}.svelte` (modify) | Scheme name links to the scheme page |
| `src/lib/components/{ScatterPlot,KemScatterPlot}.svelte` (modify) | Points carry `href`; click navigates |
| `src/lib/__tests__/{slugs,versionCompare,schemeHistory,dates,schemePage,chartNav}.test.ts` (create) | Unit tests |
| `src/lib/__tests__/dataFiles.test.ts` (modify) | Real data: unique slugs/names; history names resolve |
| `e2e/schemes.spec.ts` (create) | Scheme pages, compare, links from tables and charts |
| `CLAUDE.md`, `data/history.yaml` (modify) | Docs; site update entry |

---

### Task 1: Scheme slugs and index

**Files:**
- Modify: `src/lib/types.ts` (`SchemeYaml`, `KemSchemeYaml`)
- Create: `src/lib/slugs.ts`, `src/lib/schemeIndex.ts`
- Test: `src/lib/__tests__/slugs.test.ts`; modify `src/lib/__tests__/dataFiles.test.ts`

**Interfaces:**
- Consumes: `SchemeYaml`, `KemSchemeYaml`, `HistoryEntry` from `src/lib/types.ts`.
- Produces:
  - `type Primitive = 'signature' | 'kem'`
  - `interface SchemeRef { primitive: Primitive; name: string; slug: string }`
  - `slugify(name: string): string`
  - `schemeSlug(s: { name: string; slug?: string }): string` (throws on an invalid slug)
  - `buildSchemeIndex(sigs: { name: string; slug?: string }[], kems: { name: string; slug?: string }[]): SchemeRef[]` (sorted by slug; throws on a duplicate slug or name)
  - `schemeHref(base: string, slug: string): string` → `${base}/schemes/${slug}/`
  - runtime `schemeIndex: SchemeRef[]` and `slugOf(name: string): string` in `$lib/schemeIndex`

- [ ] **Step 1: Write the failing tests**

Create `src/lib/__tests__/slugs.test.ts`:

```ts
import { describe, expect, it } from 'vitest';
import { buildSchemeIndex, schemeHref, schemeSlug, slugify } from '$lib/slugs';

describe('slugify', () => {
	it.each([
		['MAYO', 'mayo'],
		['ML-KEM', 'ml-kem'],
		['NTRU+', 'ntru-plus'],
		['Aigis-Sig+', 'aigis-sig-plus'],
		['EHTv3 / EHTv4', 'ehtv3-ehtv4'],
		['eMLE-Sig 2.0', 'emle-sig-2-0'],
		['Xifrat1-Sign.I', 'xifrat1-sign-i'],
		['Classic McEliece', 'classic-mceliece'],
		['Café', 'cafe']
	])('%s -> %s', (name, slug) => {
		expect(slugify(name)).toBe(slug);
	});

	it('returns an empty string for a name without ASCII letters or digits', () => {
		expect(slugify('格密码')).toBe('');
	});
});

describe('schemeSlug', () => {
	it('uses the explicit slug when given', () => {
		expect(schemeSlug({ name: '格密码', slug: 'lattice-sig' })).toBe('lattice-sig');
	});

	it('rejects a name that gives no slug, and says how to fix it', () => {
		expect(() => schemeSlug({ name: '格密码' })).toThrow(/格密码.*slug:/);
	});

	it('rejects an explicit slug that is not lowercase ASCII', () => {
		expect(() => schemeSlug({ name: 'X', slug: 'Bad Slug' })).toThrow(/Bad Slug/);
	});
});

describe('buildSchemeIndex', () => {
	it('indexes both primitives, sorted by slug', () => {
		expect(buildSchemeIndex([{ name: 'MAYO' }], [{ name: 'ML-KEM' }, { name: 'BAT' }])).toEqual([
			{ primitive: 'kem', name: 'BAT', slug: 'bat' },
			{ primitive: 'signature', name: 'MAYO', slug: 'mayo' },
			{ primitive: 'kem', name: 'ML-KEM', slug: 'ml-kem' }
		]);
	});

	it('rejects a slug shared by a signature and a KEM, naming both', () => {
		expect(() => buildSchemeIndex([{ name: 'Foo+' }], [{ name: 'foo plus' }])).toThrow(
			/signature "Foo\+".*kem "foo plus".*slug:/
		);
	});

	it('rejects the same name used twice, even with distinct slugs', () => {
		expect(() => buildSchemeIndex([{ name: 'Foo' }], [{ name: 'Foo', slug: 'foo-kem' }])).toThrow(/name "Foo"/);
	});

	it('lets an explicit slug resolve a clash', () => {
		const index = buildSchemeIndex([{ name: 'Foo+' }], [{ name: 'foo plus', slug: 'foo-plus-kem' }]);
		expect(index.map((r) => r.slug)).toEqual(['foo-plus', 'foo-plus-kem']);
	});
});

describe('schemeHref', () => {
	it('prefixes the base path and ends with a slash', () => {
		expect(schemeHref('', 'mayo')).toBe('/schemes/mayo/');
		expect(schemeHref('/nist-sigs-zoo', 'mayo')).toBe('/nist-sigs-zoo/schemes/mayo/');
	});
});
```

Append to `src/lib/__tests__/dataFiles.test.ts` (and add `HistoryEntry` to its existing `import type { … } from '$lib/types'` line; add `import { buildSchemeIndex } from '$lib/slugs';` to the imports):

```ts
describe('scheme pages', () => {
	const readDir = <T>(dir: string): T[] =>
		readdirSync(join(DATA, dir))
			.filter((f) => f.endsWith('.yaml'))
			.map((f) => load<T>(join(DATA, dir, f)));
	const sigs = readDir<SchemeYaml>('schemes');
	const kems = readDir<KemSchemeYaml>('kems');

	it('gives every signature and KEM a unique lowercase ASCII slug and a unique name', () => {
		// On failure the thrown message names both schemes; add `slug:` to one YAML file.
		expect(() => buildSchemeIndex(sigs, kems)).not.toThrow();
	});

	it('every history entry names a scheme or KEM that has a page', () => {
		const names = new Set([...sigs, ...kems].map((s) => s.name));
		const history = load<HistoryEntry[]>(join(DATA, 'history.yaml'));
		const unknown = history.flatMap((e) =>
			(e.schemes ?? []).filter((n) => !names.has(n)).map((n) => `${e.date}: "${n}"`)
		);
		expect(unknown).toEqual([]);
	});
});
```

- [ ] **Step 2: Run tests to verify they fail**

Run: `npm run test -- src/lib/__tests__/slugs.test.ts src/lib/__tests__/dataFiles.test.ts`
Expected: FAIL. `slugs.test.ts` cannot resolve `$lib/slugs`, and `dataFiles.test.ts` fails the same import.

- [ ] **Step 3: Implement**

In `src/lib/types.ts`, add to `SchemeYaml` (after `assumption: string;`) and to `KemSchemeYaml` (after `assumption: string;`):

```ts
	/** Optional URL slug for /schemes/<slug>/; defaults to slugify(name). Needed only for clashes or non-ASCII names. */
	slug?: string;
```

Create `src/lib/slugs.ts`:

```ts
/** Scheme-page URLs: /schemes/<slug>/, one namespace shared by signatures and KEMs. */

export type Primitive = 'signature' | 'kem';

export interface SchemeRef {
	primitive: Primitive;
	name: string;
	slug: string;
}

const VALID_SLUG = /^[a-z0-9]+(?:-[a-z0-9]+)*$/;

/** Lowercase ASCII slug: strip diacritics, "+" -> "plus", other runs -> "-". Empty if nothing is left. */
export function slugify(name: string): string {
	return name
		.normalize('NFKD')
		.replace(/[̀-ͯ]/g, '')
		.replace(/\+/g, ' plus ')
		.toLowerCase()
		.replace(/[^a-z0-9]+/g, '-')
		.replace(/^-+|-+$/g, '');
}

export function schemeSlug(s: { name: string; slug?: string }): string {
	const slug = s.slug ?? slugify(s.name);
	if (!VALID_SLUG.test(slug)) {
		throw new Error(
			`scheme "${s.name}": slug "${slug}" is not lowercase ASCII (a-z, 0-9, single hyphens); add a "slug:" field to its YAML file`
		);
	}
	return slug;
}

export function buildSchemeIndex(
	sigs: { name: string; slug?: string }[],
	kems: { name: string; slug?: string }[]
): SchemeRef[] {
	const refs: SchemeRef[] = [
		...sigs.map((s) => ({ primitive: 'signature' as const, name: s.name, slug: schemeSlug(s) })),
		...kems.map((k) => ({ primitive: 'kem' as const, name: k.name, slug: schemeSlug(k) }))
	];
	const bySlug = new Map<string, SchemeRef>();
	const byName = new Map<string, SchemeRef>();
	for (const ref of refs) {
		const slugClash = bySlug.get(ref.slug);
		if (slugClash) {
			throw new Error(
				`slug "${ref.slug}" is used by ${slugClash.primitive} "${slugClash.name}" and ${ref.primitive} "${ref.name}"; add a "slug:" field to one of them`
			);
		}
		const nameClash = byName.get(ref.name);
		if (nameClash) {
			throw new Error(
				`name "${ref.name}" is used by a ${nameClash.primitive} and a ${ref.primitive}; history.yaml refers to schemes by name, so names must be unique`
			);
		}
		bySlug.set(ref.slug, ref);
		byName.set(ref.name, ref);
	}
	return refs.sort((a, b) => a.slug.localeCompare(b.slug));
}

export function schemeHref(base: string, slug: string): string {
	return `${base}/schemes/${slug}/`;
}
```

Create `src/lib/schemeIndex.ts`:

```ts
import { allKemData } from './kemSchemeData';
import { allSchemeData } from './schemeData';
import { buildSchemeIndex, type SchemeRef } from './slugs';

/** Every scheme page, built once from the bundled YAML. Throws at build time on a slug or name clash. */
export const schemeIndex: SchemeRef[] = buildSchemeIndex(allSchemeData, allKemData);

const byName = new Map(schemeIndex.map((r) => [r.name, r]));

export function slugOf(name: string): string {
	const ref = byName.get(name);
	if (!ref) throw new Error(`no scheme page for "${name}"`);
	return ref.slug;
}
```

- [ ] **Step 4: Run tests to verify they pass**

Run: `npm run test && npm run check`
Expected: PASS, 0 errors.

- [ ] **Step 5: Commit**

```bash
git add src/lib/types.ts src/lib/slugs.ts src/lib/schemeIndex.ts src/lib/__tests__/slugs.test.ts src/lib/__tests__/dataFiles.test.ts
git commit -m "Add scheme slugs shared by signatures and KEMs

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 2: Version comparison

**Files:**
- Create: `src/lib/versionCompare.ts`
- Test: `src/lib/__tests__/versionCompare.test.ts`

**Interfaces:**
- Consumes: `VersionYaml`, `ParameterSetYaml`, `NgccLevel` from `types.ts`; `CYCLES_PER_US` from `constants.ts`.
- Produces (all exported):
  - `type CompareMode = 'sizes' | 'cycles'`; `type MetricKey = 'pk' | 'sig' | 'signingCycles' | 'verificationCycles'`
  - `COMPARE_METRICS: Record<CompareMode, readonly [MetricKey, MetricKey]>`, `METRIC_LABELS: Record<MetricKey, string>`
  - `type FlagKind = 'broken' | 'implementation' | 'warning'`, `FLAG_TITLES: Record<FlagKind, string>`, `interface VersionFlag { kind: FlagKind; html: string }`
  - `interface VersionOption { id: string; label: string; date: string; perfSource: string | null }`
  - `interface MetricValue { value: number; estimated: boolean }`, `interface MetricChange { from: MetricValue | null; to: MetricValue | null; pct: number | null }`
  - `type FlagChange = 'none' | 'unchanged' | 'raised' | 'cleared' | 'changed' | 'n/a'`
  - `interface ParamSetComparison { name; presence: 'both' | 'added' | 'removed'; fromLevel: string | null; toLevel: string | null; metrics: Record<MetricKey, MetricChange>; fromFlag: VersionFlag | null; toFlag: VersionFlag | null; flagChange: FlagChange }`
  - `interface PerfNote { from: string | null; to: string | null; same: boolean; anyEstimated: boolean }`, `interface VersionComparison { rows: ParamSetComparison[]; perf: PerfNote }`
  - `levelText(level, ngccLevel): string`, `versionOptions(versions: VersionYaml[]): VersionOption[]` (keeps the given order), `parameterSetFlag(v, ps): VersionFlag | null`, `metricValue(ps, key): MetricValue | null`, `percentChange(from, to): number | null`, `compareVersions(from: VersionYaml, to: VersionYaml): VersionComparison`, `formatChange(pct: number | null): ChangeText` with `interface ChangeText { symbol: '▼' | '▲' | ''; text: string; direction: 'down' | 'up' | 'same' | null }`

- [ ] **Step 1: Write the failing test**

Create `src/lib/__tests__/versionCompare.test.ts`:

```ts
import { describe, expect, it } from 'vitest';
import {
	compareVersions,
	formatChange,
	levelText,
	metricValue,
	parameterSetFlag,
	percentChange,
	versionOptions
} from '$lib/versionCompare';
import type { VersionYaml } from '$lib/types';

const R2: VersionYaml = {
	version: 'Round 2',
	date: '2025-02-05',
	status: 'On-ramp',
	perf_source: 'submission document',
	parametersets: [
		{ name: 'one', level: 1, pk: 1420, sig: 454, signing_cycles: 471028, verification_cycles: 153266, warning: 'MQ' },
		{ name: 'two', level: 1, pk: 4912, sig: 186, signing_cycles: 286028, verification_cycles: 56374, broken: 'Wedge' },
		{ name: 'old', level: 3, pk: 100, sig: 100 }
	]
};

const R3: VersionYaml = {
	version: 'Round 3',
	date: '2026-08-31',
	status: 'On-ramp',
	perf_source: 'our benchmark',
	parametersets: [
		{ name: 'two', level: 1, pk: 2928, sig: 239, signing_cycles: 95622, verification_cycles: 38545 },
		{ name: 'one', level: 1, pk: 1456, sig: 464, signing_us: 47.2, verification_us: 20.9, warning: 'MQ' },
		{ name: 'new', level: 5, pk: 5554, sig: 964 }
	]
};

const row = (c: ReturnType<typeof compareVersions>, name: string) => c.rows.find((r) => r.name === name)!;

describe('compareVersions', () => {
	const c = compareVersions(R2, R3);

	it('matches parameter sets by name: To order first, then sets removed in To', () => {
		expect(c.rows.map((r) => [r.name, r.presence])).toEqual([
			['two', 'both'],
			['one', 'both'],
			['new', 'added'],
			['old', 'removed']
		]);
	});

	it('computes percentage changes', () => {
		expect(row(c, 'two').metrics.pk.pct).toBeCloseTo(-40.39, 2);
		expect(row(c, 'two').metrics.sig.pct).toBeCloseTo(28.49, 2);
		expect(row(c, 'two').metrics.signingCycles).toMatchObject({
			from: { value: 286028, estimated: false },
			to: { value: 95622, estimated: false }
		});
	});

	it('has no change for added and removed sets', () => {
		expect(row(c, 'new').metrics.pk).toEqual({ from: null, to: { value: 5554, estimated: false }, pct: null });
		expect(row(c, 'old').metrics.sig).toEqual({ from: { value: 100, estimated: false }, to: null, pct: null });
		expect([row(c, 'new').fromLevel, row(c, 'new').toLevel]).toEqual([null, '5']);
		expect([row(c, 'old').fromLevel, row(c, 'old').toLevel]).toEqual(['3', null]);
	});

	it('estimates cycles from µs when a version has no cycle count', () => {
		expect(row(c, 'one').metrics.signingCycles.to).toEqual({ value: 118000, estimated: true });
		expect(c.perf.anyEstimated).toBe(true);
	});

	it('reports the flag of each version and how it changed', () => {
		expect(row(c, 'two')).toMatchObject({ fromFlag: { kind: 'broken', html: 'Wedge' }, toFlag: null, flagChange: 'cleared' });
		expect(row(c, 'one')).toMatchObject({ flagChange: 'unchanged' });
		expect(row(c, 'new').flagChange).toBe('n/a');
		expect(row(c, 'old').flagChange).toBe('n/a');
	});

	it('notes both benchmark sources and never treats different sources as the same', () => {
		expect(c.perf).toMatchObject({ from: 'submission document', to: 'our benchmark', same: false });
		expect(compareVersions(R2, { ...R2, date: '2025-03-26' }).perf.same).toBe(true);
		expect(compareVersions({ ...R2, perf_source: undefined }, { ...R2, perf_source: undefined }).perf).toMatchObject({
			from: null,
			to: null,
			same: false
		});
	});

	it('compares a version with itself as unchanged', () => {
		const self = compareVersions(R3, R3);
		expect(self.rows.every((r) => r.presence === 'both' && (r.metrics.pk.pct === 0))).toBe(true);
		expect(self.rows.map((r) => r.flagChange)).toEqual(['none', 'unchanged', 'none']);
	});

	it('raises and changes flags', () => {
		const flagged: VersionYaml = { ...R3, warning: 'new attack' };
		expect(row(compareVersions(R3, flagged), 'two').flagChange).toBe('raised');
		const worse: VersionYaml = { ...R2, parametersets: R2.parametersets.map((p) => ({ ...p, warning: undefined, broken: 'x' })) };
		expect(row(compareVersions(R2, worse), 'one').flagChange).toBe('changed');
	});
});

describe('parameterSetFlag', () => {
	const v: VersionYaml = { version: 'v', date: '2025-01-01', status: 'On-ramp', broken: 'version-wide', parametersets: [] };

	it('applies a version-level flag to every set and lets a set override it', () => {
		expect(parameterSetFlag(v, { name: 'a', pk: 1, sig: 1 })).toEqual({ kind: 'broken', html: 'version-wide' });
		expect(parameterSetFlag(v, { name: 'b', pk: 1, sig: 1, broken: 'set-specific' })).toEqual({ kind: 'broken', html: 'set-specific' });
	});

	it('ranks broken over implementation findings over warnings', () => {
		const plain: VersionYaml = { ...v, broken: undefined };
		expect(parameterSetFlag(plain, { name: 'a', pk: 1, sig: 1, implementation: 'bug', warning: 'w' })?.kind).toBe('implementation');
		expect(parameterSetFlag(plain, { name: 'a', pk: 1, sig: 1, warning: 'w' })?.kind).toBe('warning');
		expect(parameterSetFlag(plain, { name: 'a', pk: 1, sig: 1, info: 'just a note' })).toBeNull();
	});

	it('does not treat a pre-quantum scheme as a security flag', () => {
		expect(parameterSetFlag({ ...v, broken: 'classical' }, { name: 'a', pk: 1, sig: 1 })).toBeNull();
	});
});

describe('metricValue', () => {
	it('returns null when neither cycles nor µs exist', () => {
		expect(metricValue({ name: 'a', pk: 1, sig: 1 }, 'verificationCycles')).toBeNull();
	});
});

describe('percentChange', () => {
	it('is null when a side is missing or the base is zero', () => {
		expect(percentChange(null, 5)).toBeNull();
		expect(percentChange(5, undefined)).toBeNull();
		expect(percentChange(0, 5)).toBeNull();
	});
});

describe('formatChange', () => {
	it.each([
		[null, '', '—', null],
		[0, '', 'same', 'same'],
		[-40.39, '▼', '40%', 'down'],
		[2.535, '▲', '3%', 'up'],
		[0.5, '▲', '0.5%', 'up'],
		[-0.96, '▼', '1.0%', 'down'],
		[0.04, '▲', '<0.1%', 'up']
	])('%s -> %s %s', (pct, symbol, text, direction) => {
		expect(formatChange(pct)).toEqual({ symbol, text, direction });
	});
});

describe('versionOptions', () => {
	it('keeps the given order and gives same-date versions distinct ids', () => {
		const a: VersionYaml = { version: 'Round 2', date: '2025-03-26', status: 'On-ramp', perf_source: 'x', parametersets: [] };
		const b: VersionYaml = { version: 'Round 2', date: '2025-03-26', status: 'On-ramp', parametersets: [] };
		expect(versionOptions([a, b, R2])).toEqual([
			{ id: '2025-03-26', label: 'Round 2', date: '2025-03-26', perfSource: 'x' },
			{ id: '2025-03-26-2', label: 'Round 2', date: '2025-03-26', perfSource: null },
			{ id: '2025-02-05', label: 'Round 2', date: '2025-02-05', perfSource: 'submission document' }
		]);
	});
});

describe('levelText', () => {
	it('shows a NIST level, pre-quantum or an NGCC target, never converting', () => {
		expect(levelText('Pre-Quantum', undefined)).toBe('Pre-quantum');
		expect(levelText(3, undefined)).toBe('3');
		expect(levelText(undefined, 128)).toBe('128-bit');
		expect(levelText(null, null)).toBe('—');
	});
});
```

- [ ] **Step 2: Run test to verify it fails**

Run: `npm run test -- src/lib/__tests__/versionCompare.test.ts`
Expected: FAIL, cannot resolve `$lib/versionCompare`.

- [ ] **Step 3: Implement**

Create `src/lib/versionCompare.ts`:

```ts
import { CYCLES_PER_US } from './constants';
import type { NgccLevel, ParameterSetYaml, VersionYaml } from './types';

export type CompareMode = 'sizes' | 'cycles';
export type MetricKey = 'pk' | 'sig' | 'signingCycles' | 'verificationCycles';

const ALL_METRICS: MetricKey[] = ['pk', 'sig', 'signingCycles', 'verificationCycles'];

export const COMPARE_METRICS: Record<CompareMode, readonly [MetricKey, MetricKey]> = {
	sizes: ['pk', 'sig'],
	cycles: ['signingCycles', 'verificationCycles']
};

export const METRIC_LABELS: Record<MetricKey, string> = {
	pk: 'Public key (B)',
	sig: 'Signature (B)',
	signingCycles: 'Sign (cycles)',
	verificationCycles: 'Verify (cycles)'
};

export type FlagKind = 'broken' | 'implementation' | 'warning';

export const FLAG_TITLES: Record<FlagKind, string> = {
	broken: 'Broken',
	implementation: 'Implementation findings',
	warning: 'Warning'
};

export interface VersionFlag {
	kind: FlagKind;
	/** Trusted HTML from our own YAML. */
	html: string;
}

export interface VersionOption {
	/** Unique within the scheme: the date, suffixed "-2", "-3" … for same-date versions. */
	id: string;
	label: string;
	date: string;
	perfSource: string | null;
}

export interface MetricValue {
	value: number;
	/** Cycles estimated from µs at the reference clock. */
	estimated: boolean;
}

export interface MetricChange {
	from: MetricValue | null;
	to: MetricValue | null;
	pct: number | null;
}

export type FlagChange = 'none' | 'unchanged' | 'raised' | 'cleared' | 'changed' | 'n/a';

export interface ParamSetComparison {
	name: string;
	presence: 'both' | 'added' | 'removed';
	fromLevel: string | null;
	toLevel: string | null;
	metrics: Record<MetricKey, MetricChange>;
	fromFlag: VersionFlag | null;
	toFlag: VersionFlag | null;
	/** 'n/a' unless the set exists in both versions. */
	flagChange: FlagChange;
}

export interface PerfNote {
	from: string | null;
	to: string | null;
	/** Both versions name the same, non-empty source. Even then the machines may differ. */
	same: boolean;
	anyEstimated: boolean;
}

export interface VersionComparison {
	rows: ParamSetComparison[];
	perf: PerfNote;
}

export function levelText(
	level: number | 'Pre-Quantum' | null | undefined,
	ngccLevel: NgccLevel | null | undefined
): string {
	if (level === 'Pre-Quantum') return 'Pre-quantum';
	if (level != null) return String(level);
	if (ngccLevel != null) return `${ngccLevel}-bit`;
	return '—';
}

/** Select options in the given order (callers pass newest first). */
export function versionOptions(versions: VersionYaml[]): VersionOption[] {
	const seen = new Map<string, number>();
	return versions.map((v) => {
		const n = (seen.get(v.date) ?? 0) + 1;
		seen.set(v.date, n);
		return {
			id: n === 1 ? v.date : `${v.date}-${n}`,
			label: v.version,
			date: v.date,
			perfSource: v.perf_source ?? null
		};
	});
}

/** The flag a parameter set carries in a version: set-level overrides version-level; broken > implementation > warning. */
export function parameterSetFlag(v: VersionYaml, ps: ParameterSetYaml): VersionFlag | null {
	const broken = ps.broken ?? v.broken;
	if (broken && broken !== 'classical') return { kind: 'broken', html: broken };
	const implementation = ps.implementation ?? v.implementation;
	if (implementation) return { kind: 'implementation', html: implementation };
	const warning = ps.warning ?? v.warning;
	if (warning) return { kind: 'warning', html: warning };
	return null;
}

export function metricValue(ps: ParameterSetYaml, key: MetricKey): MetricValue | null {
	if (key === 'pk' || key === 'sig') return { value: ps[key], estimated: false };
	const [cycles, us] =
		key === 'signingCycles'
			? [ps.signing_cycles, ps.signing_us]
			: [ps.verification_cycles, ps.verification_us];
	if (cycles != null && cycles > 0) return { value: cycles, estimated: false };
	if (us != null && us > 0) return { value: Math.round(us * CYCLES_PER_US), estimated: true };
	return null;
}

export function percentChange(from: number | null | undefined, to: number | null | undefined): number | null {
	if (from == null || to == null || from === 0) return null;
	return ((to - from) / from) * 100;
}

function flagChange(from: VersionFlag | null, to: VersionFlag | null): FlagChange {
	if (!from && !to) return 'none';
	if (!from) return 'raised';
	if (!to) return 'cleared';
	return from.kind === to.kind ? 'unchanged' : 'changed';
}

export function compareVersions(from: VersionYaml, to: VersionYaml): VersionComparison {
	const fromByName = new Map(from.parametersets.map((p) => [p.name, p]));
	const toNames = new Set(to.parametersets.map((p) => p.name));
	const pairs: [ParameterSetYaml | null, ParameterSetYaml | null][] = [
		...to.parametersets.map((b): [ParameterSetYaml | null, ParameterSetYaml] => [fromByName.get(b.name) ?? null, b]),
		...from.parametersets
			.filter((a) => !toNames.has(a.name))
			.map((a): [ParameterSetYaml, null] => [a, null])
	];

	const rows = pairs.map(([a, b]): ParamSetComparison => {
		const metrics = Object.fromEntries(
			ALL_METRICS.map((k) => {
				const x = a ? metricValue(a, k) : null;
				const y = b ? metricValue(b, k) : null;
				return [k, { from: x, to: y, pct: percentChange(x?.value, y?.value) }];
			})
		) as Record<MetricKey, MetricChange>;
		const fromFlag = a ? parameterSetFlag(from, a) : null;
		const toFlag = b ? parameterSetFlag(to, b) : null;
		return {
			name: (b ?? a)!.name,
			presence: a && b ? 'both' : b ? 'added' : 'removed',
			fromLevel: a ? levelText(a.level, a.ngcc_level) : null,
			toLevel: b ? levelText(b.level, b.ngcc_level) : null,
			metrics,
			fromFlag,
			toFlag,
			flagChange: a && b ? flagChange(fromFlag, toFlag) : 'n/a'
		};
	});

	const fromSource = from.perf_source ?? null;
	const toSource = to.perf_source ?? null;
	const anyEstimated = rows.some((r) =>
		COMPARE_METRICS.cycles.some((k) => r.metrics[k].from?.estimated || r.metrics[k].to?.estimated)
	);
	return {
		rows,
		perf: { from: fromSource, to: toSource, same: fromSource !== null && fromSource === toSource, anyEstimated }
	};
}

export interface ChangeText {
	symbol: '▼' | '▲' | '';
	text: string;
	direction: 'down' | 'up' | 'same' | null;
}

/** "▼ 40%", "▲ 0.5%", "▲ <0.1%", "same" or "—" (no comparable values). */
export function formatChange(pct: number | null): ChangeText {
	if (pct === null) return { symbol: '', text: '—', direction: null };
	if (pct === 0) return { symbol: '', text: 'same', direction: 'same' };
	const abs = Math.abs(pct);
	const shown = abs < 1 ? abs.toFixed(1) : String(Math.round(abs));
	const text = shown === '0.0' ? '<0.1%' : `${shown}%`;
	return pct < 0 ? { symbol: '▼', text, direction: 'down' } : { symbol: '▲', text, direction: 'up' };
}
```

- [ ] **Step 4: Run tests to verify they pass**

Run: `npm run test && npm run check`
Expected: PASS, 0 errors.

- [ ] **Step 5: Commit**

```bash
git add src/lib/versionCompare.ts src/lib/__tests__/versionCompare.test.ts
git commit -m "Add version comparison: sets matched by name, changes, flags, perf sources

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 3: Scheme history slice and date formatting

**Files:**
- Create: `src/lib/schemeHistory.ts`, `src/lib/dates.ts`, `src/lib/historyData.ts`
- Test: `src/lib/__tests__/schemeHistory.test.ts`, `src/lib/__tests__/dates.test.ts`

**Interfaces:**
- Consumes: `HistoryEntry` from `types.ts`.
- Produces:
  - `type SchemeEventKind = HistoryEntry['type'] | 'version'`
  - `interface SchemeEvent { date: string; kind: SchemeEventKind; title: string | null; html: string | null }`
  - `EVENT_LABELS: Record<SchemeEventKind, string>`
  - `schemeEvents(history: HistoryEntry[], name: string, versions: { version: string; date: string }[]): SchemeEvent[]` (newest first)
  - `fmtDate(iso: string): string`
  - runtime `allHistory: HistoryEntry[]` in `$lib/historyData`

- [ ] **Step 1: Write the failing tests**

Create `src/lib/__tests__/dates.test.ts`:

```ts
import { describe, expect, it } from 'vitest';
import { fmtDate } from '$lib/dates';

describe('fmtDate', () => {
	it('formats ISO dates as "D Mon YYYY" without time-zone shifts', () => {
		expect(fmtDate('2026-08-31')).toBe('31 Aug 2026');
		expect(fmtDate('2026-09-01')).toBe('1 Sep 2026');
		expect(fmtDate('2025-01-01')).toBe('1 Jan 2025');
	});

	it('returns anything that is not an ISO date unchanged', () => {
		expect(fmtDate('soon')).toBe('soon');
	});
});
```

Create `src/lib/__tests__/schemeHistory.test.ts`:

```ts
import { describe, expect, it } from 'vitest';
import { schemeEvents } from '$lib/schemeHistory';
import type { HistoryEntry } from '$lib/types';

const HISTORY: HistoryEntry[] = [
	{ date: '2025-01-01', type: 'update', description: 'v2 released', schemes: ['Toy'] },
	{ date: '2025-01-01', type: 'attack', description: 'same-day attack', schemes: ['Toy', 'Other'] },
	{ date: '2024-06-01', type: 'milestone', description: 'round 2', schemes: ['Other', 'Toy'] },
	{ date: '2024-03-01', type: 'update', description: 'other only', schemes: ['Other'] },
	{ date: '2023-01-01', type: 'milestone', description: 'no schemes' }
];

const VERSIONS = [
	{ version: 'v2', date: '2025-01-01' },
	{ version: 'v1', date: '2024-01-01' }
];

describe('schemeEvents', () => {
	it('keeps only entries naming the scheme, merges same-day updates into versions, newest first', () => {
		expect(schemeEvents(HISTORY, 'Toy', VERSIONS)).toEqual([
			{ date: '2025-01-01', kind: 'version', title: 'v2', html: 'v2 released' },
			{ date: '2025-01-01', kind: 'attack', title: null, html: 'same-day attack' },
			{ date: '2024-06-01', kind: 'milestone', title: null, html: 'round 2' },
			{ date: '2024-01-01', kind: 'version', title: 'v1', html: null }
		]);
	});

	it('matches names exactly, not as substrings', () => {
		expect(schemeEvents(HISTORY, 'Oth', [])).toEqual([]);
	});

	it('works without versions (KEMs)', () => {
		expect(schemeEvents(HISTORY, 'Other', []).map((e) => e.kind)).toEqual(['attack', 'milestone', 'update']);
	});
});
```

- [ ] **Step 2: Run tests to verify they fail**

Run: `npm run test -- src/lib/__tests__/dates.test.ts src/lib/__tests__/schemeHistory.test.ts`
Expected: FAIL, modules not found.

- [ ] **Step 3: Implement**

Create `src/lib/dates.ts`:

```ts
const MONTHS = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];

/** "2026-08-31" -> "31 Aug 2026". Parsed by hand: no time zone, no locale-dependent month names. */
export function fmtDate(iso: string): string {
	const m = /^(\d{4})-(\d{2})-(\d{2})/.exec(iso);
	if (!m) return iso;
	const month = MONTHS[Number(m[2]) - 1];
	return month ? `${Number(m[3])} ${month} ${m[1]}` : iso;
}
```

Create `src/lib/schemeHistory.ts`:

```ts
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
```

Create `src/lib/historyData.ts`:

```ts
import type { HistoryEntry } from './types';

const modules = import.meta.glob('../../data/history.yaml', { eager: true }) as Record<
	string,
	{ default: HistoryEntry[] }
>;

export const allHistory: HistoryEntry[] = Object.values(modules)[0]?.default ?? [];
```

(Do not change `src/routes/+page.ts` or `src/routes/timeline/+page.ts`, which load history themselves: plan 6 owns the timeline and can switch them to this loader.)

- [ ] **Step 4: Run tests to verify they pass**

Run: `npm run test && npm run check`
Expected: PASS, 0 errors.

- [ ] **Step 5: Commit**

```bash
git add src/lib/schemeHistory.ts src/lib/dates.ts src/lib/historyData.ts src/lib/__tests__/schemeHistory.test.ts src/lib/__tests__/dates.test.ts
git commit -m "Add a per-scheme history slice with version events

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 4: Scheme page model

**Files:**
- Create: `src/lib/schemePage.ts`
- Test: `src/lib/__tests__/schemePage.test.ts`

**Interfaces:**
- Consumes: `processYamlSchemes` (`data.ts`), `processKemSchemes` (`kemData.ts`), `schemeProgrammes` (`programmes.ts`), `programmeChips`, `securityBadges`, `Badge`, `BadgeKind`, `Chip` (`badges.ts`), `fmt`, `fmtCycles`, `fmtTime` (`format.ts`), `CYCLES_PER_US`, `CPU_GHZ_LABEL` (`constants.ts`), `schemeSlug`, `Primitive` (Task 1), `levelText`, `versionOptions`, `VersionOption` (Task 2), `schemeEvents`, `SchemeEvent` (Task 3).
- Produces:
  - `interface TableCell { text: string; kind: 'value' | 'estimated' | 'missing'; title?: string }`
  - `interface ParamSetRow { key: string; name: string; level: string; recommendedTag: boolean; badges: Badge[]; cells: TableCell[] }`
  - `interface ParamSetTableModel { headers: string[]; rows: ParamSetRow[] }`
  - `interface FlagNote extends Badge { sets: string[]; allSets: boolean }`
  - `interface SignaturePage { primitive: 'signature'; latest: VersionOption; versions: VersionYaml[]; options: VersionOption[]; …common }`, `interface KemPage { primitive: 'kem'; status: string; …common }`, `type SchemePageData = SignaturePage | KemPage`. The common fields are `name, slug, website, websiteHost: string | null, category, assumption, chips: Chip[], flagSummary: string, findings: { url: string; host: string } | null, table: ParamSetTableModel, flagNotes: FlagNote[], events: SchemeEvent[]`.
  - `hostOf(url: string): string | null`, `timeCell(us, cycles): TableCell`, `signatureTable(rows)`, `kemTable(rows)`, `flagNotes(rows)`, `flagSummary(notes, total)`, `signaturePage(yaml, history, registry): SignaturePage`, `kemPage(yaml, history, registry): KemPage`

- [ ] **Step 1: Write the failing test**

Create `src/lib/__tests__/schemePage.test.ts`:

```ts
import { describe, expect, it } from 'vitest';
import { flagSummary, hostOf, kemPage, signaturePage, timeCell } from '$lib/schemePage';
import type { HistoryEntry, KemSchemeYaml, ProgrammeYaml, SchemeYaml } from '$lib/types';

const REGISTRY: ProgrammeYaml[] = [
	{ id: 'nist-pqc', name: 'NIST PQC', short: 'NIST', organiser: 'NIST', region: 'US', primitives: ['signature', 'kem'], stages: [{ id: 'standardised', label: 'standard' }] },
	{ id: 'nist-onramp', name: 'On-ramp', short: 'On-ramp', organiser: 'NIST', region: 'US', primitives: ['signature'], stages: [{ id: 'round-1', label: 'round 1' }, { id: 'round-2', label: 'round 2' }] },
	{ id: 'china-ngcc', name: 'NGCC', short: 'NGCC', organiser: 'ICCS', region: 'China', primitives: ['signature', 'kem'], stages: [{ id: 'round-1', label: 'round 1' }] }
];

const SIG: SchemeYaml = {
	name: 'Toy',
	website: 'https://www.toy.example/',
	category: 'Multivariate',
	assumption: 'MQ',
	versions: [
		{
			version: 'v1', date: '2024-01-01', status: 'On-ramp', programmes: { 'nist-onramp': 'round-1' },
			parametersets: [{ name: 'a', level: 1, pk: 100, sig: 50, signing_cycles: 1000, verification_cycles: 500 }]
		},
		{
			version: 'v2', date: '2025-01-01', status: 'On-ramp', programmes: { 'nist-onramp': 'round-2' },
			findings_url: 'https://www.ngcc.dev/r.html',
			parametersets: [
				{ name: 'a', level: 1, pk: 90, sig: 60, signing_us: 2.5, verification_us: 1, warning: 'MQ estimate lowered' },
				{ name: 'b', level: 3, pk: 200, sig: 120, signing_cycles: 25000, verification_cycles: 10000, warning: 'MQ estimate lowered' }
			]
		}
	]
};

const KEM: KemSchemeYaml = {
	name: 'ToyKEM',
	website: 'not a url',
	category: 'Lattices',
	assumption: 'MLWE',
	status: 'Research proposal',
	programmes: { 'china-ngcc': 'round-1' },
	implementation: 'Decapsulation bug (Credit, ngcc.dev).',
	findings_url: 'javascript:alert(1)',
	parametersets: [{ name: 'k1', ngcc_level: 128, pk: 800, ct: 100 }]
};

const HISTORY: HistoryEntry[] = [
	{ date: '2025-01-01', type: 'update', description: 'v2 out', schemes: ['Toy'] },
	{ date: '2025-06-01', type: 'attack', description: 'KEM attack', schemes: ['ToyKEM'] }
];

describe('signaturePage', () => {
	const page = signaturePage(SIG, HISTORY, REGISTRY);

	it('describes the scheme from its newest version', () => {
		expect(page).toMatchObject({
			primitive: 'signature',
			name: 'Toy',
			slug: 'toy',
			websiteHost: 'toy.example',
			latest: { id: '2025-01-01', label: 'v2', date: '2025-01-01' },
			chips: [{ label: 'On-ramp round 2', tone: 'plain' }],
			findings: { url: 'https://www.ngcc.dev/r.html', host: 'ngcc.dev' }
		});
		expect(page.options.map((o) => o.id)).toEqual(['2025-01-01', '2024-01-01']);
		expect(page.versions.map((v) => v.version)).toEqual(['v2', 'v1']);
	});

	it('builds the parameter-set table of the newest version', () => {
		expect(page.table.headers).toEqual(['pk (B)', 'sig (B)', 'pk+sig (B)', 'Sign', 'Verify']);
		expect(page.table.rows.map((r) => [r.name, r.level])).toEqual([['a', '1'], ['b', '3']]);
		expect(page.table.rows[0].cells.map((c) => c.text)).toEqual(['90', '60', '150', '2.5 µs', '1.0 µs']);
		expect(page.table.rows[1].cells[3]).toEqual({ text: '10.0 µs', kind: 'estimated', title: 'Estimated from 25K cycles @ 2.5 GHz' });
		expect(page.table.rows[0].badges.map((b) => b.kind)).toEqual(['warning']);
	});

	it('groups identical flags into one note and summarises them', () => {
		expect(page.flagNotes).toEqual([
			expect.objectContaining({ kind: 'warning', html: 'MQ estimate lowered', sets: ['a', 'b'], allSets: true })
		]);
		expect(page.flagSummary).toBe('Warning (all sets)');
	});

	it('merges the same-day update into the version event', () => {
		expect(page.events.map((e) => [e.date, e.kind, e.title, e.html])).toEqual([
			['2025-01-01', 'version', 'v2', 'v2 out'],
			['2024-01-01', 'version', 'v1', null]
		]);
	});
});

describe('kemPage', () => {
	const page = kemPage(KEM, HISTORY, REGISTRY);

	it('has programmes, status, an NGCC-scale table and no versions', () => {
		expect(page).toMatchObject({ primitive: 'kem', slug: 'toykem', status: 'Research proposal', websiteHost: null, findings: null });
		expect(page).not.toHaveProperty('versions');
		expect(page.chips).toEqual([{ label: 'NGCC round 1', tone: 'new' }]);
		expect(page.table.headers).toEqual(['pk (B)', 'ct (B)', 'pk+ct (B)', 'Keygen', 'Encaps', 'Decaps']);
		expect(page.table.rows[0].level).toBe('128-bit');
		expect(page.table.rows[0].cells.slice(3).map((c) => c.kind)).toEqual(['missing', 'missing', 'missing']);
	});

	it('never links a non-http findings URL', () => {
		expect(page.flagNotes).toEqual([expect.objectContaining({ kind: 'implementation', href: null, allSets: true })]);
		expect(page.flagSummary).toBe('Implementation findings (all sets)');
	});

	it('shows only the history entries naming the KEM', () => {
		expect(page.events.map((e) => e.html)).toEqual(['KEM attack']);
	});
});

describe('flagSummary', () => {
	it('says there are no flags, counts partial flags, and ignores notes', () => {
		expect(flagSummary([], 4)).toBe('None on current sets');
		expect(flagSummary([{ kind: 'note', title: 'Note', html: 'n', href: null, sets: ['a'], allSets: false }], 4)).toBe('None on current sets');
		expect(
			flagSummary(
				[
					{ kind: 'broken', title: 'Broken', html: 'x', href: null, sets: ['two'], allSets: false },
					{ kind: 'warning', title: 'Warning', html: 'y', href: null, sets: ['one'], allSets: false },
					{ kind: 'warning', title: 'Warning', html: 'z', href: null, sets: ['one', 'three'], allSets: false }
				],
				4
			)
		).toBe('Broken (1 of 4 sets) · Warning (2 of 4 sets)');
	});
});

describe('hostOf / timeCell', () => {
	it('strips www and rejects non-http URLs', () => {
		expect(hostOf('https://www.pqmayo.org/x')).toBe('pqmayo.org');
		expect(hostOf('ftp://a.example')).toBeNull();
		expect(hostOf('nonsense')).toBeNull();
	});

	it('marks unbenchmarked cells', () => {
		expect(timeCell(null, 0)).toEqual({ text: '—', kind: 'missing', title: 'Not yet benchmarked' });
	});
});
```

- [ ] **Step 2: Run test to verify it fails**

Run: `npm run test -- src/lib/__tests__/schemePage.test.ts`
Expected: FAIL, cannot resolve `$lib/schemePage`.

- [ ] **Step 3: Implement**

Create `src/lib/schemePage.ts`:

```ts
import { programmeChips, securityBadges, type Badge, type BadgeKind, type Chip } from './badges';
import { CPU_GHZ_LABEL, CYCLES_PER_US } from './constants';
import { processYamlSchemes } from './data';
import { fmt, fmtCycles, fmtTime } from './format';
import { processKemSchemes } from './kemData';
import { schemeProgrammes } from './programmes';
import { schemeEvents, type SchemeEvent } from './schemeHistory';
import { schemeSlug } from './slugs';
import { levelText, versionOptions, type VersionOption } from './versionCompare';
import type {
	HistoryEntry,
	KemParameterSet,
	KemSchemeYaml,
	ParameterSet,
	ProgrammeYaml,
	SchemeYaml,
	VersionYaml
} from './types';

export interface TableCell {
	text: string;
	kind: 'value' | 'estimated' | 'missing';
	title?: string;
}

export interface ParamSetRow {
	key: string;
	name: string;
	level: string;
	/** Shown only when the designers name recommended sets and this is one. */
	recommendedTag: boolean;
	badges: Badge[];
	cells: TableCell[];
}

export interface ParamSetTableModel {
	headers: string[];
	rows: ParamSetRow[];
}

export interface FlagNote extends Badge {
	sets: string[];
	allSets: boolean;
}

interface PageCommon {
	name: string;
	slug: string;
	website: string;
	websiteHost: string | null;
	category: string;
	assumption: string;
	chips: Chip[];
	flagSummary: string;
	findings: { url: string; host: string } | null;
	table: ParamSetTableModel;
	flagNotes: FlagNote[];
	events: SchemeEvent[];
}

export interface SignaturePage extends PageCommon {
	primitive: 'signature';
	latest: VersionOption;
	/** Newest first, aligned with `options`. */
	versions: VersionYaml[];
	options: VersionOption[];
}

export interface KemPage extends PageCommon {
	primitive: 'kem';
	status: string;
}

export type SchemePageData = SignaturePage | KemPage;

export function hostOf(url: string): string | null {
	try {
		const u = new URL(url);
		if (u.protocol !== 'https:' && u.protocol !== 'http:') return null;
		return u.hostname.replace(/^www\./, '');
	} catch {
		return null;
	}
}

function findingsLink(url: string | null | undefined): { url: string; host: string } | null {
	if (!url) return null;
	const host = hostOf(url);
	return host ? { url, host } : null;
}

const num = (n: number): TableCell => ({ text: fmt(n), kind: 'value' });

/** Measured µs, else estimated from measured cycles, else "not yet benchmarked". */
export function timeCell(us: number | null, cycles: number): TableCell {
	if (us != null) return { text: fmtTime(us), kind: 'value' };
	if (cycles > 0) {
		return {
			text: fmtTime(cycles / CYCLES_PER_US),
			kind: 'estimated',
			title: `Estimated from ${fmtCycles(cycles)} cycles @ ${CPU_GHZ_LABEL}`
		};
	}
	return { text: '—', kind: 'missing', title: 'Not yet benchmarked' };
}

export function signatureTable(rows: ParameterSet[]): ParamSetTableModel {
	const tagged = rows.some((r) => !r.recommended);
	return {
		headers: ['pk (B)', 'sig (B)', 'pk+sig (B)', 'Sign', 'Verify'],
		rows: rows.map((r) => ({
			key: r.parameterset,
			name: r.parameterset,
			level: levelText(r.level, r.ngccLevel),
			recommendedTag: tagged && r.recommended,
			badges: securityBadges(r),
			cells: [
				num(r.pk),
				num(r.sig),
				num(r.pkPlusSig),
				timeCell(r.signingUs, r.signingCycles),
				timeCell(r.verificationUs, r.verificationCycles)
			]
		}))
	};
}

export function kemTable(rows: KemParameterSet[]): ParamSetTableModel {
	const tagged = rows.some((r) => !r.recommended);
	return {
		headers: ['pk (B)', 'ct (B)', 'pk+ct (B)', 'Keygen', 'Encaps', 'Decaps'],
		rows: rows.map((r) => ({
			key: r.parameterset,
			name: r.parameterset,
			level: levelText(r.level, r.ngccLevel),
			recommendedTag: tagged && r.recommended,
			badges: securityBadges(r),
			cells: [
				num(r.pk),
				num(r.ct),
				num(r.pkPlusCt),
				timeCell(r.keygenUs, r.keygenCycles),
				timeCell(r.encapsUs, r.encapsCycles),
				timeCell(r.decapsUs, r.decapsCycles)
			]
		}))
	};
}

/** One note per distinct (kind, text), listing the parameter sets it applies to. */
export function flagNotes(rows: (Parameters<typeof securityBadges>[0] & { parameterset: string })[]): FlagNote[] {
	const notes: FlagNote[] = [];
	for (const r of rows) {
		for (const b of securityBadges(r)) {
			const hit = notes.find((n) => n.kind === b.kind && n.html === b.html);
			if (hit) {
				hit.sets.push(r.parameterset);
			} else {
				const href = b.href && /^https?:\/\//.test(b.href) ? b.href : null;
				notes.push({ ...b, href, sets: [r.parameterset], allSets: false });
			}
		}
	}
	for (const n of notes) n.allSets = n.sets.length === rows.length;
	return notes;
}

const SUMMARY_KINDS: BadgeKind[] = ['pre-quantum', 'broken', 'implementation', 'warning'];

export function flagSummary(notes: FlagNote[], total: number): string {
	const parts = SUMMARY_KINDS.flatMap((kind) => {
		const ofKind = notes.filter((n) => n.kind === kind);
		if (ofKind.length === 0) return [];
		const sets = new Set(ofKind.flatMap((n) => n.sets));
		const count = sets.size === total ? 'all sets' : `${sets.size} of ${total} sets`;
		return [`${ofKind[0].title} (${count})`];
	});
	return parts.length > 0 ? parts.join(' · ') : 'None on current sets';
}

export function signaturePage(yaml: SchemeYaml, history: HistoryEntry[], registry: ProgrammeYaml[]): SignaturePage {
	const versions = [...yaml.versions].sort((a, b) => b.date.localeCompare(a.date));
	const latest = versions[0];
	const options = versionOptions(versions);
	const { parameterSets } = processYamlSchemes([yaml]);
	const notes = flagNotes(parameterSets);
	return {
		primitive: 'signature',
		name: yaml.name,
		slug: schemeSlug(yaml),
		website: yaml.website,
		websiteHost: hostOf(yaml.website),
		category: yaml.category,
		assumption: yaml.assumption,
		chips: programmeChips(
			{ status: latest.status, version: latest.version, programmes: schemeProgrammes(yaml.versions) },
			registry
		),
		flagSummary: flagSummary(notes, parameterSets.length),
		findings: findingsLink(latest.findings_url),
		table: signatureTable(parameterSets),
		flagNotes: notes,
		events: schemeEvents(history, yaml.name, versions),
		latest: options[0],
		versions,
		options
	};
}

export function kemPage(yaml: KemSchemeYaml, history: HistoryEntry[], registry: ProgrammeYaml[]): KemPage {
	const { schemes, parameterSets } = processKemSchemes([yaml]);
	const notes = flagNotes(parameterSets);
	return {
		primitive: 'kem',
		name: yaml.name,
		slug: schemeSlug(yaml),
		website: yaml.website,
		websiteHost: hostOf(yaml.website),
		category: yaml.category,
		assumption: yaml.assumption,
		chips: programmeChips({ status: yaml.status, programmes: schemes[0]?.programmes ?? [] }, registry),
		flagSummary: flagSummary(notes, parameterSets.length),
		findings: findingsLink(yaml.findings_url),
		table: kemTable(parameterSets),
		flagNotes: notes,
		events: schemeEvents(history, yaml.name, []),
		status: yaml.status
	};
}
```

Notes for the implementer:
- `processKemSchemes` skips a KEM with no parameter sets, so `schemes[0]` may be undefined. Hence the `?? []`.
- If `npm run check` rejects `Parameters<typeof securityBadges>[0]`, replace it with an explicit `{ classical: boolean; broken: false | string; implementation: false | string; warning: false | string; info: false | string; findingsUrl: string | null; parameterset: string }`.

- [ ] **Step 4: Run tests to verify they pass**

Run: `npm run test && npm run check`
Expected: PASS, 0 errors.

- [ ] **Step 5: Commit**

```bash
git add src/lib/schemePage.ts src/lib/__tests__/schemePage.test.ts
git commit -m "Add the scheme page model for signatures and KEMs

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 5: Scheme route with header, parameter sets, flags and history

**Files:**
- Create: `src/routes/schemes/[slug]/+page.ts`, `src/routes/schemes/[slug]/+page.svelte`
- Create: `src/lib/components/scheme/SchemeHeader.svelte`, `ParamSetTable.svelte`, `FlagNotes.svelte`, `SchemeEvents.svelte`
- Test: `e2e/schemes.spec.ts`

**Interfaces:**
- Consumes: `schemeIndex` (Task 1), `allHistory` (Task 3), `signaturePage`, `kemPage`, `SchemePageData`, `ParamSetTableModel`, `FlagNote` (Task 4), `fmtDate`, `EVENT_LABELS`, `SchemeEvent` (Task 3), `allSchemeData`, `allKemData`, `allProgrammes`, `BadgeIcon`, `ProgrammeChips`.
- Produces: the route `/schemes/<slug>/` with load data `{ page: SchemePageData }`. Task 6 adds the compare section inside the `{#if page.primitive === 'signature'}` block of `+page.svelte`.

- [ ] **Step 1: Write the failing e2e test**

Create `e2e/schemes.spec.ts`:

```ts
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import yaml from 'js-yaml';
import { test, expect } from '@playwright/test';
import { buildSchemeIndex, schemeSlug } from '../src/lib/slugs';

const DATA = path.join(path.dirname(fileURLToPath(import.meta.url)), '..', 'data');
const readDir = <T>(dir: string): T[] =>
	fs
		.readdirSync(path.join(DATA, dir))
		.filter((f) => f.endsWith('.yaml'))
		.map((f) => yaml.load(fs.readFileSync(path.join(DATA, dir, f), 'utf8')) as T);

interface SourceScheme {
	name: string;
	slug?: string;
	versions: { parametersets: unknown[] }[];
}
const sigs = readDir<SourceScheme>('schemes');
const kems = readDir<{ name: string; slug?: string }>('kems');
const singleVersion = sigs
	.filter((s) => s.versions.length === 1)
	.sort((a, b) => a.name.localeCompare(b.name))[0];

test.describe('Scheme pages', () => {
	test('every signature and KEM has a prerendered page', async ({ request }) => {
		for (const ref of buildSchemeIndex(sigs, kems)) {
			const res = await request.get(`/schemes/${ref.slug}/`);
			expect(res.status(), ref.name).toBe(200);
		}
	});

	test('MAYO header: family, website, programme, latest version, flags', async ({ page }) => {
		await page.goto('/schemes/mayo/');
		await expect(page.locator('h1')).toHaveText('MAYO');
		const crumb = page.getByRole('navigation', { name: 'Breadcrumb' });
		await expect(crumb.getByRole('link', { name: 'Signatures' })).toHaveAttribute('href', '/signatures/');
		await expect(page.getByText('Multivariate · Multivariate Quadratic')).toBeVisible();
		await expect(page.getByRole('link', { name: /pqmayo\.org/ })).toHaveAttribute('href', 'https://pqmayo.org');
		await expect(page.getByText('On-ramp round 3')).toBeVisible();
		await expect(page.getByText('Round 3 · 31 Aug 2026').first()).toBeVisible();
		await expect(page.getByText('None on current sets')).toBeVisible();
	});

	test('MAYO parameter sets and history slice', async ({ page }) => {
		await page.goto('/schemes/mayo/');
		const table = page.getByRole('table', { name: 'Parameter sets of MAYO' });
		await expect(table.locator('tbody tr')).toHaveCount(4);
		const history = page.getByRole('region', { name: 'History' });
		await expect(history).toContainText('New version');
		await expect(history).toContainText('Wedge attack');
		await expect(history).not.toContainText('biased vinegar'); // SNOVA-only entry
	});

	test('a single-version scheme shows its sets and no comparison', async ({ page }) => {
		await page.goto(`/schemes/${schemeSlug(singleVersion)}/`);
		await expect(page.locator('h1')).toHaveText(singleVersion.name);
		await expect(page.getByRole('table', { name: `Parameter sets of ${singleVersion.name}` }).locator('tbody tr')).toHaveCount(
			singleVersion.versions[0].parametersets.length
		);
		await expect(page.getByRole('heading', { name: 'Compare versions' })).toHaveCount(0);
		await expect(page.getByText(/Only one version of .* is recorded/)).toBeVisible();
	});

	test('a KEM page has sets, programmes and no comparison', async ({ page }) => {
		await page.goto('/schemes/ml-kem/');
		await expect(page.locator('h1')).toHaveText('ML-KEM');
		await expect(page.getByRole('navigation', { name: 'Breadcrumb' }).getByRole('link', { name: 'KEMs' })).toBeVisible();
		const table = page.getByRole('table', { name: 'Parameter sets of ML-KEM' });
		await expect(table.getByRole('columnheader', { name: 'ct (B)' })).toBeVisible();
		await expect(table.getByRole('rowheader', { name: /ML-KEM-768/ })).toBeVisible();
		await expect(page.getByRole('heading', { name: 'Compare versions' })).toHaveCount(0);
	});
});
```

- [ ] **Step 2: Run to verify it fails**

Run: `npm run test:e2e -- e2e/schemes.spec.ts`
Expected: FAIL (404 for `/schemes/mayo/`).

- [ ] **Step 3: Implement the route**

Create `src/routes/schemes/[slug]/+page.ts`:

```ts
import { error } from '@sveltejs/kit';
import { allHistory } from '$lib/historyData';
import { allKemData } from '$lib/kemSchemeData';
import { allProgrammes } from '$lib/programmeData';
import { allSchemeData } from '$lib/schemeData';
import { schemeIndex } from '$lib/schemeIndex';
import { kemPage, signaturePage, type SchemePageData } from '$lib/schemePage';
import type { EntryGenerator, PageLoad } from './$types';

export const prerender = true;
export const trailingSlash = 'always';

/** Every scheme, including ones no compare view shows by default (e.g. round-1-only schemes). */
export const entries: EntryGenerator = () => schemeIndex.map((r) => ({ slug: r.slug }));

export const load: PageLoad = ({ params }): { page: SchemePageData } => {
	const ref = schemeIndex.find((r) => r.slug === params.slug);
	if (!ref) error(404, `No scheme page "${params.slug}"`);
	if (ref.primitive === 'signature') {
		const scheme = allSchemeData.find((s) => s.name === ref.name)!;
		return { page: signaturePage(scheme, allHistory, allProgrammes) };
	}
	const kem = allKemData.find((k) => k.name === ref.name)!;
	return { page: kemPage(kem, allHistory, allProgrammes) };
};
```

Create `src/lib/components/scheme/SchemeHeader.svelte`:

```svelte
<script lang="ts">
	import { base } from '$app/paths';
	import { fmtDate } from '$lib/dates';
	import type { SchemePageData } from '$lib/schemePage';
	import ProgrammeChips from '../ProgrammeChips.svelte';

	let { page }: { page: SchemePageData } = $props();

	const section = $derived(
		page.primitive === 'signature'
			? { href: 'signatures/', label: 'Signatures' }
			: { href: 'kems/', label: 'KEMs' }
	);
	const dt = 'font-heading text-xs font-semibold uppercase tracking-wide text-pqs-steel dark:text-pqs-bluegray';
</script>

<nav aria-label="Breadcrumb" class="font-heading text-sm text-pqs-steel dark:text-pqs-bluegray">
	<a href="{base}/{section.href}" class="underline-offset-2 hover:text-pqs-apricot hover:underline">{section.label}</a>
	<span aria-hidden="true" class="mx-1">/</span>
	<span aria-current="page">{page.name}</span>
</nav>

<h1 class="mt-2 font-heading text-3xl font-bold text-pqs-midnight dark:text-pqs-smoke">{page.name}</h1>
<p class="mt-1 text-pqs-steel dark:text-pqs-bluegray">
	{page.category} · {page.assumption}{#if page.websiteHost}
		· <a
			href={page.website}
			target="_blank"
			rel="noopener noreferrer"
			class="text-pqs-apricot underline-offset-2 hover:underline"
		>{page.websiteHost}<span class="sr-only"> (opens in a new tab)</span></a>
	{/if}
</p>

<dl class="mt-6 grid gap-4 rounded border border-pqs-ashgray bg-white p-4 sm:grid-cols-3 dark:border-pqs-steel dark:bg-pqs-midnight-mid">
	<div>
		<dt class={dt}>Programmes</dt>
		<dd class="mt-1"><ProgrammeChips chips={page.chips} /></dd>
	</div>
	{#if page.primitive === 'signature'}
		<div>
			<dt class={dt}>Latest version</dt>
			<dd class="mt-1 text-pqs-midnight dark:text-pqs-smoke">
				{page.latest.label} · <time datetime={page.latest.date}>{fmtDate(page.latest.date)}</time>
			</dd>
		</div>
	{:else}
		<div>
			<dt class={dt}>Status</dt>
			<dd class="mt-1 text-pqs-midnight dark:text-pqs-smoke">{page.status}</dd>
		</div>
	{/if}
	<div>
		<dt class={dt}>Security flags</dt>
		<dd class="mt-1 text-pqs-midnight dark:text-pqs-smoke">{page.flagSummary}</dd>
	</div>
</dl>

{#if page.findings}
	<p class="mt-3 text-sm text-pqs-steel dark:text-pqs-bluegray">
		Third-party findings:
		<a
			href={page.findings.url}
			target="_blank"
			rel="noopener noreferrer"
			class="text-pqs-apricot underline-offset-2 hover:underline"
		>full report on {page.findings.host}<span class="sr-only"> (opens in a new tab)</span></a>.
	</p>
{/if}
```

The test expects the page text `Round 3 · 31 Aug 2026`. The `<time>` element is inline, so `getByText` matches the `<dd>` text. If Playwright splits it, assert on the `dd` with `toContainText` instead.

Create `src/lib/components/scheme/ParamSetTable.svelte`:

```svelte
<script lang="ts">
	import type { ParamSetTableModel } from '$lib/schemePage';
	import BadgeIcon from '../BadgeIcon.svelte';

	let { table, caption }: { table: ParamSetTableModel; caption: string } = $props();
</script>

<div class="mt-3 overflow-x-auto rounded border border-pqs-ashgray shadow-sm dark:border-pqs-steel">
	<table class="min-w-full text-sm">
		<caption class="sr-only">{caption}</caption>
		<thead>
			<tr class="bg-pqs-steel font-heading text-xs text-white">
				<th scope="col" class="whitespace-nowrap px-3 py-2.5 text-left font-semibold">Parameter set</th>
				<th scope="col" class="whitespace-nowrap px-3 py-2.5 text-right font-semibold">Level</th>
				{#each table.headers as header (header)}
					<th scope="col" class="whitespace-nowrap px-3 py-2.5 text-right font-semibold">{header}</th>
				{/each}
			</tr>
		</thead>
		<tbody class="divide-y divide-pqs-ashgray bg-white dark:divide-pqs-steel dark:bg-pqs-midnight-mid">
			{#each table.rows as row (row.key)}
				<tr>
					<th scope="row" class="whitespace-nowrap px-3 py-1.5 text-left font-mono text-xs font-normal">
						{row.name}
						{#if row.recommendedTag}
							<span class="ml-1 rounded bg-pqs-ashgray px-1.5 py-px font-heading text-[11px] font-bold text-pqs-midnight dark:bg-pqs-steel/40 dark:text-pqs-smoke">Recommended</span>
						{/if}
						{#each row.badges as badge (badge.kind)}
							<span class="ml-1" title={badge.title}><BadgeIcon kind={badge.kind} /><span class="sr-only">{badge.title}</span></span>
						{/each}
					</th>
					<td class="px-3 py-1.5 text-right tabular-nums text-pqs-steel dark:text-pqs-bluegray">{row.level}</td>
					{#each row.cells as cell, i (i)}
						<td class="whitespace-nowrap px-3 py-1.5 text-right tabular-nums">
							{#if cell.kind === 'estimated'}
								<span class="underline decoration-wavy decoration-pqs-scarlet" title={cell.title}>{cell.text}<span class="sr-only"> ({cell.title})</span></span>
							{:else if cell.kind === 'missing'}
								<span class="text-pqs-bluegray" title={cell.title}>—<span class="sr-only"> {cell.title}</span></span>
							{:else}
								{cell.text}
							{/if}
						</td>
					{/each}
				</tr>
			{/each}
		</tbody>
	</table>
</div>
```

Create `src/lib/components/scheme/FlagNotes.svelte`:

```svelte
<script lang="ts">
	import type { BadgeKind } from '$lib/badges';
	import type { FlagNote } from '$lib/schemePage';
	import BadgeIcon from '../BadgeIcon.svelte';

	let { notes }: { notes: FlagNote[] } = $props();

	const TONE: Record<BadgeKind, string> = {
		'pre-quantum': 'bg-pqs-smoke dark:bg-pqs-steel/20',
		broken: 'bg-[#fdf2f2] dark:bg-[#3b1219]',
		implementation: 'bg-[#fff4ec] dark:bg-[#3a1d0c]',
		warning: 'bg-[#fdf6e7] dark:bg-[#33270a]',
		note: 'bg-pqs-smoke dark:bg-pqs-steel/20'
	};
</script>

{#if notes.length > 0}
	<section aria-labelledby="flags-heading" class="mt-8">
		<h2 id="flags-heading" class="font-heading text-xl font-bold text-pqs-midnight dark:text-pqs-smoke">Security flags</h2>
		<ul class="mt-3 space-y-2">
			{#each notes as note (note.kind + note.html)}
				<li class="rounded px-3 py-2.5 text-sm leading-relaxed text-pqs-midnight dark:text-pqs-smoke [&_a]:text-pqs-apricot [&_a]:underline {TONE[note.kind]}">
					<BadgeIcon kind={note.kind} />
					<strong class="font-heading">{note.title}</strong>
					<span class="text-pqs-steel dark:text-pqs-bluegray">({note.allSets ? 'all parameter sets' : note.sets.join(', ')})</span>:
					{@html note.html}
					{#if note.href}
						<a href={note.href} target="_blank" rel="noopener noreferrer">Full findings report<span class="sr-only"> (opens in a new tab)</span></a>
					{/if}
				</li>
			{/each}
		</ul>
	</section>
{/if}
```

Create `src/lib/components/scheme/SchemeEvents.svelte`:

```svelte
<script lang="ts">
	import { base } from '$app/paths';
	import { fmtDate } from '$lib/dates';
	import { EVENT_LABELS, type SchemeEvent, type SchemeEventKind } from '$lib/schemeHistory';

	let { events }: { events: SchemeEvent[] } = $props();

	const TONE: Record<SchemeEventKind, string> = {
		version: 'bg-pqs-ashgray text-pqs-midnight dark:bg-pqs-steel/40 dark:text-pqs-smoke',
		update: 'bg-blue-100 text-blue-800 dark:bg-blue-900/40 dark:text-blue-300',
		attack: 'bg-red-100 text-red-800 dark:bg-red-900/40 dark:text-red-300',
		standardization: 'bg-green-100 text-green-800 dark:bg-green-900/40 dark:text-green-300',
		milestone: 'bg-purple-100 text-purple-800 dark:bg-purple-900/40 dark:text-purple-300'
	};
</script>

<section aria-labelledby="history-heading" class="mt-10">
	<h2 id="history-heading" class="font-heading text-xl font-bold text-pqs-midnight dark:text-pqs-smoke">History</h2>
	{#if events.length === 0}
		<p class="mt-3 text-sm text-pqs-steel dark:text-pqs-bluegray">No events recorded for this scheme yet.</p>
	{:else}
		<ol class="mt-3">
			{#each events as event, i (i)}
				<li class="grid gap-1 border-t border-pqs-ashgray py-4 sm:grid-cols-[8rem_minmax(0,1fr)] sm:gap-6 dark:border-pqs-steel">
					<div>
						<time datetime={event.date} class="font-mono text-xs text-pqs-steel dark:text-pqs-bluegray">{fmtDate(event.date)}</time>
						<span class="mt-1 block w-fit rounded px-1.5 py-0.5 font-heading text-xs font-semibold {TONE[event.kind]}">{EVENT_LABELS[event.kind]}</span>
					</div>
					<div class="text-sm text-pqs-midnight dark:text-pqs-smoke [&_a]:text-pqs-apricot [&_a]:hover:underline">
						{#if event.title}<p class="font-heading font-semibold">{event.title}</p>{/if}
						{#if event.html}<p class="mt-1">{@html event.html}</p>{/if}
					</div>
				</li>
			{/each}
		</ol>
	{/if}
	<p class="mt-2 text-sm"><a href="{base}/timeline/" class="text-pqs-apricot hover:underline">Full timeline →</a></p>
</section>
```

Create `src/routes/schemes/[slug]/+page.svelte`:

```svelte
<script lang="ts">
	import { SITE_NAME } from '$lib/constants';
	import { fmtDate } from '$lib/dates';
	import FlagNotes from '$lib/components/scheme/FlagNotes.svelte';
	import ParamSetTable from '$lib/components/scheme/ParamSetTable.svelte';
	import SchemeEvents from '$lib/components/scheme/SchemeEvents.svelte';
	import SchemeHeader from '$lib/components/scheme/SchemeHeader.svelte';
	import type { PageData } from './$types';

	let { data }: { data: PageData } = $props();
	const page = $derived(data.page);
</script>

<svelte:head>
	<title>{page.name} — {SITE_NAME}</title>
	<meta name="description" content="{page.name}: parameter sets, sizes, timings, programmes, security flags and history." />
</svelte:head>

<!-- Keyed so that back/forward between two scheme pages resets per-page state (e.g. the version selects). -->
{#key page.slug}
	<div class="mx-auto max-w-screen-lg px-4 py-8 sm:px-6">
		<SchemeHeader {page} />

		<section aria-labelledby="sets-heading" class="mt-10">
			<h2 id="sets-heading" class="font-heading text-xl font-bold text-pqs-midnight dark:text-pqs-smoke">Parameter sets</h2>
			<p class="mt-1 text-sm text-pqs-steel dark:text-pqs-bluegray">
				{#if page.primitive === 'signature'}
					{page.latest.label} · {fmtDate(page.latest.date)}
				{:else}
					{page.status}
				{/if}
			</p>
			<ParamSetTable table={page.table} caption="Parameter sets of {page.name}" />
		</section>

		<FlagNotes notes={page.flagNotes} />

		{#if page.primitive === 'signature' && page.options.length < 2}
			<p class="mt-10 text-sm text-pqs-steel dark:text-pqs-bluegray">
				Only one version of {page.name} is recorded, so there is nothing to compare yet.
			</p>
		{/if}

		<SchemeEvents events={page.events} />
	</div>
{/key}
```

The `History` section uses `aria-labelledby`, so `getByRole('region', { name: 'History' })` resolves.

- [ ] **Step 4: Run tests**

Run: `npm run check && npm run test && npm run test:e2e`
Expected: all pass. The build prerenders every `/schemes/<slug>/` page (look for them in `dist/schemes/`).

- [ ] **Step 5: Commit**

```bash
git add src/routes/schemes src/lib/components/scheme/SchemeHeader.svelte src/lib/components/scheme/ParamSetTable.svelte src/lib/components/scheme/FlagNotes.svelte src/lib/components/scheme/SchemeEvents.svelte e2e/schemes.spec.ts
git commit -m "Add prerendered scheme pages for signatures and KEMs

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 6: Compare versions section

**Files:**
- Create: `src/lib/components/scheme/VersionCompare.svelte`
- Modify: `src/routes/schemes/[slug]/+page.svelte`
- Test: `e2e/schemes.spec.ts` (append)

**Interfaces:**
- Consumes: `compareVersions`, `formatChange`, `COMPARE_METRICS`, `METRIC_LABELS`, `FLAG_TITLES`, `CompareMode`, `MetricValue`, `ParamSetComparison`, `VersionOption` (Task 2); `fmt`, `fmtCycles`; `fmtDate`; `stripHtml`; `CPU_GHZ_LABEL`; `SignaturePage.versions` / `.options` (Task 4).
- Produces: `<VersionCompare versions={VersionYaml[]} options={VersionOption[]} />`.

- [ ] **Step 1: Write the failing e2e tests**

Append to `e2e/schemes.spec.ts`:

```ts
test.describe('Compare versions (MAYO)', () => {
	const compareTable = (page: import('@playwright/test').Page) =>
		page.getByRole('table', { name: /^Change from / });
	const setRow = (page: import('@playwright/test').Page, name: string) =>
		compareTable(page).locator('tbody tr', { has: page.getByRole('rowheader', { name: new RegExp(`^${name}\\b`) }) });

	test.beforeEach(async ({ page }) => {
		await page.goto('/schemes/mayo/');
		await page.waitForLoadState('networkidle');
	});

	test('defaults to the previous version against the latest and tells same-label versions apart', async ({ page }) => {
		await expect(page.getByRole('heading', { name: 'Compare versions' })).toBeVisible();
		await expect(page.getByLabel('From', { exact: true })).toHaveValue('2025-03-26');
		await expect(page.getByLabel('From', { exact: true }).locator('option:checked')).toHaveText('Round 2 · 26 Mar 2025');
		await expect(page.getByLabel('From', { exact: true }).locator('option', { hasText: 'Round 2 · 5 Feb 2025' })).toHaveCount(1);
		await expect(page.getByLabel('To', { exact: true })).toHaveValue('2026-08-31');
		const two = setRow(page, 'two');
		await expect(two).toContainText('4,912');
		await expect(two).toContainText('2,928');
		await expect(two).toContainText('▼ 40%');
		await expect(two).toContainText('▲ 28%');
		await expect(two).toContainText('Flag cleared');
		await expect(setRow(page, 'three')).toContainText('same');
	});

	test('swap reverses the comparison', async ({ page }) => {
		await page.getByRole('button', { name: 'Swap From and To' }).click();
		await expect(page.getByLabel('From', { exact: true })).toHaveValue('2026-08-31');
		const two = setRow(page, 'two');
		await expect(two).toContainText('▲ 68%');
		await expect(two).toContainText('Broken');
	});

	test('cycles mode shows cycle counts with the benchmark-source caveat', async ({ page }) => {
		await page.getByRole('button', { name: 'Cycles' }).click();
		await expect(page.getByRole('button', { name: 'Cycles' })).toHaveAttribute('aria-pressed', 'true');
		await expect(compareTable(page).getByRole('columnheader', { name: 'Sign (cycles)' })).toBeVisible();
		await expect(setRow(page, 'two')).toContainText('▼ 72%');
		const note = page.getByTestId('perf-note');
		await expect(note).toContainText('submission document (Table 3.1');
		await expect(note).toContainText('machines and implementations can differ');
	});

	test('picking an older From version re-matches the sets', async ({ page }) => {
		await page.getByLabel('From', { exact: true }).selectOption('2023-06-01');
		await expect(setRow(page, 'two')).toContainText('5,488');
		await expect(setRow(page, 'two')).not.toContainText('Flag cleared');
	});
});
```

Expected values: two pk 4912 → 2928 = −40.4%; sig 186 → 239 = +28.5%; swapped pk 2928 → 4912 = +67.8%; signing cycles 338917 → 95622 = −71.8%. MAYO "Round 1" `two` carries no flag, so no "Flag cleared" (see D3).

- [ ] **Step 2: Run to verify it fails**

Run: `npm run test:e2e -- e2e/schemes.spec.ts`
Expected: the new tests FAIL (no "Compare versions" heading).

- [ ] **Step 3: Implement**

Create `src/lib/components/scheme/VersionCompare.svelte`:

```svelte
<script lang="ts">
	import { CPU_GHZ_LABEL } from '$lib/constants';
	import { fmtDate } from '$lib/dates';
	import { fmt, fmtCycles } from '$lib/format';
	import { stripHtml } from '$lib/html';
	import type { VersionYaml } from '$lib/types';
	import {
		COMPARE_METRICS,
		FLAG_TITLES,
		METRIC_LABELS,
		compareVersions,
		formatChange,
		type CompareMode,
		type MetricValue,
		type ParamSetComparison,
		type VersionOption
	} from '$lib/versionCompare';

	let { versions, options }: { versions: VersionYaml[]; options: VersionOption[] } = $props();

	// The page wraps this component in {#key slug}, so reading props once at init is intended.
	let fromId = $state(options[1]?.id ?? options[0].id);
	let toId = $state(options[0].id);
	let mode = $state<CompareMode>('sizes');

	const indexOf = (id: string) => Math.max(0, options.findIndex((o) => o.id === id));
	const fromOpt = $derived(options[indexOf(fromId)]);
	const toOpt = $derived(options[indexOf(toId)]);
	const comparison = $derived(compareVersions(versions[indexOf(fromId)], versions[indexOf(toId)]));
	const metrics = $derived(COMPARE_METRICS[mode]);

	const WORDS = {
		sizes: { down: 'smaller', up: 'larger' },
		cycles: { down: 'faster', up: 'slower' }
	} as const;

	const optionLabel = (o: VersionOption) => `${o.label} · ${fmtDate(o.date)}`;
	const show = (v: MetricValue | null) => (v === null ? '—' : mode === 'sizes' ? fmt(v.value) : fmtCycles(v.value));

	function swap() {
		[fromId, toId] = [toId, fromId];
	}

	function flagLabel(r: ParamSetComparison): { text: string; title: string; tone: string } {
		if (r.flagChange === 'cleared' && r.fromFlag) {
			return { text: 'Flag cleared', title: `Was ${FLAG_TITLES[r.fromFlag.kind]}: ${stripHtml(r.fromFlag.html)}`, tone: 'text-pqs-steel dark:text-pqs-bluegray' };
		}
		const flag = r.toFlag ?? (r.presence === 'removed' ? r.fromFlag : null);
		if (!flag) return { text: '—', title: '', tone: 'text-pqs-bluegray' };
		const tone = flag.kind === 'broken' ? 'text-[#B91C1C] dark:text-[#EF4444]' : 'text-[#8a5208] dark:text-pqs-apricot';
		return { text: FLAG_TITLES[flag.kind], title: stripHtml(flag.html), tone };
	}

	const select =
		'h-11 min-w-0 max-w-full rounded-md border border-pqs-bluegray bg-white px-3 text-[15px] text-pqs-midnight dark:border-pqs-steel dark:bg-pqs-midnight-mid dark:text-pqs-smoke';
	const toggle = 'px-3 py-1.5 font-heading text-sm font-semibold';
</script>

{#snippet valueCell(v: MetricValue | null)}
	{#if v?.estimated}
		<span class="underline decoration-wavy decoration-pqs-scarlet" title="Estimated from µs at {CPU_GHZ_LABEL}">{show(v)}<span class="sr-only"> (estimated)</span></span>
	{:else}
		{show(v)}
	{/if}
{/snippet}

<section aria-labelledby="compare-heading" class="mt-10">
	<h2 id="compare-heading" class="font-heading text-xl font-bold text-pqs-midnight dark:text-pqs-smoke">Compare versions</h2>
	<p class="mt-1 text-sm text-pqs-steel dark:text-pqs-bluegray">Parameter sets are matched by name.</p>

	<div class="mt-4 flex flex-wrap items-end gap-3">
		<div class="flex flex-col gap-1">
			<label for="compare-from" class="font-heading text-xs font-semibold text-pqs-steel dark:text-pqs-bluegray">From</label>
			<select id="compare-from" bind:value={fromId} class={select}>
				{#each options as o (o.id)}<option value={o.id}>{optionLabel(o)}</option>{/each}
			</select>
		</div>
		<button
			type="button"
			onclick={swap}
			aria-label="Swap From and To"
			class="h-11 rounded-md border border-pqs-bluegray px-3 hover:bg-pqs-ashgray focus-visible:outline-2 focus-visible:outline-pqs-apricot dark:border-pqs-steel dark:hover:bg-pqs-steel/40"
		><span aria-hidden="true">⇄</span></button>
		<div class="flex flex-col gap-1">
			<label for="compare-to" class="font-heading text-xs font-semibold text-pqs-steel dark:text-pqs-bluegray">To</label>
			<select id="compare-to" bind:value={toId} class={select}>
				{#each options as o (o.id)}<option value={o.id}>{optionLabel(o)}</option>{/each}
			</select>
		</div>
		<div role="group" aria-label="Compare" class="ml-auto inline-flex overflow-hidden rounded-md border border-pqs-bluegray dark:border-pqs-steel">
			<button type="button" aria-pressed={mode === 'sizes'} onclick={() => (mode = 'sizes')}
				class="{toggle} {mode === 'sizes' ? 'bg-pqs-steel text-white' : 'text-pqs-steel dark:text-pqs-bluegray'}">Sizes</button>
			<button type="button" aria-pressed={mode === 'cycles'} onclick={() => (mode = 'cycles')}
				class="{toggle} {mode === 'cycles' ? 'bg-pqs-steel text-white' : 'text-pqs-steel dark:text-pqs-bluegray'}">Cycles</button>
		</div>
	</div>

	{#if fromId === toId}
		<p class="mt-3 text-sm text-pqs-steel dark:text-pqs-bluegray">From and To are the same version; pick two different versions to see changes.</p>
	{/if}

	<div class="mt-4 overflow-x-auto rounded border border-pqs-ashgray shadow-sm dark:border-pqs-steel">
		<table class="min-w-full text-sm">
			<caption class="sr-only">Change from {optionLabel(fromOpt)} to {optionLabel(toOpt)}</caption>
			<thead>
				<tr class="bg-pqs-steel font-heading text-xs text-white">
					<th scope="col" class="px-3 py-2.5 text-left font-semibold">Parameter set</th>
					<th scope="col" class="px-3 py-2.5 text-right font-semibold">Level</th>
					{#each metrics as key (key)}
						<th scope="col" class="whitespace-nowrap px-3 py-2.5 text-right font-semibold">{METRIC_LABELS[key]}</th>
						<th scope="col" class="px-3 py-2.5 text-right font-semibold">Change</th>
					{/each}
					<th scope="col" class="whitespace-nowrap px-3 py-2.5 text-left font-semibold">Security flag <span class="font-normal">(today's assessment)</span></th>
				</tr>
			</thead>
			<tbody class="divide-y divide-pqs-ashgray bg-white dark:divide-pqs-steel dark:bg-pqs-midnight-mid">
				{#each comparison.rows as r (r.name)}
					{@const flag = flagLabel(r)}
					<tr>
						<th scope="row" class="whitespace-nowrap px-3 py-1.5 text-left font-mono text-xs font-normal">
							{r.name}
							{#if r.presence !== 'both'}
								<span class="ml-1 rounded bg-pqs-ashgray px-1.5 py-px font-heading text-[11px] font-bold text-pqs-midnight dark:bg-pqs-steel/40 dark:text-pqs-smoke">{r.presence === 'added' ? `new in ${toOpt.label}` : `not in ${toOpt.label}`}</span>
							{/if}
						</th>
						<td class="whitespace-nowrap px-3 py-1.5 text-right tabular-nums text-pqs-steel dark:text-pqs-bluegray">
							{r.fromLevel && r.toLevel && r.fromLevel !== r.toLevel ? `${r.fromLevel} → ${r.toLevel}` : (r.toLevel ?? r.fromLevel)}
						</td>
						{#each metrics as key (key)}
							{@const m = r.metrics[key]}
							{@const change = formatChange(m.pct)}
							<td class="whitespace-nowrap px-3 py-1.5 text-right tabular-nums">
								{@render valueCell(m.from)} <span aria-hidden="true">→</span><span class="sr-only"> to </span> {@render valueCell(m.to)}
							</td>
							<td class="whitespace-nowrap px-3 py-1.5 text-right tabular-nums {change.direction === 'down' ? 'text-pqs-steel dark:text-pqs-bluegray' : change.direction === 'up' ? 'text-[#8a5208] dark:text-pqs-apricot' : 'text-pqs-bluegray'}">
								{#if change.symbol}<span aria-hidden="true">{change.symbol} </span>{/if}{change.text}{#if change.direction === 'down' || change.direction === 'up'}<span class="sr-only"> {WORDS[mode][change.direction]}</span>{/if}
							</td>
						{/each}
						<td class="whitespace-nowrap px-3 py-1.5 font-heading text-xs font-semibold {flag.tone}" title={flag.title || undefined}>{flag.text}</td>
					</tr>
				{/each}
			</tbody>
		</table>
	</div>

	<p class="mt-2 text-xs text-pqs-steel dark:text-pqs-bluegray">
		▼ smaller or faster, ▲ larger or slower.
		{#if mode === 'sizes'}Sizes in bytes.{/if}
		Flags are today's assessment of each version's parameter sets; the history below shows when each attack was published.
	</p>
	{#if mode === 'cycles'}
		<p data-testid="perf-note" class="mt-2 rounded bg-[#fdf6e7] px-3 py-2 text-xs text-pqs-midnight dark:bg-[#33270a] dark:text-pqs-smoke">
			Cycle counts as recorded for each version. {fromOpt.label} ({fmtDate(fromOpt.date)}): {comparison.perf.from ?? 'source not recorded'}.
			{toOpt.label} ({fmtDate(toOpt.date)}): {comparison.perf.to ?? 'source not recorded'}.
			{comparison.perf.same ? 'Both cite the same source, but' : 'The sources differ, and'} machines and implementations can differ between versions, so a change mixes the scheme's own changes with platform effects.
			{#if comparison.perf.anyEstimated}Wavy underline: estimated from µs at {CPU_GHZ_LABEL}.{/if}
		</p>
	{/if}
</section>
```

The e2e test expects lowercase "machines and implementations can differ". The sentence above keeps that exact phrase in both branches.

In `src/routes/schemes/[slug]/+page.svelte`, add the import next to the other component imports:

```svelte
	import VersionCompare from '$lib/components/scheme/VersionCompare.svelte';
```

and replace the single-version block:

```svelte
		{#if page.primitive === 'signature' && page.options.length < 2}
			<p class="mt-10 text-sm text-pqs-steel dark:text-pqs-bluegray">
				Only one version of {page.name} is recorded, so there is nothing to compare yet.
			</p>
		{/if}
```

with:

```svelte
		{#if page.primitive === 'signature'}
			{#if page.options.length > 1}
				<VersionCompare versions={page.versions} options={page.options} />
			{:else}
				<p class="mt-10 text-sm text-pqs-steel dark:text-pqs-bluegray">
					Only one version of {page.name} is recorded, so there is nothing to compare yet.
				</p>
			{/if}
		{/if}
```

- [ ] **Step 4: Run tests**

The selects use separate `<label for>` elements (not wrapping labels) and the tests use `exact: true`, because the swap button's name "Swap From and To" would otherwise also match `getByLabel('From')`.

Run: `npm run check && npm run test && npm run test:e2e`
Expected: all pass. svelte-check may warn that `$state(options…)` only captures the initial value. That warning is intended (keyed component), not an error.

- [ ] **Step 5: Commit**

```bash
git add src/lib/components/scheme/VersionCompare.svelte src/routes/schemes/[slug]/+page.svelte e2e/schemes.spec.ts
git commit -m "Add the version comparison to scheme pages

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 7: Table scheme names link to scheme pages

**Files:**
- Modify: `src/lib/components/SchemeTable.svelte`, `src/lib/components/KemTable.svelte`
- Test: `e2e/schemes.spec.ts` (append)

**Interfaces:**
- Consumes: `slugOf` (`$lib/schemeIndex`), `schemeHref` (`$lib/slugs`), `base` (`$app/paths`).
- Produces: the scheme-name cell is an internal link `${base}/schemes/<slug>/`. The external website link moves to the scheme page header.

- [ ] **Step 1: Write the failing e2e tests**

Append to `e2e/schemes.spec.ts`:

```ts
test.describe('Links to scheme pages', () => {
	test('a signature table row links to its scheme page', async ({ page }) => {
		await page.goto('/signatures/');
		await page.waitForLoadState('networkidle');
		const link = page.getByRole('link', { name: 'MAYO', exact: true }).first();
		await expect(link).toHaveAttribute('href', '/schemes/mayo/');
		await link.click();
		await expect(page.locator('h1')).toHaveText('MAYO', { timeout: 15_000 });
	});

	test('a KEM table row links to its scheme page', async ({ page }) => {
		await page.goto('/kems/');
		await page.waitForLoadState('networkidle');
		const link = page.getByRole('link', { name: 'ML-KEM', exact: true }).first();
		await expect(link).toHaveAttribute('href', '/schemes/ml-kem/');
		await link.click();
		await expect(page.locator('h1')).toHaveText('ML-KEM', { timeout: 15_000 });
	});
});
```

- [ ] **Step 2: Run to verify it fails**

Run: `npm run test:e2e -- e2e/schemes.spec.ts -g "Links to scheme pages"`
Expected: FAIL (href is `https://pqmayo.org`).

- [ ] **Step 3: Implement**

In **both** `src/lib/components/SchemeTable.svelte` and `src/lib/components/KemTable.svelte`, add to the `<script>` imports:

```ts
	import { base } from '$app/paths';
	import { slugOf } from '$lib/schemeIndex';
	import { schemeHref } from '$lib/slugs';
```

and replace the scheme-name anchor:

```svelte
							<a
								href={row.website}
								target="_blank"
								rel="noopener noreferrer"
								class="font-heading font-semibold text-pqs-steel hover:text-pqs-apricot dark:text-pqs-apricot dark:hover:text-pqs-apricot-light"
							>{row.scheme}</a>
```

with:

```svelte
							<a
								href={schemeHref(base, slugOf(row.scheme))}
								class="font-heading font-semibold text-pqs-steel hover:text-pqs-apricot dark:text-pqs-apricot dark:hover:text-pqs-apricot-light"
							>{row.scheme}</a>
```

Re-read both files first: plan 4 may also be editing these tables (see Overlaps). If the anchor has changed, keep the other changes and only swap `href`/`target`/`rel`.

- [ ] **Step 4: Run tests**

Run: `npm run check && npm run test && npm run test:e2e`
Expected: all pass. Existing `signatures.spec.ts` / `kems.spec.ts` / `advanced.spec.ts` tests don't rely on the external link (checked: none assert on `website` hrefs).

- [ ] **Step 5: Commit**

```bash
git add src/lib/components/SchemeTable.svelte src/lib/components/KemTable.svelte e2e/schemes.spec.ts
git commit -m "Link table scheme names to scheme pages

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 8: Chart points open scheme pages

**Files:**
- Create: `src/lib/chartNav.ts`
- Modify: `src/lib/components/ScatterPlot.svelte`, `src/lib/components/KemScatterPlot.svelte`
- Test: `src/lib/__tests__/chartNav.test.ts`, `e2e/schemes.spec.ts` (append)

**Interfaces:**
- Consumes: `slugOf`, `schemeHref`, `base`, `goto` (`$app/navigation`).
- Produces: `interface Point { x: number; y: number }`, `CLICK_SLOP_PX = 5`, `chartClickHref(datum: unknown, down: Point | null, up: Point): string | null`.

- [ ] **Step 1: Write the failing tests**

Create `src/lib/__tests__/chartNav.test.ts`:

```ts
import { describe, expect, it } from 'vitest';
import { chartClickHref } from '$lib/chartNav';

const DATUM = { scheme: 'MAYO', href: '/schemes/mayo/' };

describe('chartClickHref', () => {
	it('returns the point href for a plain click', () => {
		expect(chartClickHref(DATUM, { x: 10, y: 10 }, { x: 12, y: 13 })).toBe('/schemes/mayo/');
	});

	it('ignores a drag (pan) that ends on a point', () => {
		expect(chartClickHref(DATUM, { x: 10, y: 10 }, { x: 40, y: 10 })).toBeNull();
	});

	it('accepts a click when no mousedown was seen (e.g. a tap)', () => {
		expect(chartClickHref(DATUM, null, { x: 0, y: 0 })).toBe('/schemes/mayo/');
	});

	it('ignores background clicks and data without an href', () => {
		expect(chartClickHref(undefined, null, { x: 0, y: 0 })).toBeNull();
		expect(chartClickHref({ scheme: 'X' }, null, { x: 0, y: 0 })).toBeNull();
		expect(chartClickHref({ href: 42 }, null, { x: 0, y: 0 })).toBeNull();
		expect(chartClickHref({ href: '' }, null, { x: 0, y: 0 })).toBeNull();
	});
});
```

Append to `e2e/schemes.spec.ts`:

```ts
test.describe('Chart points open scheme pages', () => {
	for (const route of ['/signatures/', '/kems/']) {
		test(`clicking a point on ${route} opens a scheme page`, async ({ page }) => {
			await page.goto(route);
			await page.waitForFunction(() => document.querySelector('.mark-symbol path') !== null, { timeout: 15_000 });
			await page.waitForLoadState('networkidle');
			// Any point will do: overlapping marks all carry an href.
			await page.locator('.mark-symbol path').first().click({ force: true });
			await expect(page).toHaveURL(/\/schemes\/[a-z0-9-]+\/$/, { timeout: 15_000 });
			await expect(page.locator('h1')).not.toBeEmpty();
		});
	}
});
```

- [ ] **Step 2: Run to verify they fail**

Run: `npm run test -- src/lib/__tests__/chartNav.test.ts` → FAIL (module not found).
Run: `npm run test:e2e -- e2e/schemes.spec.ts -g "Chart points"` → FAIL (URL unchanged).

- [ ] **Step 3: Implement**

Create `src/lib/chartNav.ts`:

```ts
export interface Point {
	x: number;
	y: number;
}

/** Pointer travel (px) above which a mousedown→click is a pan, not a click. */
export const CLICK_SLOP_PX = 5;

/** The scheme-page href of a clicked chart datum, or null for drags, background and non-links. */
export function chartClickHref(datum: unknown, down: Point | null, up: Point): string | null {
	if (down && Math.hypot(up.x - down.x, up.y - down.y) > CLICK_SLOP_PX) return null;
	if (!datum || typeof datum !== 'object') return null;
	const href = (datum as { href?: unknown }).href;
	return typeof href === 'string' && href.length > 0 ? href : null;
}
```

In **both** `ScatterPlot.svelte` and `KemScatterPlot.svelte`:

1. Add imports:

```ts
	import { goto } from '$app/navigation';
	import { base } from '$app/paths';
	import { chartClickHref, type Point } from '$lib/chartNav';
	import { slugOf } from '$lib/schemeIndex';
	import { schemeHref } from '$lib/slugs';
```

2. In the `values` mapping, add `href` next to `scheme: d.scheme,`:

```ts
				href: schemeHref(base, slugOf(d.scheme)),
```

and add a tooltip line as the last entry of the `tooltip` object (after the `Notes` spread):

```ts
				'Scheme page': 'click the point'
```

3. Add `cursor: 'pointer'` to the three point marks. For example `mark: { type: 'point', filled: true, size: 100 }` becomes `mark: { type: 'point', filled: true, size: 100, cursor: 'pointer' }`, and the same for both ring layers (in `KemScatterPlot` the sizes are 120/140; keep them).

4. Replace the embed lines at the end of `doRender`:

```ts
		viewHandle?.finalize();
		viewHandle = null;
		viewHandle = await embed(container, spec, { actions: false, renderer: 'svg' });
```

with:

```ts
		viewHandle?.finalize();
		viewHandle = null;
		const result = await embed(container, spec, { actions: false, renderer: 'svg' });
		viewHandle = result;
		let down: Point | null = null;
		result.view.addEventListener('mousedown', (event) => {
			const e = event as MouseEvent;
			down = { x: e.clientX, y: e.clientY };
		});
		result.view.addEventListener('click', (event, item) => {
			const e = event as MouseEvent;
			const href = chartClickHref(item?.datum, down, { x: e.clientX, y: e.clientY });
			if (href) goto(href);
		});
```

Keyboard users reach the same pages through the table links (Task 7). The chart is a pointer convenience, so its SVG is not given focusable points.

- [ ] **Step 4: Run tests**

Run: `npm run check && npm run test && npm run test:e2e`
Expected: all pass, including the existing chart/zoom tests in `signatures.spec.ts`, `advanced.spec.ts`, `kems.spec.ts`.

- [ ] **Step 5: Commit**

```bash
git add src/lib/chartNav.ts src/lib/__tests__/chartNav.test.ts src/lib/components/ScatterPlot.svelte src/lib/components/KemScatterPlot.svelte e2e/schemes.spec.ts
git commit -m "Open the scheme page when a chart point is clicked

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 9: Documentation and history entry

**Files:**
- Modify: `CLAUDE.md`, `data/history.yaml`

- [ ] **Step 1: Update `CLAUDE.md`**

- In the scheme YAML schema block (under `assumption: ...`) and the KEM YAML block, add:

```yaml
slug: ntru-plus        # optional; URL slug for /schemes/<slug>/. Default: slugify(name). Needed only
                       # when two schemes (signature or KEM) clash or the name has no ASCII letters/digits
```

- In "Unit tests", add bullets:
  - `slugs.test.ts`: slugs and the cross-primitive index;
  - `versionCompare.test.ts`: version comparison (sets matched by name, percentage changes, flags per version, perf sources);
  - `schemeHistory.test.ts` and `dates.test.ts`: history slice and date format;
  - `schemePage.test.ts`: scheme page model;
  - `chartNav.test.ts`: chart click vs. drag.
- In "E2E tests", add: `schemes.spec.ts`: scheme pages, version comparison, links from tables and chart points.
- In the architecture tree, add `slugs.ts`, `schemeIndex.ts`, `versionCompare.ts`, `schemeHistory.ts`, `historyData.ts`, `dates.ts`, `schemePage.ts`, `chartNav.ts`, `components/scheme/`, and under `routes/`:

```
    ├── schemes/[slug]/   # one prerendered page per signature or KEM (entries() from schemeIndex):
    │                     # header, parameter sets, security flags, version comparison (signatures), history slice
```

- Add a section after "Programmes":

```markdown
### Scheme pages

`/schemes/<slug>/` serves signatures and KEMs from one route; `schemeIndex` (built by
`buildSchemeIndex()` in `src/lib/slugs.ts`) maps names to slugs and fails the build on a slug or
name clash across both primitives (fix with `slug:` in the YAML). Table scheme names and chart
points link there (`schemeHref(base, slugOf(name))`). The version comparison matches parameter sets
by name; flags shown per version are today's assessment of that version (see the history slice for
when attacks were published). Cycle comparisons always state each version's `perf_source`: machines
differ between versions. History entries reach a scheme page through their `schemes:` names.
```

- [ ] **Step 2: Add a history entry**

At the top of `data/history.yaml`, insert (date from `date -u +%F`):

```yaml
- date: '2026-10-07'
  type: update
  description: "Every signature scheme and KEM now has its own page, with its programmes, parameter sets, security flags, a version-by-version comparison of sizes and cycle counts, and its slice of this history. Scheme names in the tables and points in the charts link to it."
```

(No `schemes:` field: it concerns every scheme.)

- [ ] **Step 3: Verify**

Run: `npm run check && npm run test && npm run test:e2e`
Expected: all pass.

- [ ] **Step 4: Commit**

```bash
git add CLAUDE.md data/history.yaml
git commit -m "Document scheme pages

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

## Overlaps with concurrent plans

| Shared file / concern | Other plan | Handling |
|---|---|---|
| `src/lib/components/{SchemeTable,KemTable}.svelte` scheme-name link | Plan 4 (programme pages may reuse or restyle the tables) | Task 7 changes only the anchor's `href`/`target`/`rel`. Plan 4 should link names with `schemeHref(base, slugOf(name))` rather than define its own. |
| Links from programme pages to schemes | Plan 4 | Consume `$lib/schemeIndex` / `$lib/slugs`; do not create another slug function. |
| Links from scheme pages to `/programmes/<id>/` | Plan 4 | Not added here: the prerenderer fails on links to routes that don't exist yet. Whichever plan lands second wraps the header's programme chips in links. |
| Nav (`+layout.svelte`) | Plan 4 (adds "Programmes"), plan 6 | Untouched here. The mockup's "Search schemes" box is out of scope. |
| `data/schemes/*.yaml`, `data/kems/*.yaml` (NGCC) | Plan 3 | New files must pass the slug/name uniqueness test. A clash or non-ASCII name gets `slug:` (the plan-3 generator `ngcc_data.py` may need to pass through a `slug` from `candidates.yaml`). |
| `src/lib/types.ts` | Plans 3, 4 | One additive optional field `slug?` on two YAML interfaces. |
| `src/lib/__tests__/dataFiles.test.ts` | Plan 3 (may add checks) | Task 1 appends one `describe` block at the end. |
| `data/history.yaml`, `CLAUDE.md` | Plans 3, 4, 6 | Append-only edits at the top of history / to separate CLAUDE.md sections. Re-read before editing. |
| History loading (`src/routes/+page.ts`, `timeline/+page.ts`) | Plan 6 | Left as is. Plan 6 may switch both to `$lib/historyData` and link `schemes:` names on `/timeline/` to scheme pages via `schemeHref`. |

## Self-Review

- **Spec coverage:**
  - Shared route, slugs and justification: D1, D2, Task 1, Task 5.
  - Links from every table and chart: Tasks 7 and 8; the advanced pages reuse the same components.
  - Pure version comparison with sets matched by name, added/removed sets, percentage changes and flags per version: Task 2.
  - Flag model evaluated and deferred with reasoning: D3, and the column wording in Task 6.
  - `perf_source` caveat: D4, Task 2 `PerfNote`, Task 6 note.
  - History slice from `schemes:`: Task 3, Task 4.
  - KEM pages without a comparison: Tasks 4 and 5.
  - Single-version schemes: Task 5 e2e and the `options.length` branch.
  - `findings_url` with credit: D5, Tasks 4 and 5.
  - `entries()` prerender: Task 5.
  - Base path: `schemeHref` tests and every link built with `base`.
  - Accessibility: captions, `scope`, labels, `aria-pressed`, screen-reader text for ▲/▼ and new tabs, breadcrumb `nav`.
  - British spelling: "programme", "Standardisation".
  - Single branch, small commits, no third-party numbers: Global Constraints.
- **Placeholder scan:** none. Every code step has full code.
- **Type consistency:** these names match across tasks: `VersionOption.id`, `SignaturePage.options` / `.versions` / `.latest`, `ParamSetTableModel.headers` / `.rows`, `FlagNote.sets` / `.allSets`, `SchemeEvent.kind` / `.title` / `.html`, `chartClickHref(datum, down, up)`, `schemeHref(base, slug)` and `slugOf(name)`.
- **Review Focus:** each of the five items is pinned by a named test in its owning task.
