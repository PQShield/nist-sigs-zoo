# Site Reorganization: Programme Pages Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a `/programmes/` index and one prerendered page per programme (`/programmes/nist-pqc/`, `nist-onramp`, `china-ngcc`, `kpqc`, `iso`). Each page shows a breadcrumb, a title block, a facts box, a stage timeline, an optional security-level table, an optional findings summary and the programme's candidates (chart and table) with toggles. The home page's programme table links to these pages, and the nav gets "Programmes".

**Architecture:** The pages are driven by `data/programmes.yaml`, which gains optional page fields: description, links, timeline, security-level table, scope notes, official counts and findings source. All selection and counting logic is pure and unit-tested in a new module, `src/lib/programmePage.ts`. It picks a programme's members, builds stage views, focuses an NGCC target, summarises findings, formats dates and builds compare links. The candidates sections reuse the plan-2 components unchanged:
- Signatures: `ScatterPlot` (which renders `ChartLegend`) and `SchemeTable`, fed by the existing `filterStore` singleton. The programme page owns the singleton while it is mounted (`createFilterStore()` with the programme's rows). Stage toggles rebuild it with `createFilterStore()`, as the round selector does, and NGCC target toggles set `ngccLevels`.
- KEMs: `KemScatterPlot` and `KemTable` with `rows` props, plus a local `KemFilterState` built with `defaultKemFilter()` / `filterKemRows()`.

**Tech Stack:** SvelteKit 2 + Svelte 5 runes, adapter-static, Tailwind v4 (`pqs-*` tokens), TypeScript, Vega-Lite via the existing chart components, Vitest, Playwright.

**Spec:** The design canvas boards "Programme: China NGCC" and "Sitemap and data model", copied as `.superpowers/spec/Programme.dc.html` and `.superpowers/spec/Sitemap.dc.html`. They are HTML mockups: use them for structure and wording, not for code. The NGCC rows, counts and findings on the canvas are illustrative only. The roadmap entry "Programme pages" is in `docs/superpowers/plans/2026-10-07-site-reorganization-foundation.md`, and the controller brief for plan 4 (Thom, 2026-10-07) lists the binding requirements repeated under Global Constraints.

## Global Constraints

- Everything stays on branch `feat/programmes-foundation` (single-branch project). Never switch branches. Small, focused commits, one per task. Stage specific files and never use `git add -A` or `git add .`. End every commit message with the line `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- Programme ids are exactly `nist-pqc`, `nist-onramp`, `china-ngcc`, `kpqc`, `iso`.
- Never import third-party benchmark numbers. Programme pages show only timings already in `data/` (from `bench/` and `bench-kem/`). Where there are none, the page says "sizes only" and the tables show "not yet benchmarked".
- No hard-coded finding counts. The findings summary is computed from scheme data (`broken`, `implementation`, `warning`, `findings_url`). Credit wording comes from `data/programmes.yaml`.
- Every programme page must render sensibly with **zero** members of a primitive (China NGCC before plan 3 lands). It shows an empty-state sentence instead of a chart or table, and never shows `Infinity`, `NaN` or a Vega error.
- E2E tests must pass both with and without NGCC data. Read the expected state from `data/schemes/*.yaml` the way `e2e/kems.spec.ts` does, and never assert NGCC rows exist.
- Static prerender: `/programmes/[id]/` lists every registry id in `entries()`. Trailing slashes (inherited from `src/routes/+layout.ts`). Every internal link is prefixed with `base` from `$app/paths`, because production builds with `BASE_PATH=/nist-sigs-zoo`.
- Level scales are never converted. NGCC rows are focused through `ngccLevels`, never through NIST `levels`.
- Old-URL compatibility is low priority: no `?r=` handling on programme pages.
- British spelling in copy and identifiers ("programme", "standardised", "organiser").
- Accessibility: toggles are `<fieldset>`s with a (visually hidden) `<legend>` and `aria-pressed` buttons. The breadcrumb is `<nav aria-label="Breadcrumb">` with `aria-current="page"`. Every section is a region named by its heading. Table row headers use `<th scope="row">`.
- Pure logic goes in `src/lib/programmePage.ts` with Vitest tests. Unit tests never import `$app/*`, `$lib/schemeData`, `$lib/kemSchemeData` or `$lib/programmeData`; build fixtures from YAML-shaped literals.
- Run `npm run check` and `npm run test` before every commit. Run `npm run test:e2e` in tasks that touch components or routes.
- **Keep clear of plan 3's files.** Plan 3 runs in parallel and owns `bench/`, `bench-kem/`, `bench-common/`, `scripts/`, `data/schemes/*`, `data/kems/*`, `data/ngcc/*`, `data/history.yaml`, `src/lib/__tests__/dataFiles.test.ts`, `src/lib/__tests__/ngccCandidates.test.ts`, `e2e/{signatures,kems,home}.spec.ts`. Plan 4 must not edit any of them. Registry checks go in a new `programmeRegistry.test.ts`, and the home-page link test goes in the new `e2e/programmes.spec.ts`. The only shared file is `CLAUDE.md`, in different sections. Before editing it (Task 6), re-read it and run `git log -1 -- CLAUDE.md`.
- Another agent commits to this branch. Before editing any existing file, re-read it. Replace the exact snippets this plan names, and never overwrite whole existing files except the ones this plan creates.

## Review Focus

1. **A programme with no members of a primitive (China NGCC before plan 3 imports data):** the page must show "No signatures from this programme are in our data yet." with no chart, table, `Infinity` range or console error. The findings box must say "No candidates in our data yet." Pinned by Task 2's empty-input tests (`programmeSignatureRows`, `findingsSummary`) and Task 4's data-conditional e2e test.
2. **Client-side navigation from a programme page to `/signatures/`:** the programme page owns the shared `filterStore` singleton. `/signatures/` must then show its full data (ML-DSA present after visiting `/programmes/kpqc/`), not the programme subset. Pinned by Task 4's e2e test "leaving a programme page restores the full signature comparison".
3. **A candidate whose parameter sets carry different flags** (for example, design-broken only at 512-bit, an implementation finding at version level, and a findings report): it must be counted once, under its worst class, and once under "with a published findings report". Pre-quantum `broken: classical` never counts as a design break. Pinned by Task 2's `findingsSummary` tests.
4. **Deploying under `BASE_PATH=/nist-sigs-zoo`:** the compare links, breadcrumb, index cards and home-table links must all carry the base. Pinned by Task 2's `compareHref` tests, which pass a base. The components use `{base}` for every internal href.
5. **Partial or missing timeline dates** ("Round 2: not announced", a month-only `2025-01`): these must render without "Invalid Date" or `undefined`, and the "Stage" fact must fall back to the label. Pinned by Task 2's `formatDate` / `programmeFacts` tests and Task 1's registry test (date format, non-decreasing order, at most one current entry).

---

## File Structure

| File | Responsibility |
|---|---|
| `src/lib/types.ts` (modify) | `ProgrammeLinkYaml`, `ProgrammeTimelineEntry`, `SecurityLevelTableYaml`, `FindingsSourceYaml`; optional page fields on `ProgrammeYaml` |
| `data/programmes.yaml` (modify) | Page content for all five programmes |
| `src/lib/__tests__/programmeRegistry.test.ts` (create) | Validates the new registry fields in `data/programmes.yaml` |
| `src/lib/programmePage.ts` (create) | Pure helpers: members, stage views, rows per programme, NGCC targets, target focus, timings presence, findings summary, compare links, dates, facts, index cards |
| `src/lib/__tests__/programmePage.test.ts` (create) | Unit tests for the above |
| `src/lib/components/SegmentedToggle.svelte` (create) | Generic fieldset + `aria-pressed` button group |
| `src/lib/components/programme/ProgrammeView.svelte` (create) | Page composition: breadcrumb, title block, links, facts, sections, primitive toggle |
| `src/lib/components/programme/ProgrammeTimeline.svelte` (create) | Ordered list of timeline entries |
| `src/lib/components/programme/SecurityLevelsTable.svelte` (create) | Registry-driven level table |
| `src/lib/components/programme/FindingsSummary.svelte` (create) | Computed worst-finding counts per primitive with credit |
| `src/lib/components/programme/SignatureCandidates.svelte` (create) | Stage/target toggles, `ScatterPlot`, `SchemeTable` via the filter store, empty state, compare link |
| `src/lib/components/programme/KemCandidates.svelte` (create) | Target toggle, `KemScatterPlot`, `KemTable` with local filter, empty state, compare link |
| `src/routes/programmes/+page.ts`, `+page.svelte` (create) | Index with one card per programme |
| `src/routes/programmes/[id]/+page.ts`, `+page.svelte` (create) | `entries()`, 404 for unknown ids, `{#key}` wrapper |
| `src/routes/+layout.svelte` (modify) | "Programmes" nav item |
| `src/routes/+page.svelte` (modify) | Programme names link to programme pages |
| `e2e/programmes.spec.ts` (create) | E2E for index, pages, toggles, nav, home links |
| `CLAUDE.md` (modify) | Document registry fields, routes, module, tests |

---

### Task 1: Programme registry page fields

**Files:**
- Modify: `src/lib/types.ts` (the `ProgrammeYaml` block under `// Standardisation programmes (data/programmes.yaml)`)
- Modify: `data/programmes.yaml`
- Test: `src/lib/__tests__/programmeRegistry.test.ts` (create)

**Interfaces:**
- Consumes: the existing `ProgrammeYaml`, `ProgrammeStageYaml`.
- Produces (all new `ProgrammeYaml` fields optional, so existing fixtures in `programmes.test.ts` / `badges.test.ts` still type-check):
  ```ts
  interface ProgrammeLinkYaml { label: string; url: string }
  interface ProgrammeTimelineEntry { date?: string; label: string; note?: string; current?: boolean }
  interface SecurityLevelTableYaml { intro: string; note?: string; columns: string[]; rows: string[][] }
  interface FindingsSourceYaml { label: string; url: string; credit: string; note?: string }
  ProgrammeYaml += nav_label?, organiser_detail?, description?, links?, out_of_scope?,
                   candidates_heading?, official_counts?: { signature?: number; kem?: number },
                   timeline?, security_levels?, findings?
  ```

- [ ] **Step 1: Write the failing registry test**

Create `src/lib/__tests__/programmeRegistry.test.ts`:
```ts
import { describe, it, expect } from 'vitest';
import { readFileSync } from 'node:fs';
import { join } from 'node:path';
import yaml from 'js-yaml';
import type { ProgrammeYaml } from '$lib/types';

const programmes = yaml.load(
	readFileSync(join(process.cwd(), 'data', 'programmes.yaml'), 'utf8')
) as ProgrammeYaml[];
const byId = new Map(programmes.map((p) => [p.id, p]));
const DATE = /^\d{4}(-\d{2}(-\d{2})?)?$/;

describe('programme page fields in data/programmes.yaml', () => {
	it('gives every programme a nav label, description, candidates heading and timeline', () => {
		for (const p of programmes) {
			expect(p.nav_label, p.id).toBeTruthy();
			expect(p.description, p.id).toBeTruthy();
			expect(p.candidates_heading, p.id).toBeTruthy();
			expect(p.timeline?.length, p.id).toBeGreaterThan(0);
		}
	});

	it('uses ISO dates (year, month or day precision) in non-decreasing order', () => {
		for (const p of programmes) {
			const dates = (p.timeline ?? []).flatMap((e) => (e.date ? [e.date] : []));
			for (const d of dates) expect(d, `${p.id}: ${d}`).toMatch(DATE);
			expect(dates, p.id).toEqual([...dates].sort());
		}
	});

	it('marks at most one timeline entry as current', () => {
		for (const p of programmes) {
			expect((p.timeline ?? []).filter((e) => e.current).length, p.id).toBeLessThanOrEqual(1);
		}
	});

	it('links only to https URLs', () => {
		for (const p of programmes) {
			for (const l of p.links ?? []) expect(l.url, `${p.id}: ${l.label}`).toMatch(/^https:\/\//);
			if (p.findings) expect(p.findings.url, p.id).toMatch(/^https:\/\//);
		}
	});

	it('has one cell per column in every security-level row', () => {
		for (const p of programmes) {
			const t = p.security_levels;
			if (!t) continue;
			for (const row of t.rows) expect(row.length, `${p.id}: ${row[0]}`).toBe(t.columns.length);
		}
	});

	it('only gives official counts for primitives in scope', () => {
		for (const p of programmes) {
			for (const kind of Object.keys(p.official_counts ?? {})) {
				expect(p.primitives, `${p.id}: ${kind}`).toContain(kind);
			}
		}
	});

	it('lists the four China NGCC targets, with 384-bit optional', () => {
		const t = byId.get('china-ngcc')?.security_levels;
		expect(t?.rows.map((r) => r[0])).toEqual(['128-bit', '256-bit', '384-bit', '512-bit']);
		expect(t?.rows.map((r) => r[3])).toEqual(['Required', 'Required', 'Optional', 'Required']);
	});

	it('records the China NGCC round-1 dates', () => {
		const dated = Object.fromEntries(
			(byId.get('china-ngcc')?.timeline ?? []).flatMap((e) => (e.date ? [[e.label, e.date]] : []))
		);
		expect(dated).toEqual({
			'Programme launched': '2025-02-05',
			'Call for proposals': '2025-10-09',
			'Submissions closed': '2026-06-30',
			'Round 1': '2026-09-20'
		});
		expect(byId.get('china-ngcc')?.official_counts).toEqual({ signature: 34, kem: 41 });
	});
});
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `npx vitest run src/lib/__tests__/programmeRegistry.test.ts`
Expected: FAIL. "gives every programme a nav label…" fails with `expected undefined to be truthy`, and the NGCC tests fail.

- [ ] **Step 3: Add the types**

In `src/lib/types.ts`, replace the existing block:
```ts
export interface ProgrammeYaml {
	id: string;
	name: string;
	short: string;
	organiser: string;
	region: string;
	url?: string;
	primitives: ('signature' | 'kem')[];
	stages: ProgrammeStageYaml[];
}
```
with:
```ts
export interface ProgrammeLinkYaml {
	label: string;
	url: string;
}

/** One step on a programme page's timeline. `date` is YYYY, YYYY-MM or YYYY-MM-DD; omit when not announced. */
export interface ProgrammeTimelineEntry {
	date?: string;
	label: string;
	note?: string;
	/** The step the programme is at now (at most one per programme). */
	current?: boolean;
}

/** A programme's security-level scale, shown as a table. The first cell of each row is its row header. */
export interface SecurityLevelTableYaml {
	intro: string;
	note?: string;
	columns: string[];
	rows: string[][];
}

/** Where a programme's published findings come from, and how we credit them. */
export interface FindingsSourceYaml {
	label: string;
	url: string;
	credit: string;
	note?: string;
}

export interface ProgrammeYaml {
	id: string;
	name: string;
	short: string;
	organiser: string;
	region: string;
	url?: string;
	primitives: ('signature' | 'kem')[];
	stages: ProgrammeStageYaml[];
	// Programme page content (all optional; the page omits what is missing)
	/** Breadcrumb, card and tab title, e.g. "China NGCC". Falls back to `short`. */
	nav_label?: string;
	/** Facts-box organiser, e.g. "ICCS, Beijing". Falls back to `organiser`. */
	organiser_detail?: string;
	description?: string;
	links?: ProgrammeLinkYaml[];
	/** Facts-box "Not tracked" text, e.g. "9 key exchange, 35 hash functions". */
	out_of_scope?: string;
	candidates_heading?: string;
	/** Official candidate counts at the current stage, for "34 candidates · 0 in our data". */
	official_counts?: { signature?: number; kem?: number };
	timeline?: ProgrammeTimelineEntry[];
	security_levels?: SecurityLevelTableYaml;
	findings?: FindingsSourceYaml;
}
```

- [ ] **Step 4: Fill in the registry**

Replace the whole of `data/programmes.yaml` with the content below. It keeps every existing key and value: id, name, short, organiser, region, url, primitives and stages are unchanged. Re-read the file first, and if another agent has changed an existing key since this plan was written, keep their value. `&nist-levels` / `*nist-levels` is a YAML anchor, which js-yaml (used by the Vite plugin and the tests) resolves.

```yaml
# Standardisation programmes tracked on the site.
# Referenced from data/schemes/*.yaml (versions[].programmes) and data/kems/*.yaml (programmes).
# Page fields (nav_label … findings) drive /programmes/<id>/; see src/lib/types.ts (ProgrammeYaml).
- id: nist-pqc
  name: NIST post-quantum cryptography standardisation
  short: NIST
  organiser: NIST
  region: United States
  url: https://csrc.nist.gov/projects/post-quantum-cryptography
  primitives: [signature, kem]
  stages:
    - { id: round-1, label: round 1 }
    - { id: round-2, label: round 2 }
    - { id: round-3, label: round 3 }
    - { id: round-4, label: round 4 }
    - { id: selected, label: selected }
    - { id: standardised, label: standard }
  nav_label: NIST PQC
  organiser_detail: NIST, Gaithersburg
  description: >-
    NIST's public process to select quantum-resistant public-key algorithms, opened with a call
    for proposals in December 2016. It produced FIPS 203 (ML-KEM), FIPS 204 (ML-DSA) and FIPS 205
    (SLH-DSA) in August 2024. Falcon (FN-DSA) and HQC were also selected and are being
    standardised. Round-3 and round-4 KEMs that were not selected are listed for reference.
  links:
    - { label: NIST PQC project page, url: 'https://csrc.nist.gov/projects/post-quantum-cryptography' }
    - { label: Third-round report (NIST IR 8413), url: 'https://doi.org/10.6028/NIST.IR.8413' }
    - { label: Fourth-round report (NIST IR 8545), url: 'https://doi.org/10.6028/NIST.IR.8545' }
  candidates_heading: Standards and candidates
  timeline:
    - { date: '2016-12-20', label: Call for proposals }
    - { date: '2017-11-30', label: Submissions closed }
    - { date: '2017-12-21', label: Round 1, note: 69 candidates }
    - { date: '2019-01-30', label: Round 2, note: 26 candidates }
    - { date: '2020-07-22', label: Round 3, note: '7 finalists, 8 alternates' }
    - { date: '2022-07-05', label: First selections and round 4, note: '4 selected, 4 KEMs to round 4' }
    - { date: '2024-08-13', label: 'FIPS 203, 204 and 205 published' }
    - { date: '2025-03-11', label: HQC selected }
    - { label: FN-DSA and HQC standards, note: in progress, current: true }
  security_levels: &nist-levels
    intro: >-
      NIST defines five security categories by comparison with attacks on AES and SHA-2/SHA-3.
      Each parameter set claims a category. This site shows each scheme on its own programme's
      scale and does not convert between scales.
    columns: [Category, At least as hard to break as]
    rows:
      - ['1', Key search on AES-128]
      - ['2', Collision search on SHA-256 / SHA3-256]
      - ['3', Key search on AES-192]
      - ['4', Collision search on SHA-384 / SHA3-384]
      - ['5', Key search on AES-256]
- id: nist-onramp
  name: Additional digital signature schemes
  short: On-ramp
  organiser: NIST
  region: United States
  url: https://csrc.nist.gov/projects/pqc-dig-sig
  primitives: [signature]
  stages:
    - { id: round-1, label: round 1 }
    - { id: round-2, label: round 2 }
    - { id: round-3, label: round 3 }
  nav_label: NIST on-ramp
  organiser_detail: NIST, Gaithersburg
  description: >-
    NIST's call for additional post-quantum signature schemes, to diversify beyond lattice-based
    signatures, with interest in short signatures and fast verification. Use the stage toggle to
    see each round's entrants as submitted, or every entrant at its latest specification.
  links:
    - { label: On-ramp project page, url: 'https://csrc.nist.gov/projects/pqc-dig-sig' }
    - { label: First-round report (NIST IR 8528), url: 'https://doi.org/10.6028/NIST.IR.8528' }
  candidates_heading: Candidates
  official_counts: { signature: 9 }
  timeline:
    - { date: '2022-09-06', label: Call for proposals }
    - { date: '2023-06-01', label: Submissions closed }
    - { date: '2023-07-17', label: Round 1, note: 40 candidates }
    - { date: '2024-10-24', label: Round 2, note: 14 candidates }
    - { date: '2026-05-14', label: Round 3, note: 9 candidates, current: true }
  security_levels: *nist-levels
- id: china-ngcc
  name: Next-generation commercial cryptographic algorithms
  short: NGCC
  organiser: ICCS
  region: China
  url: https://www.niccs.org.cn/en/
  primitives: [signature, kem]
  stages:
    - { id: round-1, label: round 1 }
  nav_label: China NGCC
  organiser_detail: ICCS, Beijing
  description: >-
    China's open call for post-quantum public-key algorithms and hash functions, run by the
    Institute of Commercial Cryptography Standards (ICCS). Algorithms already standardised by NIST
    were not eligible. This site tracks the signatures and KEMs; the 9 key-exchange protocols and
    35 hash functions in round 1 are out of scope.
  links:
    - { label: ICCS programme page, url: 'https://www.niccs.org.cn/en/' }
    - { label: 'Independent analysis: ngcc.dev', url: 'https://ngcc.dev/' }
  out_of_scope: 9 key exchange, 35 hash functions
  candidates_heading: Round 1 candidates
  official_counts: { signature: 34, kem: 41 }
  timeline:
    - { date: '2025-02-05', label: Programme launched }
    - { date: '2025-10-09', label: Call for proposals }
    - { date: '2026-06-30', label: Submissions closed }
    - { date: '2026-09-20', label: Round 1, note: 119 candidates, current: true }
    - { label: Round 2, note: not announced }
  security_levels:
    intro: >-
      NGCC sets its targets in bits of classical security, each with a minimum quantum security.
      NIST categories are defined by comparison with brute-force attacks on AES and SHA, so the
      two scales do not line up. This site shows each scheme on its own programme's scale and
      does not convert between them.
    note: >-
      Even the lowest NGCC target asks for 128-bit classical security; the 512-bit target is
      beyond any NIST category.
    columns: [NGCC target, Classical (bits), Quantum (bits), Status]
    rows:
      - [128-bit, '128', '≥ 80', Required]
      - [256-bit, '256', '≥ 128', Required]
      - [384-bit, '384', '—', Optional]
      - [512-bit, '512', '≥ 256', Required]
  findings:
    label: ngcc.dev
    url: https://ngcc.dev/
    note: >-
      Many design findings cap only the 256- or 512-bit parameter sets, so a candidate with a
      design finding may still meet its 128-bit target.
    credit: >-
      Findings were published by Markku-Juhani O. Saarinen's evaluation effort and outside
      researchers; each report credits its authors. Our summaries are in our own words.
- id: kpqc
  name: Korean post-quantum cryptography competition
  short: KpqC
  organiser: KpqC
  region: South Korea
  url: https://kpqc.or.kr
  primitives: [signature, kem]
  stages:
    - { id: round-1, label: round 1 }
    - { id: round-2, label: round 2 }
    - { id: selected, label: selected }
  nav_label: KpqC
  description: >-
    South Korea's national post-quantum competition. In January 2025 it selected HAETAE and AIMer
    (signatures) and SMAUG-T and NTRU+ (KEMs). Only the selected schemes are in our data so far.
  links:
    - { label: KpqC competition site, url: 'https://kpqc.or.kr' }
  candidates_heading: Candidates
  official_counts: { signature: 2, kem: 2 }
  timeline:
    - { date: '2021-11', label: Competition launched }
    - { date: '2022-12', label: Round 1, note: 16 candidates }
    - { date: '2023-12', label: Round 2, note: 8 candidates }
    - { date: '2025-01', label: Final selection, note: '2 signatures, 2 KEMs', current: true }
- id: iso
  name: ISO/IEC 18033-2 amendment
  short: ISO
  organiser: ISO/IEC
  region: International
  primitives: [kem]
  stages:
    - { id: in-progress, label: in progress }
  nav_label: ISO/IEC 18033-2
  organiser_detail: ISO/IEC JTC 1/SC 27
  description: >-
    An amendment to ISO/IEC 18033-2 (asymmetric encryption) that adds post-quantum KEMs. Our data
    covers FrodoKEM and Classic McEliece, the conservative choices that NIST did not standardise.
  links:
    - { label: ISO/IEC 18033-2 at ISO, url: 'https://www.iso.org/standard/37971.html' }
  candidates_heading: Schemes in the amendment
  timeline:
    - { label: Amendment in preparation, current: true }
```

- [ ] **Step 5: Have Thom confirm the flagged values**

Before committing, send Thom the "Values for Thom to confirm" table at the end of this plan. Apply his answers to `data/programmes.yaml`, and delete any value he cannot confirm (every page field is optional except those Step 1 tests). If he does not answer, commit anyway and list the unconfirmed values in the commit body under `Unconfirmed:`.

- [ ] **Step 6: Run the tests to verify they pass**

Run: `npx vitest run src/lib/__tests__/programmeRegistry.test.ts src/lib/__tests__/programmes.test.ts && npm run check`
Expected: PASS, and 0 svelte-check errors. Also run `npm run test` (full suite). It must pass, because the existing `dataFiles.test.ts` checks of `programmes.yaml` still hold.

- [ ] **Step 7: Commit**

```bash
git add src/lib/types.ts data/programmes.yaml src/lib/__tests__/programmeRegistry.test.ts
git commit -m "Add programme page content to the programme registry

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 2: Pure programme-page helpers

**Files:**
- Create: `src/lib/programmePage.ts`
- Test: `src/lib/__tests__/programmePage.test.ts`

**Interfaces:**
- Consumes: `processYamlSchemes` (`$lib/data`), `processKemSchemes` (`$lib/kemData`), `versionProgrammes` / `STANDARDS_PROGRAMME` (`$lib/programmes`), `sigTimes` / `kemTimes` (`$lib/timing`), `NGCC_LEVELS` (`$lib/constants`), and the Task 1 types.
- Produces (used by Tasks 3–5):
  ```ts
  const CURRENT_VIEW = 'current';
  type Primitive = 'signature' | 'kem';
  interface StageView { id: string; label: string }
  signatureMembers(programmeId: string, sigs: SchemeYaml[]): SchemeYaml[]
  kemMembers(programmeId: string, kems: KemSchemeYaml[]): KemSchemeYaml[]
  stageViews(programme: ProgrammeYaml, sigs: SchemeYaml[]): StageView[]
  programmeSignatureRows(programmeId: string, sigs: SchemeYaml[], view: string): { parameterSets: ParameterSet[]; ranges: DataRanges }
  programmeKemRows(programmeId: string, kems: KemSchemeYaml[]): KemParameterSet[]
  ngccTargets(rows: { ngccLevel: NgccLevel | null }[]): NgccLevel[]
  unmappedAtTarget(rows: { scheme: string; ngccLevel: NgccLevel | null; recommended: boolean }[], target: NgccLevel): string[]
  focusTarget<T extends CommonFilter>(filter: T, target: NgccLevel | null): T
  anySigTimings(rows: ParameterSet[]): boolean
  anyKemTimings(rows: KemParameterSet[]): boolean
  type FindingClass = 'design' | 'implementation' | 'warning' | 'none'
  const FINDING_ORDER: readonly FindingClass[]; const FINDING_LABELS: Record<FindingClass, string>
  interface FindingRow { scheme; classical; broken; implementation; warning; findingsUrl }
  interface FindingsCount { candidates: number; design: number; implementation: number; warning: number; none: number; reports: number }
  findingsSummary(rows: FindingRow[]): FindingsCount
  compareHref(base: string, programmeId: string, kind: Primitive, opts?: { target?: NgccLevel | null; stage?: string }): string
  formatDate(date: string): string
  currentStage(programme: ProgrammeYaml): ProgrammeTimelineEntry | null
  stageText(programme: ProgrammeYaml): string | null
  interface Fact { term: string; detail: string }
  programmeFacts(programme: ProgrammeYaml, counts: { signature: number; kem: number }): Fact[]
  interface ProgrammeCard { programme: ProgrammeYaml; stage: string | null; signatures: number | null; kems: number | null }
  programmeCards(registry: ProgrammeYaml[], sigs: SchemeYaml[], kems: KemSchemeYaml[]): ProgrammeCard[]
  ```

- [ ] **Step 1: Write the failing tests**

Create `src/lib/__tests__/programmePage.test.ts`:
```ts
import { describe, it, expect } from 'vitest';
import {
	anyKemTimings,
	anySigTimings,
	compareHref,
	CURRENT_VIEW,
	currentStage,
	findingsSummary,
	focusTarget,
	formatDate,
	kemMembers,
	ngccTargets,
	programmeCards,
	programmeFacts,
	programmeKemRows,
	programmeSignatureRows,
	signatureMembers,
	stageViews,
	unmappedAtTarget,
	type FindingRow
} from '$lib/programmePage';
import { defaultCommonFilter } from '$lib/compareFilters';
import type {
	KemSchemeYaml,
	ParameterSetYaml,
	ProgrammeYaml,
	SchemeYaml,
	VersionYaml
} from '$lib/types';

function ver(
	date: string,
	programmes: Record<string, string | string[]> | undefined,
	sets: Partial<ParameterSetYaml>[] = [{}],
	extra: Partial<VersionYaml> = {}
): VersionYaml {
	return {
		version: date,
		date,
		status: 'On-ramp',
		programmes,
		parametersets: sets.map((p, i) => ({ name: `${date}-${i}`, pk: 10, sig: 20, ...p })),
		...extra
	};
}

function sig(name: string, versions: VersionYaml[]): SchemeYaml {
	return { name, website: 'https://example.org', category: 'Lattices', assumption: 'x', versions };
}

function kem(name: string, programmes?: Record<string, string | string[]>): KemSchemeYaml {
	return {
		name,
		website: 'https://example.org',
		category: 'Lattices',
		assumption: 'x',
		status: 'Candidate',
		programmes,
		parametersets: [{ name: `${name}-1`, pk: 800, ct: 700, ngcc_level: 128 }]
	};
}

const ONRAMP: ProgrammeYaml = {
	id: 'nist-onramp', name: 'Additional signatures', short: 'On-ramp', organiser: 'NIST', region: 'US',
	primitives: ['signature'],
	stages: [{ id: 'round-1', label: 'round 1' }, { id: 'round-2', label: 'round 2' }, { id: 'round-3', label: 'round 3' }]
};

const NGCC: ProgrammeYaml = {
	id: 'china-ngcc', name: 'Next-generation commercial cryptographic algorithms', short: 'NGCC',
	organiser: 'ICCS', region: 'China', primitives: ['signature', 'kem'],
	stages: [{ id: 'round-1', label: 'round 1' }],
	organiser_detail: 'ICCS, Beijing',
	out_of_scope: '9 key exchange, 35 hash functions',
	official_counts: { signature: 34, kem: 41 },
	timeline: [
		{ date: '2026-06-30', label: 'Submissions closed' },
		{ date: '2026-09-20', label: 'Round 1', current: true },
		{ label: 'Round 2', note: 'not announced' }
	]
};

const KPQC: ProgrammeYaml = {
	id: 'kpqc', name: 'KpqC', short: 'KpqC', organiser: 'KpqC', region: 'South Korea',
	primitives: ['signature', 'kem'], stages: [{ id: 'selected', label: 'selected' }]
};

const ISO: ProgrammeYaml = {
	id: 'iso', name: 'ISO/IEC 18033-2 amendment', short: 'ISO', organiser: 'ISO/IEC', region: 'International',
	primitives: ['kem'], stages: [{ id: 'in-progress', label: 'in progress' }],
	timeline: [{ label: 'Amendment in preparation', current: true }]
};

const MAYO = sig('MAYO', [
	ver('2023-06-01', { 'nist-onramp': 'round-1' }),
	ver('2025-02-05', { 'nist-onramp': 'round-2' }),
	ver('2026-08-01', undefined)
]);
const THREEWISE = sig('3WISE', [ver('2023-06-01', { 'nist-onramp': 'round-1' })]);
const MLDSA = sig('ML-DSA', [ver('2024-08-13', { 'nist-pqc': 'standardised' })]);
const AIMER = sig('AIMer', [ver('2023-06-01', { 'nist-onramp': 'round-1', kpqc: 'selected' })]);
const SIGS = [MAYO, THREEWISE, MLDSA, AIMER];

describe('members', () => {
	it('selects signature schemes with a version in the programme', () => {
		expect(signatureMembers('nist-onramp', SIGS).map((s) => s.name)).toEqual(['MAYO', '3WISE', 'AIMer']);
		expect(signatureMembers('kpqc', SIGS).map((s) => s.name)).toEqual(['AIMer']);
		expect(signatureMembers('china-ngcc', SIGS)).toEqual([]);
	});
	it('selects KEMs by their scheme-level memberships', () => {
		const kems = [kem('A', { kpqc: 'selected' }), kem('B', { 'nist-pqc': 'round-3' }), kem('C')];
		expect(kemMembers('kpqc', kems).map((k) => k.name)).toEqual(['A']);
		expect(kemMembers('iso', kems)).toEqual([]);
	});
});

describe('stageViews', () => {
	it('offers latest specs plus each round with recorded versions, most advanced first', () => {
		expect(stageViews(ONRAMP, SIGS)).toEqual([
			{ id: CURRENT_VIEW, label: 'All, latest specs' },
			{ id: 'round-2', label: 'Round 2' },
			{ id: 'round-1', label: 'Round 1' }
		]);
	});
	it('offers nothing when fewer than two rounds have versions', () => {
		const ngccSig = sig('BiT', [ver('2026-09-20', { 'china-ngcc': 'round-1' })]);
		expect(stageViews(NGCC, [ngccSig])).toEqual([]);
		expect(stageViews(NGCC, [])).toEqual([]);
	});
	it('ignores non-round stages such as "selected"', () => {
		expect(stageViews(KPQC, SIGS)).toEqual([]);
	});
});

describe('programmeSignatureRows', () => {
	it('shows every member at its newest version in the current view', () => {
		const rows = programmeSignatureRows('nist-onramp', SIGS, CURRENT_VIEW).parameterSets;
		expect(rows.map((r) => [r.scheme, r.version])).toEqual([
			['MAYO', '2026-08-01'],
			['3WISE', '2023-06-01'],
			['AIMer', '2023-06-01']
		]);
	});
	it('shows only that round’s entrants, pinned to the version recorded at the round', () => {
		const r1 = programmeSignatureRows('nist-onramp', SIGS, 'round-1').parameterSets;
		expect(r1.map((r) => [r.scheme, r.version])).toEqual([
			['MAYO', '2023-06-01'],
			['3WISE', '2023-06-01'],
			['AIMer', '2023-06-01']
		]);
		const r2 = programmeSignatureRows('nist-onramp', SIGS, 'round-2').parameterSets;
		expect(r2.map((r) => [r.scheme, r.version])).toEqual([['MAYO', '2025-02-05']]);
	});
	it('never includes reference schemes', () => {
		const rows = programmeSignatureRows('nist-onramp', SIGS, 'round-1').parameterSets;
		expect(rows.some((r) => r.scheme === 'ML-DSA')).toBe(false);
	});
	it('returns no rows for a programme without members', () => {
		expect(programmeSignatureRows('china-ngcc', SIGS, CURRENT_VIEW).parameterSets).toEqual([]);
		expect(programmeSignatureRows('china-ngcc', [], CURRENT_VIEW).parameterSets).toEqual([]);
	});
});

describe('programmeKemRows', () => {
	it('flattens the members’ parameter sets', () => {
		const rows = programmeKemRows('kpqc', [kem('A', { kpqc: 'selected' }), kem('B')]);
		expect(rows.map((r) => r.parameterset)).toEqual(['A-1']);
	});
	it('is empty without members', () => {
		expect(programmeKemRows('china-ngcc', [])).toEqual([]);
	});
});

describe('NGCC targets', () => {
	it('lists the targets present, in scale order', () => {
		expect(ngccTargets([{ ngccLevel: 512 }, { ngccLevel: null }, { ngccLevel: 128 }, { ngccLevel: 512 }])).toEqual([128, 512]);
		expect(ngccTargets([])).toEqual([]);
	});
	it('names schemes without a recommended set at a target', () => {
		const rows = [
			{ scheme: 'X', ngccLevel: 128 as const, recommended: true },
			{ scheme: 'Y', ngccLevel: 256 as const, recommended: true },
			{ scheme: 'Z', ngccLevel: 128 as const, recommended: false },
			{ scheme: 'Z', ngccLevel: 256 as const, recommended: true }
		];
		expect(unmappedAtTarget(rows, 128)).toEqual(['Y', 'Z']);
		expect(unmappedAtTarget(rows, 256)).toEqual(['X']);
	});
	it('focuses a filter on one target without touching NIST levels or mutating the input', () => {
		const f = defaultCommonFilter(['china-ngcc']);
		const focused = focusTarget(f, 256);
		expect([...focused.ngccLevels]).toEqual([256]);
		expect(focused.levels).toEqual(f.levels);
		expect(f.ngccLevels.size).toBe(4);
		expect(focusTarget(f, null)).toBe(f);
	});
});

describe('timings presence', () => {
	const base = { signingUs: null, verificationUs: null, signingCycles: 0, verificationCycles: 0 };
	it('detects signature timings, measured or from cycles', () => {
		expect(anySigTimings([])).toBe(false);
		expect(anySigTimings(programmeSignatureRows('nist-onramp', SIGS, CURRENT_VIEW).parameterSets)).toBe(false);
		const rows = programmeSignatureRows('nist-onramp', SIGS, CURRENT_VIEW).parameterSets;
		expect(anySigTimings([{ ...rows[0], ...base, signingCycles: 1000 }])).toBe(true);
	});
	it('detects KEM timings', () => {
		const rows = programmeKemRows('kpqc', [kem('A', { kpqc: 'selected' })]);
		expect(anyKemTimings(rows)).toBe(false);
		expect(anyKemTimings([{ ...rows[0], encapsUs: 12 }])).toBe(true);
	});
});

describe('findingsSummary', () => {
	const row = (scheme: string, f: Partial<FindingRow> = {}): FindingRow => ({
		scheme, classical: false, broken: false, implementation: false, warning: false, findingsUrl: null, ...f
	});
	it('counts each candidate once, under its worst finding', () => {
		const s = findingsSummary([
			row('A', { broken: 'design break at 512-bit' }),
			row('A', { implementation: 'bug', findingsUrl: 'https://ngcc.dev/reports/sign-01.html' }),
			row('A', { findingsUrl: 'https://ngcc.dev/reports/sign-01.html' }),
			row('B', { implementation: 'bug', findingsUrl: 'https://ngcc.dev/reports/sign-02.html' }),
			row('B', { warning: 'heads-up' }),
			row('C', { warning: 'heads-up' }),
			row('D')
		]);
		expect(s).toEqual({ candidates: 4, design: 1, implementation: 1, warning: 1, none: 1, reports: 2 });
	});
	it('does not count a pre-quantum scheme as broken by design', () => {
		expect(findingsSummary([row('RSA', { classical: true, broken: 'classical' })]).design).toBe(0);
	});
	it('is all zeros without candidates', () => {
		expect(findingsSummary([])).toEqual({ candidates: 0, design: 0, implementation: 0, warning: 0, none: 0, reports: 0 });
	});
});

describe('compareHref', () => {
	it('links to the compare page filtered to the programme, keeping the base', () => {
		expect(compareHref('/nist-sigs-zoo', 'china-ngcc', 'signature', { target: 128 })).toBe(
			'/nist-sigs-zoo/signatures/?p=china-ngcc&nl=128'
		);
		expect(compareHref('', 'kpqc', 'kem')).toBe('/kems/?p=kpqc');
	});
	it('maps NIST PQC onto both of its filter groups', () => {
		expect(compareHref('', 'nist-pqc', 'kem')).toBe('/kems/?p=nist-standards%2Cnist-pqc-rounds');
	});
	it('carries the on-ramp round for signatures only', () => {
		expect(compareHref('', 'nist-onramp', 'signature', { stage: 'round-2' })).toBe('/signatures/?p=nist-onramp&r=2');
		expect(compareHref('', 'nist-onramp', 'signature', { stage: CURRENT_VIEW })).toBe('/signatures/?p=nist-onramp');
		expect(compareHref('', 'china-ngcc', 'signature', { stage: 'round-1', target: null })).toBe('/signatures/?p=china-ngcc');
	});
});

describe('dates and facts', () => {
	it('formats day, month and year precision without a locale', () => {
		expect(formatDate('2026-09-20')).toBe('20 Sep 2026');
		expect(formatDate('2025-01')).toBe('Jan 2025');
		expect(formatDate('2021')).toBe('2021');
		expect(formatDate('soon')).toBe('soon');
	});
	it('finds the current timeline entry', () => {
		expect(currentStage(NGCC)?.label).toBe('Round 1');
		expect(currentStage(KPQC)).toBeNull();
	});
	it('builds the facts box with official and tracked counts', () => {
		expect(programmeFacts(NGCC, { signature: 0, kem: 41 })).toEqual([
			{ term: 'Organiser', detail: 'ICCS, Beijing' },
			{ term: 'Region', detail: 'China' },
			{ term: 'Stage', detail: 'Round 1, since 20 Sep 2026' },
			{ term: 'Signatures', detail: '34 candidates · 0 in our data' },
			{ term: 'KEMs', detail: '41 candidates' },
			{ term: 'Not tracked', detail: '9 key exchange, 35 hash functions' }
		]);
	});
	it('falls back to the organiser, an undated stage and plain counts', () => {
		expect(programmeFacts(ISO, { signature: 0, kem: 1 })).toEqual([
			{ term: 'Organiser', detail: 'ISO/IEC' },
			{ term: 'Region', detail: 'International' },
			{ term: 'Stage', detail: 'Amendment in preparation' },
			{ term: 'KEMs', detail: '1 scheme' }
		]);
	});
	it('builds index cards, with null counts for primitives out of scope', () => {
		const cards = programmeCards([NGCC, ISO], SIGS, [kem('F', { iso: 'in-progress' })]);
		expect(cards.map((c) => [c.programme.id, c.stage, c.signatures, c.kems])).toEqual([
			['china-ngcc', 'Round 1, since 20 Sep 2026', 0, 0],
			['iso', 'Amendment in preparation', null, 1]
		]);
	});
});
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `npx vitest run src/lib/__tests__/programmePage.test.ts`
Expected: FAIL with `Failed to resolve import "$lib/programmePage"`.

- [ ] **Step 3: Write the module**

Create `src/lib/programmePage.ts`:
```ts
import { NGCC_LEVELS } from './constants';
import { processYamlSchemes } from './data';
import { processKemSchemes } from './kemData';
import { STANDARDS_PROGRAMME, versionProgrammes } from './programmes';
import { kemTimes, sigTimes } from './timing';
import type {
	CommonFilter,
	DataRanges,
	KemParameterSet,
	KemSchemeYaml,
	NgccLevel,
	ParameterSet,
	ProgrammeTimelineEntry,
	ProgrammeYaml,
	SchemeYaml
} from './types';

/** Stage-view id meaning "every member at its newest version". */
export const CURRENT_VIEW = 'current';

export type Primitive = 'signature' | 'kem';

export interface StageView {
	id: string;
	label: string;
}

const ROUND = /^round-(\d+)$/;
const MONTHS = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];

const capitalise = (t: string) => t.charAt(0).toUpperCase() + t.slice(1);

function inProgramme(v: { programmes?: Record<string, string | string[]> }, programmeId: string): boolean {
	return programmeId in versionProgrammes(v);
}

function atStage(v: { programmes?: Record<string, string | string[]> }, programmeId: string, stage: string): boolean {
	return versionProgrammes(v)[programmeId]?.includes(stage) ?? false;
}

export function signatureMembers(programmeId: string, sigs: SchemeYaml[]): SchemeYaml[] {
	return sigs.filter((s) => s.versions.some((v) => inProgramme(v, programmeId)));
}

export function kemMembers(programmeId: string, kems: KemSchemeYaml[]): KemSchemeYaml[] {
	return kems.filter((k) => inProgramme(k, programmeId));
}

/**
 * Stage toggle for the signature candidates: "All, latest specs" plus every round at which a
 * member has a recorded version (most advanced first). Empty when fewer than two rounds have
 * versions, so there is nothing to toggle.
 */
export function stageViews(programme: ProgrammeYaml, sigs: SchemeYaml[]): StageView[] {
	const members = signatureMembers(programme.id, sigs);
	const rounds = [...programme.stages]
		.reverse()
		.filter(
			(stage) =>
				ROUND.test(stage.id) &&
				members.some((s) => s.versions.some((v) => atStage(v, programme.id, stage.id)))
		);
	if (rounds.length < 2) return [];
	return [
		{ id: CURRENT_VIEW, label: 'All, latest specs' },
		...rounds.map((s) => ({ id: s.id, label: capitalise(s.label) }))
	];
}

/**
 * Signature rows of a programme's members only (no reference schemes). The current view shows
 * each member's newest version; a stage view shows the entrants at that stage, pinned to the
 * version recorded there.
 */
export function programmeSignatureRows(
	programmeId: string,
	sigs: SchemeYaml[],
	view: string
): { parameterSets: ParameterSet[]; ranges: DataRanges } {
	const members = signatureMembers(programmeId, sigs);
	if (view === CURRENT_VIEW) {
		const { parameterSets, ranges } = processYamlSchemes(members);
		return { parameterSets, ranges };
	}
	const entrants = members.filter((s) => s.versions.some((v) => atStage(v, programmeId, view)));
	const { parameterSets, ranges } = processYamlSchemes(entrants, { programme: programmeId, stage: view });
	return { parameterSets, ranges };
}

export function programmeKemRows(programmeId: string, kems: KemSchemeYaml[]): KemParameterSet[] {
	return processKemSchemes(kemMembers(programmeId, kems)).parameterSets;
}

/** NGCC targets present in the rows, in scale order. */
export function ngccTargets(rows: { ngccLevel: NgccLevel | null }[]): NgccLevel[] {
	return NGCC_LEVELS.filter((l) => rows.some((r) => r.ngccLevel === l));
}

/** Schemes with no recommended parameter set at `target`, sorted. */
export function unmappedAtTarget(
	rows: { scheme: string; ngccLevel: NgccLevel | null; recommended: boolean }[],
	target: NgccLevel
): string[] {
	const mapped = new Set(rows.filter((r) => r.ngccLevel === target && r.recommended).map((r) => r.scheme));
	return [...new Set(rows.map((r) => r.scheme))].filter((s) => !mapped.has(s)).sort((a, b) => a.localeCompare(b));
}

/** Show one NGCC target only. NIST levels are left alone: the scales are never converted. */
export function focusTarget<T extends CommonFilter>(filter: T, target: NgccLevel | null): T {
	return target === null ? filter : { ...filter, ngccLevels: new Set<NgccLevel>([target]) };
}

export function anySigTimings(rows: ParameterSet[]): boolean {
	return rows.some((r) => {
		const t = sigTimes(r);
		return t.sign !== null || t.verify !== null;
	});
}

export function anyKemTimings(rows: KemParameterSet[]): boolean {
	return rows.some((r) => {
		const t = kemTimes(r);
		return t.keygen !== null || t.encaps !== null || t.decaps !== null;
	});
}

export type FindingClass = 'design' | 'implementation' | 'warning' | 'none';

export const FINDING_ORDER: readonly FindingClass[] = ['design', 'implementation', 'warning', 'none'];

export const FINDING_LABELS: Record<FindingClass, string> = {
	design: 'broken by design',
	implementation: 'implementation findings only',
	warning: 'security warnings only',
	none: 'nothing recorded'
};

/** The row fields the findings summary reads. ParameterSet and KemParameterSet both satisfy it. */
export interface FindingRow {
	scheme: string;
	classical: boolean;
	broken: false | string;
	implementation: false | string;
	warning: false | string;
	findingsUrl: string | null;
}

export interface FindingsCount {
	candidates: number;
	design: number;
	implementation: number;
	warning: number;
	none: number;
	/** Candidates with a link to a published findings report. */
	reports: number;
}

const RANK: Record<FindingClass, number> = { design: 3, implementation: 2, warning: 1, none: 0 };

function rowClass(r: FindingRow): FindingClass {
	if (r.broken !== false && !r.classical) return 'design';
	if (r.implementation !== false) return 'implementation';
	if (r.warning !== false) return 'warning';
	return 'none';
}

/** Worst recorded finding per candidate (scheme), counted once each. */
export function findingsSummary(rows: FindingRow[]): FindingsCount {
	const worst = new Map<string, FindingClass>();
	const reports = new Set<string>();
	for (const r of rows) {
		const c = rowClass(r);
		const prev = worst.get(r.scheme);
		if (prev === undefined || RANK[c] > RANK[prev]) worst.set(r.scheme, c);
		if (r.findingsUrl) reports.add(r.scheme);
	}
	const count: FindingsCount = { candidates: worst.size, design: 0, implementation: 0, warning: 0, none: 0, reports: reports.size };
	for (const c of worst.values()) count[c]++;
	return count;
}

/** Link to /signatures/ or /kems/ with the programme filter (and NGCC target / on-ramp round) set. */
export function compareHref(
	base: string,
	programmeId: string,
	kind: Primitive,
	opts: { target?: NgccLevel | null; stage?: string } = {}
): string {
	const params = new URLSearchParams();
	params.set('p', programmeId === STANDARDS_PROGRAMME ? 'nist-standards,nist-pqc-rounds' : programmeId);
	if (opts.target != null) params.set('nl', String(opts.target));
	const round = kind === 'signature' && programmeId === 'nist-onramp' ? ROUND.exec(opts.stage ?? '') : null;
	if (round) params.set('r', round[1]);
	return `${base}/${kind === 'signature' ? 'signatures' : 'kems'}/?${params}`;
}

/** "20 Sep 2026", "Jan 2025" or "2021"; anything else is returned unchanged. */
export function formatDate(date: string): string {
	const m = /^(\d{4})(?:-(\d{2})(?:-(\d{2}))?)?$/.exec(date);
	if (!m) return date;
	const [, year, month, day] = m;
	if (!month) return year;
	const mon = MONTHS[Number(month) - 1];
	return day ? `${Number(day)} ${mon} ${year}` : `${mon} ${year}`;
}

export function currentStage(programme: ProgrammeYaml): ProgrammeTimelineEntry | null {
	return programme.timeline?.find((e) => e.current) ?? null;
}

/** "Round 1, since 20 Sep 2026", or the bare label when undated; null without a current entry. */
export function stageText(programme: ProgrammeYaml): string | null {
	const cur = currentStage(programme);
	if (!cur) return null;
	return cur.date ? `${cur.label}, since ${formatDate(cur.date)}` : cur.label;
}

export interface Fact {
	term: string;
	detail: string;
}

function countText(tracked: number, official: number | undefined): string {
	if (official === undefined) return `${tracked} ${tracked === 1 ? 'scheme' : 'schemes'}`;
	const head = `${official} ${official === 1 ? 'candidate' : 'candidates'}`;
	return tracked === official ? head : `${head} · ${tracked} in our data`;
}

export function programmeFacts(programme: ProgrammeYaml, counts: { signature: number; kem: number }): Fact[] {
	const facts: Fact[] = [
		{ term: 'Organiser', detail: programme.organiser_detail ?? programme.organiser },
		{ term: 'Region', detail: programme.region }
	];
	const stage = stageText(programme);
	if (stage) facts.push({ term: 'Stage', detail: stage });
	const kinds = [['signature', 'Signatures'], ['kem', 'KEMs']] as const;
	for (const [kind, term] of kinds) {
		if (!programme.primitives.includes(kind)) continue;
		facts.push({ term, detail: countText(counts[kind], programme.official_counts?.[kind]) });
	}
	if (programme.out_of_scope) facts.push({ term: 'Not tracked', detail: programme.out_of_scope });
	return facts;
}

export interface ProgrammeCard {
	programme: ProgrammeYaml;
	stage: string | null;
	/** Members in our data; null when the primitive is out of the programme's scope. */
	signatures: number | null;
	kems: number | null;
}

export function programmeCards(registry: ProgrammeYaml[], sigs: SchemeYaml[], kems: KemSchemeYaml[]): ProgrammeCard[] {
	return registry.map((programme) => ({
		programme,
		stage: stageText(programme),
		signatures: programme.primitives.includes('signature') ? signatureMembers(programme.id, sigs).length : null,
		kems: programme.primitives.includes('kem') ? kemMembers(programme.id, kems).length : null
	}));
}
```

- [ ] **Step 4: Run the tests to verify they pass**

Run: `npx vitest run src/lib/__tests__/programmePage.test.ts && npm run test && npm run check`
Expected: PASS, and 0 svelte-check errors.

- [ ] **Step 5: Commit**

```bash
git add src/lib/programmePage.ts src/lib/__tests__/programmePage.test.ts
git commit -m "Add pure helpers for programme pages

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 3: Programme detail route (header, facts, timeline, levels, findings)

**Files:**
- Create: `src/lib/components/SegmentedToggle.svelte`
- Create: `src/lib/components/programme/{ProgrammeView,ProgrammeTimeline,SecurityLevelsTable,FindingsSummary}.svelte`
- Create: `src/routes/programmes/[id]/+page.ts`, `src/routes/programmes/[id]/+page.svelte`
- Test: `e2e/programmes.spec.ts` (create)

**Interfaces:**
- Consumes: from Task 2, `CURRENT_VIEW`, `findingsSummary`, `FINDING_ORDER`, `FINDING_LABELS`, `FindingsCount`, `formatDate`, `kemMembers`, `programmeFacts`, `programmeKemRows`, `programmeSignatureRows` and `signatureMembers`; from Task 1, the types.
- Produces: `SegmentedToggle` props `{ label: string; options: { value: T; label: string }[]; value: T; onchange: (v: T) => void }` (generic `T extends string | number`); `ProgrammeView` props `{ programme: ProgrammeYaml }`; the route `/programmes/<id>/` for every registry id. Section region names: "Timeline", "Security levels", "Published findings".

- [ ] **Step 1: Write the failing e2e tests**

Create `e2e/programmes.spec.ts`:
```ts
import { test, expect } from '@playwright/test';

const IDS = ['nist-pqc', 'nist-onramp', 'china-ngcc', 'kpqc', 'iso'];

test.describe('Programme pages', () => {
	for (const id of IDS) {
		test(`${id} page has a heading, breadcrumb and facts`, async ({ page }) => {
			await page.goto(`/programmes/${id}/`);
			await expect(page.locator('h1')).toBeVisible();
			const crumbs = page.getByRole('navigation', { name: 'Breadcrumb' });
			await expect(crumbs.getByRole('link', { name: 'Programmes' })).toHaveAttribute('href', /\/programmes\/$/);
			await expect(crumbs.locator('[aria-current="page"]')).toBeVisible();
			await expect(page.locator('main dl dt').first()).toHaveText('Organiser');
			await expect(page.getByRole('region', { name: 'Timeline' })).toBeVisible();
		});
	}

	test('China NGCC page shows its timeline, security targets and findings box', async ({ page }) => {
		await page.goto('/programmes/china-ngcc/');
		await expect(page.locator('h1')).toHaveText('Next-generation commercial cryptographic algorithms (NGCC)');
		const timeline = page.getByRole('region', { name: 'Timeline' });
		await expect(timeline).toContainText('Submissions closed');
		await expect(timeline).toContainText('30 Jun 2026');
		await expect(timeline.locator('[aria-current="step"]')).toContainText('Round 1');
		const levels = page.getByRole('region', { name: 'Security levels' });
		await expect(levels.locator('tbody th')).toHaveText(['128-bit', '256-bit', '384-bit', '512-bit']);
		const findings = page.getByRole('region', { name: 'Published findings' });
		await expect(findings).toContainText('Signatures');
		await expect(findings).toContainText('KEMs');
		await expect(findings.getByRole('link', { name: 'ngcc.dev' })).toHaveAttribute('href', 'https://ngcc.dev/');
	});

	test('NIST pages show the NIST category table and no findings box', async ({ page }) => {
		await page.goto('/programmes/nist-onramp/');
		await expect(page.getByRole('region', { name: 'Security levels' }).locator('tbody tr')).toHaveCount(5);
		await expect(page.getByRole('region', { name: 'Published findings' })).toHaveCount(0);
	});
});
```

- [ ] **Step 2: Run them to verify they fail**

Run: `npm run test:e2e -- e2e/programmes.spec.ts`
Expected: FAIL. The build's prerender has no `/programmes/` route, so the page shows SvelteKit's 404 and `h1` / breadcrumb assertions fail.

- [ ] **Step 3: Write `SegmentedToggle.svelte`**

Create `src/lib/components/SegmentedToggle.svelte`:
```svelte
<script lang="ts" generics="T extends string | number">
	let {
		label,
		options,
		value,
		onchange
	}: {
		label: string;
		options: { value: T; label: string }[];
		value: T;
		onchange: (value: T) => void;
	} = $props();
</script>

<fieldset class="inline-flex flex-wrap overflow-hidden rounded border border-pqs-bluegray dark:border-pqs-steel">
	<legend class="sr-only">{label}</legend>
	{#each options as o (o.value)}
		<button
			type="button"
			aria-pressed={value === o.value}
			onclick={() => onchange(o.value)}
			class="h-9 px-4 font-heading text-sm font-semibold {value === o.value
				? 'bg-pqs-steel text-white'
				: 'bg-white text-pqs-midnight hover:bg-pqs-smoke dark:bg-pqs-midnight-mid dark:text-pqs-smoke dark:hover:bg-pqs-steel/30'}"
		>{o.label}</button>
	{/each}
</fieldset>
```

- [ ] **Step 4: Write the section components**

Create `src/lib/components/programme/ProgrammeTimeline.svelte`:
```svelte
<script lang="ts">
	import { formatDate } from '$lib/programmePage';
	import type { ProgrammeTimelineEntry } from '$lib/types';

	let { entries }: { entries: ProgrammeTimelineEntry[] } = $props();
</script>

<ol class="grid gap-3 sm:grid-cols-2 lg:grid-cols-5">
	{#each entries as e, i (i)}
		<li
			aria-current={e.current ? 'step' : undefined}
			class="rounded border-t-4 bg-white px-3 py-2 dark:bg-pqs-midnight-mid {e.current
				? 'border-pqs-apricot'
				: 'border-pqs-ashgray dark:border-pqs-steel'}"
		>
			<p class="font-heading text-sm font-semibold">
				{e.label}{#if e.note}<span class="font-normal">: {e.note}</span>{/if}
			</p>
			{#if e.current || e.date}
				<p class="text-xs text-pqs-steel dark:text-pqs-bluegray">
					{#if e.current}<span class="font-semibold text-pqs-midnight dark:text-pqs-apricot">Current</span>{#if e.date} · {/if}{/if}
					{#if e.date}<time datetime={e.date}>{formatDate(e.date)}</time>{/if}
				</p>
			{/if}
		</li>
	{/each}
</ol>
```

Create `src/lib/components/programme/SecurityLevelsTable.svelte`:
```svelte
<script lang="ts">
	import type { SecurityLevelTableYaml } from '$lib/types';

	let { table }: { table: SecurityLevelTableYaml } = $props();
</script>

<p class="max-w-[80ch] text-sm text-pqs-steel dark:text-pqs-bluegray">{table.intro}</p>
<div class="mt-3 max-w-3xl overflow-x-auto rounded border border-pqs-ashgray bg-white dark:border-pqs-steel dark:bg-pqs-midnight-mid">
	<table class="min-w-full text-sm">
		<thead>
			<tr class="bg-pqs-steel font-heading text-xs text-white">
				{#each table.columns as c (c)}
					<th scope="col" class="px-3 py-2 text-left font-semibold">{c}</th>
				{/each}
			</tr>
		</thead>
		<tbody class="divide-y divide-pqs-ashgray dark:divide-pqs-steel">
			{#each table.rows as row, i (i)}
				<tr>
					{#each row as cell, j (j)}
						{#if j === 0}
							<th scope="row" class="px-3 py-2 text-left font-heading font-semibold">{cell}</th>
						{:else}
							<td class="px-3 py-2">{cell}</td>
						{/if}
					{/each}
				</tr>
			{/each}
		</tbody>
	</table>
</div>
{#if table.note}
	<p class="mt-2 max-w-[80ch] text-xs text-pqs-steel dark:text-pqs-bluegray">{table.note}</p>
{/if}
```

Create `src/lib/components/programme/FindingsSummary.svelte`:
```svelte
<script lang="ts">
	import { FINDING_LABELS, FINDING_ORDER, type FindingsCount } from '$lib/programmePage';
	import type { FindingsSourceYaml } from '$lib/types';

	let {
		source,
		groups
	}: { source: FindingsSourceYaml; groups: { label: string; count: FindingsCount }[] } = $props();
</script>

<p class="text-sm text-pqs-steel dark:text-pqs-bluegray">
	Worst recorded finding per candidate, from our data. Full reports:
	<a href={source.url} target="_blank" rel="noopener noreferrer" class="text-pqs-steel underline hover:text-pqs-apricot dark:text-pqs-apricot">{source.label}</a>.
</p>
<div class="mt-3 grid gap-4 md:grid-cols-2">
	{#each groups as g (g.label)}
		<div class="rounded border border-pqs-ashgray bg-white p-4 dark:border-pqs-steel dark:bg-pqs-midnight-mid">
			<h3 class="font-heading font-semibold">
				{g.label}
				<span class="font-normal text-pqs-steel dark:text-pqs-bluegray">· {g.count.candidates} {g.count.candidates === 1 ? 'candidate' : 'candidates'}</span>
			</h3>
			{#if g.count.candidates === 0}
				<p class="mt-2 text-sm">No candidates in our data yet.</p>
			{:else}
				<ul class="mt-2 space-y-1 text-sm">
					{#each FINDING_ORDER as c (c)}
						<li><span class="font-semibold tabular-nums">{g.count[c]}</span> {FINDING_LABELS[c]}</li>
					{/each}
					<li><span class="font-semibold tabular-nums">{g.count.reports}</span> with a published findings report</li>
				</ul>
			{/if}
		</div>
	{/each}
</div>
<p class="mt-3 max-w-[80ch] text-xs text-pqs-steel dark:text-pqs-bluegray">
	Design findings concern the specified scheme; implementation findings concern the submitted code.
	{#if source.note}{source.note}{/if}
	{source.credit}
</p>
```

- [ ] **Step 5: Write `ProgrammeView.svelte`**

Create `src/lib/components/programme/ProgrammeView.svelte`:
```svelte
<script lang="ts">
	import { untrack } from 'svelte';
	import { base } from '$app/paths';
	import { SITE_NAME } from '$lib/constants';
	import { allKemData } from '$lib/kemSchemeData';
	import { allSchemeData } from '$lib/schemeData';
	import {
		CURRENT_VIEW,
		findingsSummary,
		kemMembers,
		programmeFacts,
		programmeKemRows,
		programmeSignatureRows,
		signatureMembers
	} from '$lib/programmePage';
	import type { ProgrammeYaml } from '$lib/types';
	import FindingsSummary from './FindingsSummary.svelte';
	import ProgrammeTimeline from './ProgrammeTimeline.svelte';
	import SecurityLevelsTable from './SecurityLevelsTable.svelte';

	let { programme }: { programme: ProgrammeYaml } = $props();

	// The route remounts this component per programme ({#key}), so one read of the prop is enough.
	const p = untrack(() => programme);
	const hasSigs = p.primitives.includes('signature');
	const hasKems = p.primitives.includes('kem');
	const sigCount = signatureMembers(p.id, allSchemeData).length;
	const kemCount = kemMembers(p.id, allKemData).length;
	const facts = programmeFacts(p, { signature: sigCount, kem: kemCount });
	const navLabel = p.nav_label ?? p.short;
	const heading = p.name.includes(p.short) ? p.name : `${p.name} (${p.short})`;
	const findingGroups = [
		...(hasSigs
			? [{ label: 'Signatures', count: findingsSummary(programmeSignatureRows(p.id, allSchemeData, CURRENT_VIEW).parameterSets) }]
			: []),
		...(hasKems ? [{ label: 'KEMs', count: findingsSummary(programmeKemRows(p.id, allKemData)) }] : [])
	];

	const LINK = 'text-pqs-steel underline hover:text-pqs-apricot dark:text-pqs-apricot';
	const H2 = 'mb-3 font-heading text-2xl font-bold';
</script>

<svelte:head>
	<title>{navLabel} programme · {SITE_NAME}</title>
</svelte:head>

<div class="mx-auto max-w-screen-2xl space-y-12 px-6 py-8">
	<header>
		<nav aria-label="Breadcrumb" class="mb-4 font-heading text-sm">
			<ol class="flex flex-wrap gap-2 text-pqs-steel dark:text-pqs-bluegray">
				<li><a href="{base}/programmes/" class={LINK}>Programmes</a></li>
				<li aria-hidden="true">/</li>
				<li aria-current="page" class="font-semibold text-pqs-midnight dark:text-white">{navLabel}</li>
			</ol>
		</nav>
		<div class="grid gap-8 lg:grid-cols-[1fr_320px]">
			<div class="border-l-4 border-pqs-apricot pl-4">
				<h1 class="font-heading text-3xl font-bold text-pqs-midnight dark:text-white">{heading}</h1>
				{#if p.description}
					<p class="mt-3 max-w-[72ch] text-pqs-steel dark:text-pqs-bluegray">{p.description}</p>
				{/if}
				{#if p.links?.length}
					<ul class="mt-4 flex flex-wrap gap-x-5 gap-y-2 font-heading text-sm font-semibold">
						{#each p.links as l (l.url)}
							<li><a href={l.url} target="_blank" rel="noopener noreferrer" class={LINK}>{l.label} ↗</a></li>
						{/each}
					</ul>
				{/if}
			</div>
			<dl class="grid content-start gap-3 rounded border border-pqs-ashgray bg-white p-4 text-sm dark:border-pqs-steel dark:bg-pqs-midnight-mid">
				{#each facts as f (f.term)}
					<div>
						<dt class="font-heading text-xs font-semibold uppercase tracking-wide text-pqs-steel dark:text-pqs-bluegray">{f.term}</dt>
						<dd>{f.detail}</dd>
					</div>
				{/each}
			</dl>
		</div>
	</header>

	{#if p.timeline?.length}
		<section aria-labelledby="timeline-h">
			<h2 id="timeline-h" class={H2}>Timeline</h2>
			<ProgrammeTimeline entries={p.timeline} />
		</section>
	{/if}

	{#if p.security_levels}
		<section aria-labelledby="levels-h">
			<h2 id="levels-h" class={H2}>Security levels</h2>
			<SecurityLevelsTable table={p.security_levels} />
		</section>
	{/if}

	{#if p.findings}
		<section aria-labelledby="findings-h">
			<h2 id="findings-h" class={H2}>Published findings</h2>
			<FindingsSummary source={p.findings} groups={findingGroups} />
		</section>
	{/if}
</div>
```

- [ ] **Step 6: Write the route**

Create `src/routes/programmes/[id]/+page.ts`:
```ts
import { error } from '@sveltejs/kit';
import { allProgrammes } from '$lib/programmeData';
import type { EntryGenerator, PageLoad } from './$types';

export const prerender = true;

/** Prerender a page for every programme in data/programmes.yaml. */
export const entries: EntryGenerator = () => allProgrammes.map((p) => ({ id: p.id }));

export const load: PageLoad = ({ params }) => {
	const programme = allProgrammes.find((p) => p.id === params.id);
	if (!programme) error(404, `Unknown programme "${params.id}"`);
	return { programme };
};
```

Create `src/routes/programmes/[id]/+page.svelte`:
```svelte
<script lang="ts">
	import ProgrammeView from '$lib/components/programme/ProgrammeView.svelte';
	import type { PageData } from './$types';

	let { data }: { data: PageData } = $props();
</script>

<!-- Remount per programme: ProgrammeView and the candidate sections initialise once from their props. -->
{#key data.programme.id}
	<ProgrammeView programme={data.programme} />
{/key}
```

- [ ] **Step 7: Run checks and the e2e tests**

Run: `npm run check && npm run test && npm run test:e2e -- e2e/programmes.spec.ts`
Expected: 0 svelte-check errors (warnings about `state_referenced_locally` must not appear, because every top-level prop read goes through `untrack`). Unit tests PASS and e2e PASS. If a reused preview server serves a stale build, stop it first, because `reuseExistingServer` is on.

- [ ] **Step 8: Commit**

```bash
git add src/lib/components/SegmentedToggle.svelte src/lib/components/programme/ProgrammeView.svelte src/lib/components/programme/ProgrammeTimeline.svelte src/lib/components/programme/SecurityLevelsTable.svelte src/lib/components/programme/FindingsSummary.svelte 'src/routes/programmes/[id]/+page.ts' 'src/routes/programmes/[id]/+page.svelte' e2e/programmes.spec.ts
git commit -m "Add programme pages with timeline, security levels and findings summary

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 4: Candidates per primitive, stage and target

**Files:**
- Create: `src/lib/components/programme/SignatureCandidates.svelte`, `src/lib/components/programme/KemCandidates.svelte`
- Modify: `src/lib/components/programme/ProgrammeView.svelte` (replace the whole file; created in Task 3)
- Test: `e2e/programmes.spec.ts` (append)

**Interfaces:**
- Consumes: `createFilterStore` (`$lib/filterStore`, which returns `{ store, defaults, filteredRows }` with stable singleton references), `ScatterPlot` (props `xField`, `yField`; reads the filter store), `SchemeTable` (reads the filter store), `KemScatterPlot` (props `rows`, `xField`, `yField`), `KemTable` (prop `rows`), `ChartModeToggle` (props `mode`, `onchange`), `computeKemRanges` / `defaultKemFilter` / `filterKemRows` (`$lib/kemData`), `presentProgrammeGroups` (`$lib/compareFilters`), `SegmentedToggle` (Task 3), and from Task 2: `CURRENT_VIEW`, `stageViews`, `programmeSignatureRows`, `programmeKemRows`, `ngccTargets`, `focusTarget`, `unmappedAtTarget`, `anySigTimings`, `anyKemTimings`, `compareHref`.
- Produces: `SignatureCandidates` / `KemCandidates` props `{ programme: ProgrammeYaml }`. Region name = `programme.candidates_heading ?? 'Schemes'`. Toggle legends: "Primitive", "Stage", "Security target". Empty-state sentences: "No signatures from this programme are in our data yet." / "No KEMs from this programme are in our data yet." Link names: "Open in the signature comparison →" / "Open in the KEM comparison →".

- [ ] **Step 1: Write the failing e2e tests**

At the top of `e2e/programmes.spec.ts`, replace:
```ts
import { test, expect } from '@playwright/test';
```
with:
```ts
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import yaml from 'js-yaml';
import { test, expect } from '@playwright/test';

// NGCC data arrives with plan 3; expectations are read from data/ so these tests pass with or without it.
const SCHEMES_DIR = path.join(path.dirname(fileURLToPath(import.meta.url)), '..', 'data', 'schemes');

function signatureMemberCount(programme: string): number {
	return fs
		.readdirSync(SCHEMES_DIR)
		.filter((f) => f.endsWith('.yaml'))
		.filter((f) => {
			const s = yaml.load(fs.readFileSync(path.join(SCHEMES_DIR, f), 'utf8')) as {
				versions: { programmes?: Record<string, unknown> }[];
			};
			return s.versions.some((v) => v.programmes !== undefined && programme in v.programmes);
		}).length;
}
```
Then, inside `test.describe('Programme pages', …)`, after the last test, add:
```ts
	test('on-ramp stage toggle shows each round’s entrants', async ({ page }) => {
		await page.goto('/programmes/nist-onramp/');
		await page.waitForLoadState('networkidle');
		const candidates = page.getByRole('region', { name: 'Candidates' });
		const stages = candidates.getByRole('group', { name: 'Stage' });
		await expect(stages.getByRole('button', { name: 'All, latest specs' })).toHaveAttribute('aria-pressed', 'true');

		await stages.getByRole('button', { name: 'Round 1' }).click();
		await expect(stages.getByRole('button', { name: 'Round 1' })).toHaveAttribute('aria-pressed', 'true');
		await expect(candidates.locator('tbody')).toContainText('3WISE');
		await expect(candidates.getByRole('link', { name: /signature comparison/ })).toHaveAttribute('href', /\/signatures\/\?p=nist-onramp&r=1$/);

		await stages.getByRole('button', { name: 'Round 2' }).click();
		await expect(candidates.locator('tbody')).not.toContainText('3WISE');
		await expect(candidates.locator('tbody')).toContainText('MAYO');
	});

	test('NIST PQC primitive toggle switches between signatures and KEMs', async ({ page }) => {
		await page.goto('/programmes/nist-pqc/');
		await page.waitForLoadState('networkidle');
		const candidates = page.getByRole('region', { name: 'Standards and candidates' });
		const kinds = candidates.getByRole('group', { name: 'Primitive' });
		await expect(candidates.locator('tbody')).toContainText('ML-DSA');
		await kinds.getByRole('button', { name: /^KEMs/ }).click();
		await expect(kinds.getByRole('button', { name: /^KEMs/ })).toHaveAttribute('aria-pressed', 'true');
		await expect(candidates.locator('tbody')).toContainText('ML-KEM');
	});

	test('ISO page lists its KEMs without a primitive toggle', async ({ page }) => {
		await page.goto('/programmes/iso/');
		const candidates = page.getByRole('region', { name: 'Schemes in the amendment' });
		await expect(candidates.getByRole('group', { name: 'Primitive' })).toHaveCount(0);
		await expect(candidates.locator('tbody')).toContainText('FrodoKEM');
	});

	test('China NGCC candidates render with or without imported data', async ({ page }) => {
		await page.goto('/programmes/china-ngcc/');
		const candidates = page.getByRole('region', { name: 'Round 1 candidates' });
		if (signatureMemberCount('china-ngcc') === 0) {
			await expect(candidates).toContainText('No signatures from this programme are in our data yet.');
			await expect(candidates.locator('table')).toHaveCount(0);
		} else {
			await expect(candidates.locator('tbody tr').first()).toBeVisible();
			await expect(candidates.getByRole('link', { name: /signature comparison/ })).toHaveAttribute('href', /\/signatures\/\?p=china-ngcc/);
		}
	});

	test('leaving a programme page restores the full signature comparison', async ({ page }) => {
		await page.goto('/programmes/kpqc/');
		await page.waitForLoadState('networkidle');
		await expect(page.getByRole('region', { name: 'Candidates' }).locator('tbody')).not.toContainText('ML-DSA');
		await page.getByRole('banner').getByRole('link', { name: 'Signatures', exact: true }).click();
		await expect(page.locator('h1')).toHaveText('Post-Quantum Signature Schemes', { timeout: 15_000 });
		await expect(page.locator('tbody')).toContainText('ML-DSA');
	});
```

- [ ] **Step 2: Run them to verify they fail**

Run: `npm run test:e2e -- e2e/programmes.spec.ts`
Expected: the five new tests FAIL (no region named "Candidates", "Standards and candidates", and so on). The Task 3 tests still PASS.

- [ ] **Step 3: Write `SignatureCandidates.svelte`**

Create `src/lib/components/programme/SignatureCandidates.svelte`:
```svelte
<script lang="ts">
	import { untrack } from 'svelte';
	import { base } from '$app/paths';
	import { createFilterStore } from '$lib/filterStore';
	import {
		anySigTimings,
		compareHref,
		CURRENT_VIEW,
		focusTarget,
		ngccTargets,
		programmeSignatureRows,
		stageViews,
		unmappedAtTarget
	} from '$lib/programmePage';
	import { allSchemeData } from '$lib/schemeData';
	import type { NgccLevel, ProgrammeYaml } from '$lib/types';
	import ChartModeToggle from '../ChartModeToggle.svelte';
	import ScatterPlot from '../ScatterPlot.svelte';
	import SchemeTable from '../SchemeTable.svelte';
	import SegmentedToggle from '../SegmentedToggle.svelte';

	let { programme }: { programme: ProgrammeYaml } = $props();

	const p = untrack(() => programme);
	const views = stageViews(p, allSchemeData);
	const initial = programmeSignatureRows(p.id, allSchemeData, CURRENT_VIEW);
	const firstTarget = ngccTargets(initial.parameterSets)[0] ?? null;

	// ScatterPlot and SchemeTable read the shared filter store; this section owns it while mounted
	// (every compare page rebuilds it on load). createFilterStore() keeps the same store object.
	const { store } = createFilterStore(initial.parameterSets, initial.ranges, new URLSearchParams());
	store.update((f) => focusTarget(f, firstTarget));

	let view = $state(CURRENT_VIEW);
	let rows = $state(initial.parameterSets);
	let target = $state<NgccLevel | null>(firstTarget);

	const targets = $derived(ngccTargets(rows));
	const timed = $derived(anySigTimings(rows));
	const unmapped = $derived(target === null ? [] : unmappedAtTarget(rows, target));

	function chooseView(next: string) {
		view = next;
		const r = programmeSignatureRows(p.id, allSchemeData, next);
		rows = r.parameterSets;
		createFilterStore(r.parameterSets, r.ranges, new URLSearchParams());
		target = ngccTargets(r.parameterSets)[0] ?? null;
		store.update((f) => focusTarget(f, target));
	}

	function chooseTarget(next: NgccLevel) {
		target = next;
		store.update((f) => focusTarget(f, next));
	}
</script>

{#if initial.parameterSets.length === 0}
	<p class="rounded border border-dashed border-pqs-bluegray px-4 py-6 text-sm text-pqs-steel dark:border-pqs-steel dark:text-pqs-bluegray">
		No signatures from this programme are in our data yet.
	</p>
{:else}
	<div class="space-y-4">
		{#if views.length > 0 || targets.length > 1}
			<div class="flex flex-wrap items-center gap-3">
				{#if views.length > 0}
					<SegmentedToggle label="Stage" options={views.map((v) => ({ value: v.id, label: v.label }))} value={view} onchange={chooseView} />
				{/if}
				{#if targets.length > 1}
					<SegmentedToggle
						label="Security target"
						options={targets.map((t) => ({ value: t, label: `${t}-bit` }))}
						value={target ?? targets[0]}
						onchange={chooseTarget}
					/>
				{/if}
			</div>
		{/if}

		<div class="rounded border border-pqs-ashgray bg-white p-4 shadow-sm dark:border-pqs-steel dark:bg-pqs-midnight-mid">
			<div class="mb-3 flex flex-wrap items-center justify-between gap-3">
				<h3 class="font-heading text-base font-semibold text-pqs-steel dark:text-pqs-apricot">
					{$store.chart === 'speed' ? 'Signing vs. verification time' : 'pk size vs. sig size'}
					<span class="font-normal text-pqs-bluegray">(log–log scale)</span>
				</h3>
				{#if timed}
					<ChartModeToggle mode={$store.chart} onchange={(chart) => store.update((f) => ({ ...f, chart }))} />
				{/if}
			</div>
			<ScatterPlot
				xField={$store.chart === 'speed' ? 'signingTime' : 'pk'}
				yField={$store.chart === 'speed' ? 'verificationTime' : 'sig'}
			/>
		</div>

		{#if !timed}
			<p role="note" class="text-xs text-pqs-steel dark:text-pqs-bluegray">
				Sizes only for now: none of these candidates is in our benchmark harness yet, so the table
				shows them as not yet benchmarked. Timings will follow once they are added.
			</p>
		{/if}

		<SchemeTable />

		{#if unmapped.length > 0}
			<p class="text-xs text-pqs-steel dark:text-pqs-bluegray">
				No recommended {target}-bit parameter set in our data: {unmapped.join(', ')}.
			</p>
		{/if}

		<p class="text-right">
			<a href={compareHref(base, p.id, 'signature', { target, stage: view })} class="font-heading text-sm font-semibold text-pqs-steel hover:text-pqs-apricot dark:text-pqs-apricot">
				Open in the signature comparison →
			</a>
		</p>
	</div>
{/if}
```

- [ ] **Step 4: Write `KemCandidates.svelte`**

Create `src/lib/components/programme/KemCandidates.svelte`:
```svelte
<script lang="ts">
	import { untrack } from 'svelte';
	import { base } from '$app/paths';
	import { presentProgrammeGroups } from '$lib/compareFilters';
	import { computeKemRanges, defaultKemFilter, filterKemRows } from '$lib/kemData';
	import { allKemData } from '$lib/kemSchemeData';
	import {
		anyKemTimings,
		compareHref,
		focusTarget,
		ngccTargets,
		programmeKemRows,
		unmappedAtTarget
	} from '$lib/programmePage';
	import type { NgccLevel, ProgrammeYaml } from '$lib/types';
	import ChartModeToggle from '../ChartModeToggle.svelte';
	import KemScatterPlot from '../KemScatterPlot.svelte';
	import KemTable from '../KemTable.svelte';
	import SegmentedToggle from '../SegmentedToggle.svelte';

	let { programme }: { programme: ProgrammeYaml } = $props();

	const p = untrack(() => programme);
	const rows = programmeKemRows(p.id, allKemData);
	const targets = ngccTargets(rows);
	const timed = anyKemTimings(rows);
	const defaults = defaultKemFilter(
		computeKemRanges(rows),
		new Set(rows.map((r) => r.scheme)),
		presentProgrammeGroups(rows)
	);
	const firstTarget = targets[0] ?? null;

	let target = $state<NgccLevel | null>(firstTarget);
	let filter = $state(focusTarget(defaults, firstTarget));

	const filtered = $derived(filterKemRows(rows, filter));
	const unmapped = $derived(target === null ? [] : unmappedAtTarget(rows, target));

	function chooseTarget(next: NgccLevel) {
		target = next;
		filter = focusTarget(filter, next);
	}
</script>

{#if rows.length === 0}
	<p class="rounded border border-dashed border-pqs-bluegray px-4 py-6 text-sm text-pqs-steel dark:border-pqs-steel dark:text-pqs-bluegray">
		No KEMs from this programme are in our data yet.
	</p>
{:else}
	<div class="space-y-4">
		{#if targets.length > 1}
			<SegmentedToggle
				label="Security target"
				options={targets.map((t) => ({ value: t, label: `${t}-bit` }))}
				value={target ?? targets[0]}
				onchange={chooseTarget}
			/>
		{/if}

		<div class="rounded border border-pqs-ashgray bg-white p-4 shadow-sm dark:border-pqs-steel dark:bg-pqs-midnight-mid">
			<div class="mb-3 flex flex-wrap items-center justify-between gap-3">
				<h3 class="font-heading text-base font-semibold text-pqs-steel dark:text-pqs-apricot">
					{filter.chart === 'speed' ? 'Encapsulation vs. decapsulation time' : 'pk size vs. ciphertext size'}
					<span class="font-normal text-pqs-bluegray">(log–log scale)</span>
				</h3>
				{#if timed}
					<ChartModeToggle mode={filter.chart} onchange={(chart) => (filter = { ...filter, chart })} />
				{/if}
			</div>
			<KemScatterPlot
				rows={filtered}
				xField={filter.chart === 'speed' ? 'encapsTime' : 'pk'}
				yField={filter.chart === 'speed' ? 'decapsTime' : 'ct'}
			/>
		</div>

		{#if !timed}
			<p role="note" class="text-xs text-pqs-steel dark:text-pqs-bluegray">
				Sizes only for now: none of these candidates is in our benchmark harness yet, so the table
				shows them as not yet benchmarked. Timings will follow once they are added.
			</p>
		{/if}

		<KemTable rows={filtered} />

		{#if unmapped.length > 0}
			<p class="text-xs text-pqs-steel dark:text-pqs-bluegray">
				No recommended {target}-bit parameter set in our data: {unmapped.join(', ')}.
			</p>
		{/if}

		<p class="text-right">
			<a href={compareHref(base, p.id, 'kem', { target })} class="font-heading text-sm font-semibold text-pqs-steel hover:text-pqs-apricot dark:text-pqs-apricot">
				Open in the KEM comparison →
			</a>
		</p>
	</div>
{/if}
```

- [ ] **Step 5: Add the candidates section to `ProgrammeView.svelte`**

Re-read `src/lib/components/programme/ProgrammeView.svelte`, then make three edits.

(a) Replace:
```ts
	import FindingsSummary from './FindingsSummary.svelte';
```
with:
```ts
	import SegmentedToggle from '../SegmentedToggle.svelte';
	import FindingsSummary from './FindingsSummary.svelte';
	import KemCandidates from './KemCandidates.svelte';
	import SignatureCandidates from './SignatureCandidates.svelte';
```

(b) Replace:
```ts
	const LINK = 'text-pqs-steel underline hover:text-pqs-apricot dark:text-pqs-apricot';
```
with:
```ts
	type Kind = 'signature' | 'kem';
	const kindOptions: { value: Kind; label: string }[] = [
		...(hasSigs ? [{ value: 'signature' as Kind, label: `Signatures (${sigCount})` }] : []),
		...(hasKems ? [{ value: 'kem' as Kind, label: `KEMs (${kemCount})` }] : [])
	];
	let kind = $state<Kind>(hasSigs ? 'signature' : 'kem');

	const LINK = 'text-pqs-steel underline hover:text-pqs-apricot dark:text-pqs-apricot';
```

(c) Replace the last section and the closing tag:
```svelte
	{#if p.findings}
		<section aria-labelledby="findings-h">
			<h2 id="findings-h" class={H2}>Published findings</h2>
			<FindingsSummary source={p.findings} groups={findingGroups} />
		</section>
	{/if}
</div>
```
with:
```svelte
	{#if p.findings}
		<section aria-labelledby="findings-h">
			<h2 id="findings-h" class={H2}>Published findings</h2>
			<FindingsSummary source={p.findings} groups={findingGroups} />
		</section>
	{/if}

	<section aria-labelledby="candidates-h" class="space-y-4">
		<h2 id="candidates-h" class={H2}>{p.candidates_heading ?? 'Schemes'}</h2>
		{#if kindOptions.length > 1}
			<SegmentedToggle label="Primitive" options={kindOptions} value={kind} onchange={(k) => (kind = k)} />
		{:else}
			<p class="font-heading text-sm font-semibold">{kindOptions[0].label}</p>
		{/if}
		{#if kind === 'signature'}
			<SignatureCandidates programme={p} />
		{:else}
			<KemCandidates programme={p} />
		{/if}
	</section>
</div>
```

- [ ] **Step 6: Run checks and tests**

Run: `npm run check && npm run test && npm run test:e2e`
Expected: 0 svelte-check errors. All unit tests PASS. All e2e specs PASS, including `signatures.spec.ts` and `kems.spec.ts`, which shows the shared filter store still behaves on the compare pages. Then open `/programmes/china-ngcc/` in `npm run preview` and check the browser console. If NGCC data is absent, it must show no errors and the "No … in our data yet." sentences.

- [ ] **Step 7: Commit**

```bash
git add src/lib/components/programme/SignatureCandidates.svelte src/lib/components/programme/KemCandidates.svelte src/lib/components/programme/ProgrammeView.svelte e2e/programmes.spec.ts
git commit -m "Show programme candidates with stage and security-target toggles

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 5: Programme index, nav item and home-table links

**Files:**
- Create: `src/routes/programmes/+page.ts`, `src/routes/programmes/+page.svelte`
- Modify: `src/routes/+layout.svelte` (the `NAV` array)
- Modify: `src/routes/+page.svelte` (the programme name cell)
- Test: `e2e/programmes.spec.ts` (append)

**Interfaces:**
- Consumes: `programmeCards` / `ProgrammeCard` (Task 2), `allProgrammes`, `allSchemeData`, `allKemData`.
- Produces: `/programmes/` with one link per programme to `{base}/programmes/<id>/`; a nav item "Programmes" (`aria-current="page"` on `/programmes/**`); home-table programme names linking to their pages.

- [ ] **Step 1: Write the failing e2e tests**

Inside `test.describe('Programme pages', …)` in `e2e/programmes.spec.ts`, after the last test, add:
```ts
	test('index links to every programme page', async ({ page }) => {
		await page.goto('/programmes/');
		await expect(page.locator('h1')).toHaveText('Programmes');
		for (const id of IDS) {
			await expect(page.locator(`main a[href$="/programmes/${id}/"]`)).toHaveCount(1);
		}
	});

	test('nav links to the programme index and marks it current on programme pages', async ({ page }) => {
		await page.goto('/programmes/kpqc/');
		const link = page.getByRole('banner').getByRole('link', { name: 'Programmes', exact: true });
		await expect(link).toHaveAttribute('href', /\/programmes\/$/);
		await expect(link).toHaveAttribute('aria-current', 'page');
	});

	test('home programme table links to programme pages', async ({ page }) => {
		await page.goto('/');
		const region = page.getByRole('region', { name: 'Programmes' });
		for (const id of IDS) {
			await expect(region.locator(`a[href$="/programmes/${id}/"]`)).toHaveCount(1);
		}
	});
```

- [ ] **Step 2: Run them to verify they fail**

Run: `npm run test:e2e -- e2e/programmes.spec.ts`
Expected: the three new tests FAIL (the index is a 404, there is no nav link, and the home table has no links).

- [ ] **Step 3: Write the index route**

Create `src/routes/programmes/+page.ts`:
```ts
import { allKemData } from '$lib/kemSchemeData';
import { allProgrammes } from '$lib/programmeData';
import { programmeCards } from '$lib/programmePage';
import { allSchemeData } from '$lib/schemeData';

export function load() {
	return { cards: programmeCards(allProgrammes, allSchemeData, allKemData) };
}
```

Create `src/routes/programmes/+page.svelte`:
```svelte
<script lang="ts">
	import { base } from '$app/paths';
	import { SITE_NAME } from '$lib/constants';
	import type { PageData } from './$types';

	let { data }: { data: PageData } = $props();

	const count = (n: number | null, word: string) =>
		n === null ? `${word}: not in scope` : `${n} ${word.toLowerCase()} in our data`;
</script>

<svelte:head>
	<title>Programmes · {SITE_NAME}</title>
</svelte:head>

<div class="mx-auto max-w-screen-2xl px-6 py-8">
	<div class="mb-8 border-l-4 border-pqs-apricot pl-4">
		<h1 class="font-heading text-3xl font-bold text-pqs-midnight dark:text-white">Programmes</h1>
		<p class="mt-2 max-w-[72ch] text-sm text-pqs-steel dark:text-pqs-bluegray">
			Standardisation programmes and competitions whose candidates we track. Each page has the
			programme's timeline, its security-level scale and its candidates.
		</p>
	</div>
	<ul class="grid gap-4 sm:grid-cols-2 lg:grid-cols-3">
		{#each data.cards as c (c.programme.id)}
			<li class="rounded border border-pqs-ashgray bg-white p-5 dark:border-pqs-steel dark:bg-pqs-midnight-mid">
				<h2 class="font-heading text-xl font-bold">
					<a href="{base}/programmes/{c.programme.id}/" class="text-pqs-steel hover:text-pqs-apricot dark:text-pqs-apricot">
						{c.programme.nav_label ?? c.programme.short}
					</a>
				</h2>
				<p class="mt-1 text-sm">{c.programme.name}</p>
				<p class="mt-1 text-xs text-pqs-steel dark:text-pqs-bluegray">
					{c.programme.organiser} · {c.programme.region}{#if c.stage} · {c.stage}{/if}
				</p>
				<p class="mt-3 text-sm">{count(c.signatures, 'Signatures')} · {count(c.kems, 'KEMs')}</p>
			</li>
		{/each}
	</ul>
</div>
```

- [ ] **Step 4: Add the nav item**

Re-read `src/routes/+layout.svelte`, then replace:
```ts
	const NAV = [
		{ href: 'signatures/', label: 'Signatures' },
		{ href: 'kems/', label: 'KEMs' },
		{ href: 'timeline/', label: 'Timeline' }
	];
```
with:
```ts
	const NAV = [
		{ href: 'signatures/', label: 'Signatures' },
		{ href: 'kems/', label: 'KEMs' },
		{ href: 'programmes/', label: 'Programmes' },
		{ href: 'timeline/', label: 'Timeline' }
	];
```

- [ ] **Step 5: Link the home-table rows**

Re-read `src/routes/+page.svelte`, then replace:
```svelte
								<span class="font-heading font-semibold">{s.programme.name}</span>
```
with:
```svelte
								<a href="{base}/programmes/{s.programme.id}/" class="font-heading font-semibold text-pqs-steel underline hover:text-pqs-apricot dark:text-pqs-apricot">{s.programme.name}</a>
```

- [ ] **Step 6: Run checks and tests**

Run: `npm run check && npm run test && npm run test:e2e`
Expected: 0 svelte-check errors, and all unit and e2e tests PASS. That includes `e2e/home.spec.ts`: its "lists every programme" and "shows KpqC signature picks" still match, because the link keeps the same text.

- [ ] **Step 7: Check the base-path build**

Run: `BASE_PATH=/nist-sigs-zoo npm run build && grep -o 'href="/nist-sigs-zoo/programmes/[a-z-]*/"' dist/programmes/index.html | sort -u && ls dist/programmes`
Expected: five distinct `href="/nist-sigs-zoo/programmes/<id>/"` lines, and `dist/programmes/` contains `index.html` plus the directories `china-ngcc iso kpqc nist-onramp nist-pqc`. Rebuild without `BASE_PATH` afterwards (`npm run build`) so a reused preview server serves the normal build.

- [ ] **Step 8: Commit**

```bash
git add src/routes/programmes/+page.ts src/routes/programmes/+page.svelte src/routes/+layout.svelte src/routes/+page.svelte e2e/programmes.spec.ts
git commit -m "Add the programme index, a Programmes nav item and home-table links

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 6: Document programme pages in CLAUDE.md

**Files:**
- Modify: `CLAUDE.md`

**Interfaces:**
- Consumes: everything above.
- Produces: documentation only.

- [ ] **Step 1: Re-read CLAUDE.md**

Run: `git log -3 --format='%h %s' -- CLAUDE.md` and read the file. Plan 3 may have added an "NGCC" subsection under `## Data`. Leave it untouched and edit only the hunks below.

- [ ] **Step 2: Unit and E2E test lists**

Replace:
```
- `legacy.test.ts` — legacy URL forwarding; `programmes.test.ts` — programme definitions
```
with:
```
- `legacy.test.ts` — legacy URL forwarding; `programmes.test.ts` — programme definitions
- `programmePage.test.ts` — programme-page helpers (members, stage views, NGCC targets, findings summary, compare links, facts)
- `programmeRegistry.test.ts` — page fields in `data/programmes.yaml` (dates, links, level tables, NGCC values)
```
Then replace:
```
- `home.spec.ts` — home page: programme table, KpqC picks, compare links, latest changes, no redirect on plain visit
```
with:
```
- `home.spec.ts` — home page: programme table, KpqC picks, compare links, latest changes, no redirect on plain visit
- `programmes.spec.ts` — `/programmes/` index and every `/programmes/<id>/` page: breadcrumb, facts, timeline, level table, findings box, stage / primitive toggles, NGCC empty state (reads `data/schemes`, so it passes with or without NGCC data), nav and home links
```

- [ ] **Step 3: Programmes section**

Replace the paragraph under `### Programmes` that starts with ``data/programmes.yaml` is the registry`` and ends with `the one-off tags → programmes migration.` with:
```
`data/programmes.yaml` is the registry of standardisation programmes (id, name, short, organiser,
region, url, primitives, stages). Scheme versions and KEMs reference programmes by id and stage id
in their `programmes:` maps. `src/lib/__tests__/dataFiles.test.ts` checks that every reference
resolves to a known programme and stage, that no legacy `tags` remain, and the on-ramp round counts.
`scripts/migrate_programmes.py` was the one-off tags → programmes migration.

The registry also drives the programme pages (all optional; see `ProgrammeYaml` in `types.ts`):
`nav_label`, `organiser_detail`, `description`, `links`, `out_of_scope`, `candidates_heading`,
`official_counts` (official candidate numbers, shown next to "N in our data"), `timeline`
(`date` YYYY / YYYY-MM / YYYY-MM-DD, `label`, `note`, at most one `current: true`),
`security_levels` (`intro`, `note`, `columns`, `rows`; first cell is the row header) and `findings`
(`label`, `url`, `credit`, `note`). `programmeRegistry.test.ts` validates them.

`/programmes/` lists the programmes. `/programmes/<id>/` (prerendered via `entries()`) shows breadcrumb,
title block, facts, timeline, level table, findings summary and candidates. Logic is in
`src/lib/programmePage.ts` (pure). Candidates reuse the compare components: signatures go through the
`filterStore` singleton (the section calls `createFilterStore()` with the programme's members only;
every compare page rebuilds it on load), KEMs through `KemTable` / `KemScatterPlot` with a local filter.
Stage toggle: "All, latest specs" plus each `round-N` stage with recorded versions (pinned versions,
as the on-ramp round selector). NGCC targets are focused with `ngccLevels` only. The findings summary
counts each member once under its worst flag (`broken` not classical > `implementation` > `warning`)
plus members with `findings_url`; nothing is hard-coded. With no members, sections show
"No … from this programme are in our data yet."
```

- [ ] **Step 4: Architecture tree**

Replace:
```
│   ├── programmeData.ts  # programme YAML loader
```
with:
```
│   ├── programmeData.ts  # programme YAML loader
│   ├── programmePage.ts  # programme pages: members, stage views, NGCC targets, findings summary, facts, compare links
```
Replace:
```
│       ├── ChartModeToggle.svelte # Sizes / Speed
```
with:
```
│       ├── ChartModeToggle.svelte # Sizes / Speed
│       ├── SegmentedToggle.svelte # generic fieldset of aria-pressed buttons (programme pages)
│       ├── programme/            # ProgrammeView, ProgrammeTimeline, SecurityLevelsTable, FindingsSummary, SignatureCandidates, KemCandidates
```
Replace:
```
    ├── timeline/         # history panel (was /history/)
```
with:
```
    ├── programmes/       # index (+page.ts: programmeCards) and [id]/ (entries() per registry id; {#key} ProgrammeView)
    ├── timeline/         # history panel (was /history/)
```
Replace:
```
    ├── +layout.svelte    # nav (logo, Signatures, KEMs, Timeline, Contribute, theme toggle; no round selector), footer
```
with:
```
    ├── +layout.svelte    # nav (logo, Signatures, KEMs, Programmes, Timeline, Contribute, theme toggle; no round selector), footer
```
Replace:
```
    ├── +page.svelte      # home: hero, programme table, latest changes; forwards legacy /?filters links (legacyHomeRedirect)
```
with:
```
    ├── +page.svelte      # home: hero, programme table (names link to /programmes/<id>/), latest changes; forwards legacy /?filters links (legacyHomeRedirect)
```
In the `src/lib/__tests__/` comment line of the tree, add `programmePage, programmeRegistry,` after `programmes,`. In the `e2e/` list, add a line `├── programmes.spec.ts`.

If any of these snippets no longer matches exactly (because another agent edited CLAUDE.md), apply the same change to the current wording and do not overwrite the other agent's text.

- [ ] **Step 5: Verify and commit**

Run: `npm run check && npm run test`
Expected: PASS (a docs-only change, but this confirms the tree is untouched).

```bash
git add CLAUDE.md
git commit -m "Document programme pages and registry page fields

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

## Values for Thom to confirm (before Task 1's commit)

| Programme | Field | Value in plan | Why unsure |
|---|---|---|---|
| china-ngcc | `organiser_detail` | ICCS, Beijing | Location taken from the canvas, not from the brief |
| china-ngcc | `findings.credit` / `findings.note` | Saarinen credit wording; "many design findings cap only 256/512-bit" | Plan 3 gates attribution on Thom's agreement with Saarinen; the note is a claim about ngcc.dev's results |
| china-ngcc | links | No "Round 1 candidate list" link (canvas had one) | URL not known |
| nist-pqc | timeline | HQC selected 2025-03-11; current entry "FN-DSA and HQC standards: in progress" | Date from memory; FIPS 206 status at 2026-10 unknown |
| nist-pqc | links | NIST IR 8545 as the fourth-round report | Report number from memory |
| nist-pqc, nist-onramp | `organiser_detail` | NIST, Gaithersburg | Not in the brief; delete if unwanted |
| nist-onramp | timeline / counts | Round 3 on 2026-05-14 with 9 candidates (from the HAWK YAML comment and CLAUDE.md); `official_counts.signature: 9` | Round 3 date taken from a data comment |
| nist-onramp | links | NIST IR 8528 as the first-round report | Report number from memory |
| kpqc | whole timeline | 2021-11 launch; 2022-12 round 1 (16); 2023-12 round 2 (8); 2025-01 final (2 + 2) | Months and counts from memory |
| kpqc | security levels | No table (assumes candidates used NIST categories) | Unconfirmed KpqC scale |
| iso | description, `organiser_detail`, link, timeline | "adds post-quantum KEMs" (FrodoKEM, Classic McEliece in our data); JTC 1/SC 27; iso.org/standard/37971; single undated "in preparation" | Amendment scope, status and URL not verified |

## Overlaps with plan 3

- **None in code or data files.** Plan 4 adds `src/lib/programmePage.ts`, `src/lib/components/{SegmentedToggle,programme/*}.svelte`, `src/routes/programmes/**`, `src/lib/__tests__/{programmePage,programmeRegistry}.test.ts` and `e2e/programmes.spec.ts`. It edits `src/lib/types.ts` (the `ProgrammeYaml` block), `data/programmes.yaml`, `src/routes/+layout.svelte` and `src/routes/+page.svelte`. Plan 3 touches none of these.
- **`CLAUDE.md`:** both plans edit it, in different sections (plan 3: an NGCC subsection under `## Data`, plus bench docs). Task 6 re-reads it first and edits only its own hunks.
- **Shared data, read-only for plan 4:** plan 4 reads `data/schemes/*`, `data/kems/*` and `findings_url`, `implementation` and `broken`, which plan 3 writes. Plan 4's pages and e2e tests are written to work before and after plan 3's import and after its gated findings task (Task 14).
- **`e2e/home.spec.ts`:** plan 3 appends to it. Plan 4 deliberately puts its home-link test in `e2e/programmes.spec.ts` instead.
