# Site Reorganization: Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Turn the NIST-only signature site into a multi-programme site. Schemes record which standardisation programmes they belong to, and each parameter set records its security level on its own programme's scale. Signatures move to `/signatures/`, a new home page lists the programmes, and every old URL keeps working.

**Architecture:** A new registry, `data/programmes.yaml`, defines the programmes (NIST PQC, NIST on-ramp, China NGCC, KpqC, ISO) and their stages. Scheme versions replace the old `tags: [round-N]` with `programmes: {nist-onramp: round-N, ...}`, and KEM files gain a scheme-level `programmes:` map. Pure functions in `src/lib/programmes.ts` resolve memberships, and the existing round selector becomes a filter on (programme, stage). Parameter sets gain an optional `ngcc_level` (classical bits) alongside the NIST `level`, a `recommended` flag for the default view, and an `implementation` flag that separates code bugs from design breaks. This prepares the data model for China NGCC, whose 128/256/512-bit targets don't map onto NIST categories. Routes are reshuffled: old routes become small redirect pages that keep the query string and hash.

**Tech Stack:** SvelteKit 2 + Svelte 5 runes, adapter-static, Tailwind v4, TypeScript, Vitest, Playwright, `uv` + `ruamel.yaml` for the one-off data migration.

**Spec:** The design canvas "PQC Zoo reorganization" (https://claude.ai/artifact/GMwciQE71HXauaGfS8MEiC). The relevant boards are "Sitemap and data model", "Home" and "Programme: China NGCC" (security-level table). Read the sitemap board before starting.

## Roadmap (this plan is part 1 of 6)

The design covers several independent subsystems, so it is split into separate plans. Each plan ships working software on its own:

1. **Foundation (this plan):** programme data model, security-level scales, recommended sets and implementation findings in the data model, route moves and redirects, layout nav, new home page.
2. **Compare pages:** bring `/signatures/` and `/kems/` up to the canvas. This covers:
   - a programme filter and a Programmes column;
   - Category, pk+sig and Assumption columns kept;
   - security badges that open an expanding detail row;
   - a Sizes/Speed chart toggle;
   - the KEM page matching the signature layout (scheme tree, performance ranges, benchmark-env panel);
   - filters collapsed on mobile;
   - the 1536 px layout;
   - a second security-level filter group and colour scale for NGCC targets (128/256/384/512-bit), never mapped onto NIST levels;
   - a "Recommended / All parameter sets" toggle;
   - an implementation-findings badge and "Hide …" filters separate from "broken by design";
   - "not yet benchmarked" cells, with unbenchmarked rows left out of the Speed view.
3. **China NGCC data import:** our own extraction from the round-1 submission archives, not ngcc.dev's harness output. It covers:
   - 34 signature and 41 KEM YAML files with sizes, claimed `ngcc_level` per parameter set, and the recommended set per level from each spec;
   - a `findings_url` per candidate linking to ngcc.dev, plus `broken` and `implementation` flags summarised in our own words with credit, after agreeing attribution with Markku-Juhani Saarinen;
   - adding the candidates to `bench/` and `bench-kem/` so timings come from our harness.
4. **Programme pages:** `/programmes/` and `/programmes/[id]/`, including the NGCC page (timeline, security-level table, findings summary, candidates per target).
5. **Scheme pages:** `/schemes/[slug]/` with the version comparison and the scheme's slice of history.
6. **Timeline and rename:** a programme filter on `/timeline/`, the site name and domain, and a redirect stub on the old GitHub Pages URL.

Write plans 2–6 only after this one is merged: they depend on the types and routes defined here. Plans 2 and 3 can run in parallel.

## Global Constraints

- Programme ids: `nist-pqc`, `nist-onramp`, `china-ngcc`, `kpqc`, `iso`. These exact strings are used in YAML, code and URLs.
- Old URLs must keep working, including query strings and hashes: `/?r=…&s=…`, `/advanced/?x=…`, `/history/`. Old `/` links that carry signature filter params must land on `/signatures/` with the params intact.
- Every internal link and redirect target is prefixed with `base` from `$app/paths`. Production builds with `BASE_PATH=/nist-sigs-zoo`.
- `data/schemes/*.yaml` must no longer contain `tags:` after the migration, and every programme id and stage in data must exist in `data/programmes.yaml`.
- The site name is one constant, `SITE_NAME` in `src/lib/constants.ts`, so plan 5 can rename the site in one line. Its value stays `'NIST Signatures Zoo'` for now.
- Python: run with `uv` (PEP 723 inline dependencies, like `bench-kem/update_scheme_data.py`). Format and lint with `ruff format` and `ruff check --fix`.
- Work on branch `feat/programmes-foundation`, never on `main`. Stage specific files and never use `git add -A`.
- Update `data/history.yaml` and `CLAUDE.md` in the tasks that change the data schema or the routes.
- A parameter set states its security level on its own programme's scale: `level` (NIST category 1–5 or `Pre-Quantum`) **or** `ngcc_level` (128, 256, 384 or 512 classical bits), never both, and code never converts one into the other.
- `broken` means a design break. Bugs in submitted code go in `implementation`, which never sets `broken`.
- Never import third-party benchmark numbers (for example ngcc.dev's cycle counts). Timings come only from our `bench/` and `bench-kem/` harnesses, or from submission documents for historic NIST rounds as today.

## Review Focus

1. **A shared link like `/?r=2&s=MAYO,UOV&sort=sig` from a paper or tweet:** it should open `/signatures/?r=2&s=MAYO,UOV&sort=sig` with the round-2 view and those filters. Pinned by Task 5's unit tests and Task 6's e2e test.
2. **A typo in data, such as `programmes: {nist-onramp: round3}` or an unknown programme id:** it must fail `npm run test` with a message naming the file, and must not silently drop the scheme from every view. Pinned by Task 1's and Task 2's `dataFiles.test.ts`.
3. **A scheme that only belongs to KpqC or China NGCC (for example a future NGCC candidate):** it must not appear in the NIST on-ramp round 1/2/3 views, while ML-DSA, SLH-DSA, Falcon and the pre-quantum ECDSA, EdDSA and RSA still do. Pinned by Task 2's unit tests.
4. **Deploying under `BASE_PATH=/nist-sigs-zoo`:** redirects must point to `/nist-sigs-zoo/signatures/…`, not `/signatures/…`. Pinned by Task 5's unit tests, which pass a base.
5. **A parameter set with only an NGCC level (`ngcc_level: 128`, no `level`):** it must not match any NIST level filter, must not be labelled "Level 1" in tables or charts, and must not crash sorting or the scatter plot. Pinned by Task 3's unit tests. (Unknown legacy `?r=7` values are still covered by Task 5's tests.)

---

## File Structure

| File | Responsibility |
|---|---|
| `data/programmes.yaml` (create) | Registry of programmes and their stages |
| `src/lib/types.ts` (modify) | `ProgrammeYaml`, `ProgrammeStageYaml`, `ProgrammeMembership`, `StageFilter`; `programmes` on version, scheme, KEM and parameter-set types; `tags` removed |
| `src/lib/programmes.ts` (create) | Pure helpers: `versionProgrammes`, `schemeProgrammes`, `isReferenceScheme`, `membershipLabel`, `programmeSummaries` |
| `src/lib/programmeData.ts` (create) | `import.meta.glob` loader for `programmes.yaml` |
| `src/lib/data.ts` (modify) | `processYamlSchemes` filters by `StageFilter` instead of a tag string; carries `ngccLevel`, `recommended`, `implementation`, `findingsUrl` |
| `src/lib/kemData.ts` (modify) | `processKemSchemes` carries `programmes`, `ngccLevel`, `recommended`, `implementation`, `findingsUrl`; level filter skips NGCC-scale rows |
| `src/lib/filterStore.ts`, `src/lib/components/{ScatterPlot,KemScatterPlot,SchemeTable,KemTable}.svelte` (modify) | Handle `level: null` (NGCC-scale rows) without mislabelling or crashing |
| `src/lib/legacy.ts` (create) | `legacyHomeRedirect`, `legacyPathRedirect`: pure URL rewriting for old links |
| `src/lib/constants.ts` (modify) | `SITE_NAME` |
| `src/lib/roundStore.ts` (modify) | `roundToFilter(round)` mapping a round to a `StageFilter` |
| `src/lib/components/LegacyRedirect.svelte` (create) | Redirect page body: meta refresh, canonical link, JS redirect that keeps the query and hash |
| `src/lib/components/RoundSelect.svelte` (create) | The round `<select>` that used to be the nav button group |
| `scripts/migrate_programmes.py` (create) | One-off: tags → programmes for signature and KEM YAML |
| `src/routes/+layout.ts` (create) | `prerender` and `trailingSlash = 'always'` for the whole site |
| `src/routes/+layout.svelte` (modify) | Nav: Signatures, KEMs, Timeline, Contribute; round buttons removed; `SITE_NAME` |
| `src/routes/+page.ts`, `+page.svelte` (rewrite) | New home page |
| `src/routes/signatures/+page.ts`, `+page.svelte` (moved from `src/routes/`) | Signatures compare page |
| `src/routes/signatures/advanced/*` (moved from `src/routes/advanced/`) | Advanced graph |
| `src/routes/timeline/*` (moved from `src/routes/history/`) | Timeline |
| `src/routes/advanced/+page.svelte`, `src/routes/history/+page.svelte` (create) | Redirect stubs |
| `src/lib/__tests__/programmes.test.ts`, `legacy.test.ts`, `dataFiles.test.ts` (create) | Unit tests |
| `e2e/signatures.spec.ts` (renamed from `main.spec.ts`), `e2e/home.spec.ts`, `e2e/redirects.spec.ts` (create) | E2E tests |

---

### Task 1: Programme registry, types and pure helpers

**Files:**
- Create: `data/programmes.yaml`
- Create: `src/lib/programmes.ts`
- Create: `src/lib/programmeData.ts`
- Modify: `src/lib/types.ts` (add types; `VersionYaml` gains `programmes`; `KemSchemeYaml` gains `programmes`)
- Test: `src/lib/__tests__/programmes.test.ts`, `src/lib/__tests__/dataFiles.test.ts`

**Interfaces:**
- Produces (types, in `src/lib/types.ts`):
  ```ts
  export interface ProgrammeStageYaml { id: string; label: string }
  export interface ProgrammeYaml {
  	id: string; name: string; short: string; organiser: string; region: string;
  	url?: string; primitives: ('signature' | 'kem')[]; stages: ProgrammeStageYaml[];
  }
  export interface ProgrammeMembership { programme: string; stage: string }
  export interface StageFilter { programme: string; stage: string }
  ```
- Produces (functions, in `src/lib/programmes.ts`):
  - `versionProgrammes(v: { programmes?: Record<string, string> }): Record<string, string>`
  - `schemeProgrammes(versions: VersionYaml[]): ProgrammeMembership[]`: one entry per programme, taking the stage from the newest version in that programme, in the order first seen when walking from newest to oldest.
  - `isReferenceScheme(versions: VersionYaml[]): boolean`: true when no version belongs to any programme other than `nist-pqc`.
  - `membershipLabel(m: ProgrammeMembership, programmes: ProgrammeYaml[]): string`: returns `"<short> <stage label>"` and throws on an unknown programme or stage.
  - `programmeSummaries(programmes, sigs: SchemeYaml[], kems: KemSchemeYaml[]): ProgrammeSummary[]`, where `ProgrammeSummary = { programme: ProgrammeYaml; signatures: string[]; kems: string[] }`. Scheme names are sorted.
- Produces: `allProgrammes: ProgrammeYaml[]` from `src/lib/programmeData.ts`.

- [ ] **Step 1: Create branch**

```bash
git switch -c feat/programmes-foundation
```

- [ ] **Step 2: Write the registry `data/programmes.yaml`**

```yaml
# Standardisation programmes tracked on the site.
# Referenced from data/schemes/*.yaml (versions[].programmes) and data/kems/*.yaml (programmes).
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
- id: china-ngcc
  name: Next-generation commercial cryptographic algorithms
  short: NGCC
  organiser: ICCS
  region: China
  url: https://www.niccs.org.cn/en/
  primitives: [signature, kem]
  stages:
    - { id: round-1, label: round 1 }
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
- id: iso
  name: ISO/IEC 18033-2 amendment
  short: ISO
  organiser: ISO/IEC
  region: International
  primitives: [kem]
  stages:
    - { id: in-progress, label: in progress }
```

`url` is optional; every programme has one today. Round 1 was published on 20 Sep 2026; plan 3 imports its candidates.

- [ ] **Step 3: Add the types to `src/lib/types.ts`**

Append at the end of the file:

```ts
// Standardisation programmes (data/programmes.yaml)

export interface ProgrammeStageYaml {
	id: string;
	label: string;
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
}

/** One programme a scheme took part in, and how far it got. */
export interface ProgrammeMembership {
	programme: string;
	stage: string;
}

/** Selects the versions that were at `stage` in `programme`. */
export interface StageFilter {
	programme: string;
	stage: string;
}
```

In `VersionYaml`, add after `tags?: string[];`:

```ts
	/** Programme id → stage id, e.g. { 'nist-onramp': 'round-3' } */
	programmes?: Record<string, string>;
```

In `KemSchemeYaml`, add after `status: string;`:

```ts
	/** Programme id → stage id, e.g. { 'nist-pqc': 'standardised' } */
	programmes?: Record<string, string>;
```

(`tags` is removed in Task 2, together with the data migration.)

- [ ] **Step 4: Write the failing unit tests `src/lib/__tests__/programmes.test.ts`**

```ts
import { describe, it, expect } from 'vitest';
import {
	versionProgrammes,
	schemeProgrammes,
	isReferenceScheme,
	membershipLabel,
	programmeSummaries
} from '$lib/programmes';
import type { KemSchemeYaml, ProgrammeYaml, SchemeYaml, VersionYaml } from '$lib/types';

const PROGRAMMES: ProgrammeYaml[] = [
	{ id: 'nist-pqc', name: 'NIST PQC', short: 'NIST', organiser: 'NIST', region: 'US', primitives: ['signature', 'kem'],
	  stages: [{ id: 'standardised', label: 'standard' }, { id: 'round-3', label: 'round 3' }] },
	{ id: 'nist-onramp', name: 'On-ramp', short: 'On-ramp', organiser: 'NIST', region: 'US', primitives: ['signature'],
	  stages: [{ id: 'round-1', label: 'round 1' }, { id: 'round-3', label: 'round 3' }] },
	{ id: 'kpqc', name: 'KpqC', short: 'KpqC', organiser: 'KpqC', region: 'KR', primitives: ['signature', 'kem'],
	  stages: [{ id: 'selected', label: 'selected' }] }
];

function v(date: string, programmes?: Record<string, string>): VersionYaml {
	return { version: date, date, status: 'On-ramp', programmes, parametersets: [] };
}

describe('versionProgrammes', () => {
	it('returns the programmes map', () => {
		expect(versionProgrammes(v('2024-01-01', { 'nist-onramp': 'round-1' }))).toEqual({ 'nist-onramp': 'round-1' });
	});
	it('returns an empty map when absent', () => {
		expect(versionProgrammes(v('2024-01-01'))).toEqual({});
	});
});

describe('schemeProgrammes', () => {
	it('takes the stage from the newest version in each programme', () => {
		const versions = [
			v('2023-01-01', { 'nist-onramp': 'round-1' }),
			v('2026-05-01', { 'nist-onramp': 'round-3' }),
			v('2026-09-01')
		];
		expect(schemeProgrammes(versions)).toEqual([{ programme: 'nist-onramp', stage: 'round-3' }]);
	});
	it('lists every programme a scheme was in', () => {
		const versions = [v('2023-05-31', { 'nist-onramp': 'round-1', kpqc: 'selected' })];
		expect(schemeProgrammes(versions)).toEqual([
			{ programme: 'nist-onramp', stage: 'round-1' },
			{ programme: 'kpqc', stage: 'selected' }
		]);
	});
	it('is empty for a scheme in no programme', () => {
		expect(schemeProgrammes([v('2020-01-01')])).toEqual([]);
	});
	it('kems without programmes resolve to an empty list', () => {
		const kems: KemSchemeYaml[] = [{ name: 'BAT', website: '', category: 'Lattices', assumption: '', status: '', parametersets: [] }];
		expect(programmeSummaries(PROGRAMMES, [], kems).every((s) => s.kems.length === 0)).toBe(true);
	});
});

describe('isReferenceScheme', () => {
	it('is true for a scheme in no programme (pre-quantum)', () => {
		expect(isReferenceScheme([v('2020-01-01')])).toBe(true);
	});
	it('is true for a NIST standard', () => {
		expect(isReferenceScheme([v('2024-08-13', { 'nist-pqc': 'standardised' })])).toBe(true);
	});
	it('is false for an on-ramp candidate', () => {
		expect(isReferenceScheme([v('2024-01-01', { 'nist-onramp': 'round-1' })])).toBe(false);
	});
	it('is false for a KpqC-only scheme', () => {
		expect(isReferenceScheme([v('2024-01-01', { kpqc: 'selected' })])).toBe(false);
	});
});

describe('membershipLabel', () => {
	it('combines short name and stage label', () => {
		expect(membershipLabel({ programme: 'nist-onramp', stage: 'round-3' }, PROGRAMMES)).toBe('On-ramp round 3');
	});
	it('throws on an unknown programme', () => {
		expect(() => membershipLabel({ programme: 'nope', stage: 'x' }, PROGRAMMES)).toThrow(/unknown programme "nope"/);
	});
	it('throws on an unknown stage', () => {
		expect(() => membershipLabel({ programme: 'kpqc', stage: 'round-9' }, PROGRAMMES)).toThrow(/unknown stage "round-9"/);
	});
});

describe('programmeSummaries', () => {
	it('lists signature and KEM members per programme, sorted', () => {
		const sigs: SchemeYaml[] = [
			{ name: 'MAYO', website: '', category: 'Multivariate', assumption: '', versions: [v('2026-08-31', { 'nist-onramp': 'round-3' })] },
			{ name: 'HAETAE', website: '', category: 'Lattices', assumption: '', versions: [v('2023-05-31', { 'nist-onramp': 'round-1', kpqc: 'selected' })] },
			{ name: 'AIMer', website: '', category: 'Symmetric', assumption: '', versions: [v('2023-05-31', { 'nist-onramp': 'round-1', kpqc: 'selected' })] }
		];
		const kems: KemSchemeYaml[] = [
			{ name: 'SMAUG-T', website: '', category: 'Lattices', assumption: '', status: '', programmes: { kpqc: 'selected' }, parametersets: [] }
		];
		const kpqc = programmeSummaries(PROGRAMMES, sigs, kems).find((s) => s.programme.id === 'kpqc')!;
		expect(kpqc.signatures).toEqual(['AIMer', 'HAETAE']);
		expect(kpqc.kems).toEqual(['SMAUG-T']);
	});
});
```

- [ ] **Step 5: Run the tests to verify they fail**

Run: `npm run test -- programmes`
Expected: FAIL with `Failed to resolve import "$lib/programmes"`.

- [ ] **Step 6: Implement `src/lib/programmes.ts`**

```ts
import type {
	KemSchemeYaml,
	ProgrammeMembership,
	ProgrammeYaml,
	SchemeYaml,
	VersionYaml
} from './types';

/** Programme id of the NIST standards track; its members are shown in every view. */
export const STANDARDS_PROGRAMME = 'nist-pqc';

export function versionProgrammes(v: { programmes?: Record<string, string> }): Record<string, string> {
	return { ...(v.programmes ?? {}) };
}

export function schemeProgrammes(versions: VersionYaml[]): ProgrammeMembership[] {
	const newestFirst = [...versions].sort((a, b) => b.date.localeCompare(a.date));
	const seen = new Map<string, string>();
	for (const version of newestFirst) {
		for (const [programme, stage] of Object.entries(versionProgrammes(version))) {
			if (!seen.has(programme)) seen.set(programme, stage);
		}
	}
	return [...seen].map(([programme, stage]) => ({ programme, stage }));
}

/** Standards and pre-quantum schemes: shown as reference rows in every competition view. */
export function isReferenceScheme(versions: VersionYaml[]): boolean {
	return versions.every((v) =>
		Object.keys(versionProgrammes(v)).every((p) => p === STANDARDS_PROGRAMME)
	);
}

export function membershipLabel(m: ProgrammeMembership, programmes: ProgrammeYaml[]): string {
	const programme = programmes.find((p) => p.id === m.programme);
	if (!programme) throw new Error(`unknown programme "${m.programme}"`);
	const stage = programme.stages.find((s) => s.id === m.stage);
	if (!stage) throw new Error(`unknown stage "${m.stage}" in programme "${m.programme}"`);
	return `${programme.short} ${stage.label}`;
}

export interface ProgrammeSummary {
	programme: ProgrammeYaml;
	signatures: string[];
	kems: string[];
}

export function programmeSummaries(
	programmes: ProgrammeYaml[],
	sigs: SchemeYaml[],
	kems: KemSchemeYaml[]
): ProgrammeSummary[] {
	return programmes.map((programme) => ({
		programme,
		signatures: sigs
			.filter((s) => schemeProgrammes(s.versions).some((m) => m.programme === programme.id))
			.map((s) => s.name)
			.sort((a, b) => a.localeCompare(b)),
		kems: kems
			.filter((k) => programme.id in (k.programmes ?? {}))
			.map((k) => k.name)
			.sort((a, b) => a.localeCompare(b))
	}));
}
```

- [ ] **Step 7: Implement the loader `src/lib/programmeData.ts`**

```ts
import type { ProgrammeYaml } from './types';

const modules = import.meta.glob('../../data/programmes.yaml', { eager: true }) as Record<
	string,
	{ default: ProgrammeYaml[] }
>;

export const allProgrammes: ProgrammeYaml[] = Object.values(modules)[0]?.default ?? [];
```

- [ ] **Step 8: Write the registry integrity test `src/lib/__tests__/dataFiles.test.ts`**

It reads the real data files from disk, so a typo fails `npm run test`. It uses `node:fs` and `js-yaml` (already a dev dependency) and does not use `import.meta.glob`, so it runs in plain Vitest.

```ts
import { describe, it, expect } from 'vitest';
import { readFileSync, readdirSync } from 'node:fs';
import { join } from 'node:path';
import yaml from 'js-yaml';
import type { KemSchemeYaml, ProgrammeYaml, SchemeYaml } from '$lib/types';

const DATA = join(process.cwd(), 'data');
const load = <T>(path: string): T => yaml.load(readFileSync(path, 'utf8')) as T;

const programmes = load<ProgrammeYaml[]>(join(DATA, 'programmes.yaml'));
const stagesOf = new Map(programmes.map((p) => [p.id, new Set(p.stages.map((s) => s.id))]));

function checkMembership(file: string, map: Record<string, string> | undefined): string[] {
	return Object.entries(map ?? {}).flatMap(([programme, stage]) => {
		const stages = stagesOf.get(programme);
		if (!stages) return [`${file}: unknown programme "${programme}"`];
		if (!stages.has(stage)) return [`${file}: unknown stage "${stage}" in programme "${programme}"`];
		return [];
	});
}

describe('data/programmes.yaml', () => {
	it('has unique programme ids', () => {
		const ids = programmes.map((p) => p.id);
		expect(new Set(ids).size).toBe(ids.length);
	});
	it('has unique stage ids within each programme', () => {
		for (const p of programmes) {
			const ids = p.stages.map((s) => s.id);
			expect(new Set(ids).size, p.id).toBe(ids.length);
		}
	});
});

describe('programme references in scheme data', () => {
	it('every signature version references a known programme and stage', () => {
		const dir = join(DATA, 'schemes');
		const errors = readdirSync(dir)
			.filter((f) => f.endsWith('.yaml'))
			.flatMap((f) => load<SchemeYaml>(join(dir, f)).versions.flatMap((v) => checkMembership(f, v.programmes)));
		expect(errors).toEqual([]);
	});
	it('every KEM references a known programme and stage', () => {
		const dir = join(DATA, 'kems');
		const errors = readdirSync(dir)
			.filter((f) => f.endsWith('.yaml'))
			.flatMap((f) => checkMembership(f, load<KemSchemeYaml>(join(dir, f)).programmes));
		expect(errors).toEqual([]);
	});
});
```

- [ ] **Step 9: Run all unit tests**

Run: `npm run test`
Expected: PASS. No data file has `programmes` yet, so the reference checks pass trivially.

- [ ] **Step 10: Type-check**

Run: `npm run check`
Expected: `svelte-check found 0 errors`.

- [ ] **Step 11: Commit**

```bash
git add data/programmes.yaml src/lib/types.ts src/lib/programmes.ts src/lib/programmeData.ts src/lib/__tests__/programmes.test.ts src/lib/__tests__/dataFiles.test.ts
git commit -m "Add programme registry and membership helpers"
```

---

### Task 2: Migrate data from tags to programmes and filter by stage

The migration and the code switch land in one commit. Splitting them would leave a commit where the round views are empty.

**Files:**
- Create: `scripts/migrate_programmes.py`
- Modify: every `data/schemes/*.yaml` and `data/kems/*.yaml` (generated by the script)
- Modify: `src/lib/types.ts` (`tags` removed from `VersionYaml` and `Scheme`; `programmes` added to `Scheme`, `ParameterSet`, `KemScheme` and `KemParameterSet`)
- Modify: `src/lib/data.ts:153-285` (`processYamlSchemes`)
- Modify: `src/lib/kemData.ts:17-60` (`processKemSchemes`)
- Modify: `src/lib/roundStore.ts`
- Modify: `src/routes/+page.ts`, `src/routes/+page.svelte:28-33`, `src/routes/advanced/+page.ts`
- Modify: `src/lib/__tests__/data.test.ts`, `src/lib/__tests__/kemData.test.ts`, `src/lib/__tests__/dataFiles.test.ts`
- Modify: `data/history.yaml`, `CLAUDE.md`

**Interfaces:**
- Consumes: `StageFilter`, `ProgrammeMembership`, `schemeProgrammes`, `isReferenceScheme`, `versionProgrammes` (Task 1).
- Produces:
  - `processYamlSchemes(schemeData: SchemeYaml[], filter?: StageFilter, opts?: { useLatestVersion?: boolean })`. Same return shape as before.
  - `Scheme.programmes: ProgrammeMembership[]`, `ParameterSet.programmes: ProgrammeMembership[]`, `KemScheme.programmes: ProgrammeMembership[]`, `KemParameterSet.programmes: ProgrammeMembership[]`.
  - `roundToFilter(round: Round): { filter: StageFilter; useLatestVersion: boolean }` in `src/lib/roundStore.ts`.

- [ ] **Step 1: Write the migration script `scripts/migrate_programmes.py`**

```python
#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.10"
# dependencies = ["ruamel.yaml>=0.18"]
# ///
"""One-off migration: replace `tags: [round-N]` with `programmes:` maps.

Signatures: versions[].tags [round-N] -> versions[].programmes {nist-onramp: round-N}.
NIST standards and KpqC picks get their memberships from SIG_EXTRA.
KEMs: a scheme-level `programmes:` map from KEM_PROGRAMMES.

Usage:
    uv run scripts/migrate_programmes.py
"""

import sys
from pathlib import Path

from ruamel.yaml import YAML
from ruamel.yaml.comments import CommentedMap

REPO_ROOT = Path(__file__).parent.parent
SCHEMES_DIR = REPO_ROOT / "data" / "schemes"
KEMS_DIR = REPO_ROOT / "data" / "kems"

# Scheme name -> memberships added to *every* version of that scheme.
SIG_EXTRA_ALL_VERSIONS: dict[str, dict[str, str]] = {
    "ML-DSA": {"nist-pqc": "standardised"},
    "SLH-DSA": {"nist-pqc": "standardised"},
    "Falcon": {"nist-pqc": "selected"},
}
# Scheme name -> memberships added to the *newest* version only.
SIG_EXTRA_LATEST: dict[str, dict[str, str]] = {
    "HAETAE": {"kpqc": "selected"},
    "AIMer": {"kpqc": "selected"},
}
KEM_PROGRAMMES: dict[str, dict[str, str]] = {
    "ML-KEM": {"nist-pqc": "standardised"},
    "HQC": {"nist-pqc": "selected"},
    "FrodoKEM": {"nist-pqc": "round-3", "iso": "in-progress"},
    "Classic McEliece": {"nist-pqc": "round-4", "iso": "in-progress"},
    "NTRU": {"nist-pqc": "round-3"},
    "NTRU Prime": {"nist-pqc": "round-3"},
    "SABER": {"nist-pqc": "round-3"},
    "SMAUG-T": {"kpqc": "selected"},
    "NTRU+": {"kpqc": "selected"},
}


def make_yaml() -> YAML:
    yaml = YAML()
    yaml.preserve_quotes = True
    yaml.width = 4096
    yaml.indent(mapping=2, sequence=2, offset=0)
    return yaml


def set_programmes(version: CommentedMap, extra: dict[str, str]) -> None:
    programmes = CommentedMap(version.get("programmes") or {})
    for tag in version.get("tags") or []:
        if not str(tag).startswith("round-"):
            sys.exit(f"unexpected tag {tag!r} in version {version.get('version')!r}")
        programmes["nist-onramp"] = str(tag)
    programmes.update(extra)
    if "tags" in version:
        del version["tags"]
    if programmes:
        # Place programmes right after status, where tags used to sit.
        keys = list(version.keys())
        version.insert(keys.index("status") + 1, "programmes", programmes)


def migrate_signatures(yaml: YAML) -> None:
    for path in sorted(SCHEMES_DIR.glob("*.yaml")):
        doc = yaml.load(path)
        name = doc["name"]
        newest = max(doc["versions"], key=lambda v: str(v["date"]))
        for version in doc["versions"]:
            extra = dict(SIG_EXTRA_ALL_VERSIONS.get(name, {}))
            if version is newest:
                extra.update(SIG_EXTRA_LATEST.get(name, {}))
            set_programmes(version, extra)
        yaml.dump(doc, path)


def migrate_kems(yaml: YAML) -> None:
    for path in sorted(KEMS_DIR.glob("*.yaml")):
        doc = yaml.load(path)
        memberships = KEM_PROGRAMMES.get(doc["name"])
        if memberships:
            keys = list(doc.keys())
            doc.insert(keys.index("status") + 1, "programmes", CommentedMap(memberships))
        yaml.dump(doc, path)


def main() -> None:
    yaml = make_yaml()
    migrate_signatures(yaml)
    migrate_kems(yaml)


if __name__ == "__main__":
    main()
```

- [ ] **Step 2: Lint the script**

Run: `ruff format scripts/migrate_programmes.py && ruff check --fix scripts/migrate_programmes.py`
Expected: no remaining errors.

- [ ] **Step 3: Run the migration and inspect the diff**

Run: `uv run scripts/migrate_programmes.py && git diff --stat data/ && git diff data/schemes/HAETAE.yaml data/schemes/ML-DSA.yaml data/kems/FrodoKEM.yaml`
Expected:
- `HAETAE.yaml` shows `tags:\n  - round-1` replaced by `programmes:\n    nist-onramp: round-1\n    kpqc: selected`.
- `ML-DSA.yaml` gains `programmes:\n    nist-pqc: standardised`.
- `FrodoKEM.yaml` gains a scheme-level `programmes:` block.
- No other changes, such as re-quoted strings or reflowed long lines. If there are any, revert with `git checkout data/`, fix the YAML settings and rerun.

Run: `grep -l "tags:" data/schemes/*.yaml`
Expected: no output.

- [ ] **Step 4: Extend `dataFiles.test.ts` so no `tags` remain**

Add inside `describe('programme references in scheme data', …)`:

```ts
	it('no signature version still uses legacy tags', () => {
		const dir = join(DATA, 'schemes');
		const offenders = readdirSync(dir)
			.filter((f) => f.endsWith('.yaml'))
			.filter((f) => load<{ versions: Record<string, unknown>[] }>(join(dir, f)).versions.some((v) => 'tags' in v));
		expect(offenders).toEqual([]);
	});
	it('the on-ramp has 40, 14 and 9 schemes in rounds 1, 2 and 3', () => {
		const dir = join(DATA, 'schemes');
		const count = (stage: string) =>
			readdirSync(dir)
				.filter((f) => f.endsWith('.yaml'))
				.filter((f) => load<SchemeYaml>(join(dir, f)).versions.some((v) => v.programmes?.['nist-onramp'] === stage)).length;
		expect([count('round-1'), count('round-2'), count('round-3')]).toEqual([40, 14, 9]);
	});
```

(The counts match the number of files carrying each tag before the migration: `grep -l` gives 40, 14 and 9.)

- [ ] **Step 5: Update the unit tests in `src/lib/__tests__/data.test.ts` to the new API**

Replace every `tags: ['round-N']` in fixtures with `programmes: { 'nist-onramp': 'round-N' }`. Replace every call `processYamlSchemes(data, 'round-2'…)` with `processYamlSchemes(data, { programme: 'nist-onramp', stage: 'round-2' }…)`. Then add these tests at the end of the `describe` block:

```ts
	const ONRAMP_R2 = { programme: 'nist-onramp', stage: 'round-2' };

	it('includes NIST standards as reference schemes in on-ramp views', () => {
		const data = [scheme({ name: 'ML-DSA', versions: [{ version: 'FIPS 204', date: '2024-08-13', status: 'FIPS', programmes: { 'nist-pqc': 'standardised' }, parametersets: [BASE_PS] }] })];
		const { schemes } = processYamlSchemes(data, ONRAMP_R2);
		expect(schemes.map((s) => s.scheme)).toEqual(['ML-DSA']);
	});

	it('excludes schemes that are only in another competition (KpqC, NGCC)', () => {
		const data = [
			scheme({ name: 'KpqcOnly', versions: [{ version: 'v1', date: '2024-01-01', status: 'KpqC', programmes: { kpqc: 'selected' }, parametersets: [BASE_PS] }] }),
			scheme({ name: 'NgccOnly', versions: [{ version: 'v1', date: '2025-06-01', status: 'NGCC', programmes: { 'china-ngcc': 'round-1' }, parametersets: [BASE_PS] }] })
		];
		const { schemes } = processYamlSchemes(data, ONRAMP_R2);
		expect(schemes).toEqual([]);
	});

	it('exposes every programme membership on schemes and parameter sets', () => {
		const data = [scheme({ name: 'HAETAE', versions: [{ version: 'Round 1', date: '2023-05-31', status: 'On-ramp', programmes: { 'nist-onramp': 'round-1', kpqc: 'selected' }, parametersets: [BASE_PS] }] })];
		const { schemes, parameterSets } = processYamlSchemes(data);
		const expected = [{ programme: 'nist-onramp', stage: 'round-1' }, { programme: 'kpqc', stage: 'selected' }];
		expect(schemes[0].programmes).toEqual(expected);
		expect(parameterSets[0].programmes).toEqual(expected);
	});
```

- [ ] **Step 6: Add a KEM programmes test to `src/lib/__tests__/kemData.test.ts`**

Add inside the `describe` that tests `processKemSchemes`. It reuses the file's `mlkem` fixture:

```ts
	it('carries scheme-level programmes onto schemes and parameter sets', () => {
		const data = [{ ...mlkem, name: 'FrodoKEM', programmes: { 'nist-pqc': 'round-3', iso: 'in-progress' } }];
		const { schemes, parameterSets } = processKemSchemes(data);
		const expected = [{ programme: 'nist-pqc', stage: 'round-3' }, { programme: 'iso', stage: 'in-progress' }];
		expect(schemes[0].programmes).toEqual(expected);
		expect(parameterSets.every((p) => JSON.stringify(p.programmes) === JSON.stringify(expected))).toBe(true);
	});
```

- [ ] **Step 7: Run the tests to verify they fail**

Run: `npm run test`
Expected: FAIL. TypeScript complains that `programmes` doesn't exist on `Scheme`, and the new data tests fail because the filter still uses tags.

- [ ] **Step 8: Update the types in `src/lib/types.ts`**

- In `Scheme`, replace `tags: string[];` with `programmes: ProgrammeMembership[];`.
- In `ParameterSet`, add `programmes: ProgrammeMembership[];` after `perfSource`.
- In `VersionYaml`, delete `tags?: string[];`.
- In `KemScheme`, add `programmes: ProgrammeMembership[];` after `classical`.
- In `KemParameterSet`, add `programmes: ProgrammeMembership[];` after `notes`.

- [ ] **Step 9: Update `processYamlSchemes` in `src/lib/data.ts`**

Change the import line to:

```ts
import { CPUSPEED } from './constants';
import { isReferenceScheme, schemeProgrammes, versionProgrammes } from './programmes';
import type { DataRanges, NistLevel, ParameterSet, Scheme, SchemeYaml, StageFilter } from './types';
```

Replace the signature and the version-selection block (from `export function processYamlSchemes(` through `const scheme: Scheme = {` … `tags: …,` `};`) with:

```ts
export function processYamlSchemes(
	schemeData: SchemeYaml[],
	filter?: StageFilter,
	{ useLatestVersion = false }: { useLatestVersion?: boolean } = {}
): {
	schemes: Scheme[];
	parameterSets: ParameterSet[];
	ranges: DataRanges;
} {
	const schemes: Scheme[] = [];
	const rows: ParameterSet[] = [];

	for (const yaml of schemeData) {
		if (!yaml.versions || yaml.versions.length === 0) continue;

		const sorted = [...yaml.versions].sort((a, b) => b.date.localeCompare(a.date));

		let latest;
		if (filter) {
			const atStage = sorted.filter(
				(v) => versionProgrammes(v)[filter.programme] === filter.stage
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

		const scheme: Scheme = {
			scheme: yaml.name,
			status: latest.status,
			website: yaml.website,
			category: yaml.category,
			assumption: yaml.assumption,
			broken: latest.broken ?? false,
			warning: latest.warning ?? false,
			info: latest.info ?? false,
			classical: latest.broken === 'classical',
			programmes,
		};
```

In the `rows.push({ … })` object, add `programmes,` after `perfSource: latest.perf_source ?? null,`.

In the legacy CSV `parseSchemes` function, replace `tags: [],` with `programmes: [],`. In `parseParameterSets`, add `programmes: [],` after `perfSource: null,`.

- [ ] **Step 10: Update `processKemSchemes` in `src/lib/kemData.ts`**

After `for (const yaml of schemeData) {` and the empty-parametersets `continue`, add:

```ts
		const programmes = Object.entries(yaml.programmes ?? {}).map(([programme, stage]) => ({
			programme,
			stage
		}));
```

Add `programmes,` to the object pushed to `schemes` (after `classical`) and to the object pushed to `rows` (after `notes`).

- [ ] **Step 11: Map rounds to stage filters in `src/lib/roundStore.ts`**

Replace the file with:

```ts
import { writable } from 'svelte/store';
import type { StageFilter } from './types';

export type Round = 'round-1' | 'round-2' | 'round-3' | 'latest';

export const roundStore = writable<Round>('latest');

/** "Latest" and "Round 3" both select round-3 entrants; "Latest" shows their newest spec. */
export function roundToFilter(round: Round): { filter: StageFilter; useLatestVersion: boolean } {
	const stage = round === 'latest' ? 'round-3' : round;
	return {
		filter: { programme: 'nist-onramp', stage },
		useLatestVersion: round === 'latest' || round === 'round-3'
	};
}
```

- [ ] **Step 12: Update the callers**

`src/routes/+page.ts` and `src/routes/advanced/+page.ts`: replace

```ts
processYamlSchemes(allSchemeData, 'round-3', { useLatestVersion: true })
```

with

```ts
processYamlSchemes(allSchemeData, roundToFilter('latest').filter, { useLatestVersion: true })
```

and add `import { roundToFilter } from '$lib/roundStore';`.

`src/routes/+page.svelte`, in `applyRound`: replace the two lines that compute `tagFilter` and call `processYamlSchemes` with:

```ts
		const { filter, useLatestVersion } = roundToFilter(round);
		const result = processYamlSchemes(allSchemeData, filter, { useLatestVersion });
```

Add `roundToFilter` to the existing `import { roundStore, type Round } from '$lib/roundStore';`. Do the same for the module-level `_init` call:

```ts
	const _init = processYamlSchemes(allSchemeData, roundToFilter('latest').filter, { useLatestVersion: true });
```

- [ ] **Step 13: Run unit tests and type-check**

Run: `npm run test && npm run check`
Expected: all PASS, 0 errors.

- [ ] **Step 14: Run the e2e suite to confirm the round views are unchanged**

Run: `npm run test:e2e`
Expected: all PASS (main, advanced, kems).

- [ ] **Step 15: Update `CLAUDE.md` and `data/history.yaml`**

In `CLAUDE.md`'s YAML schema block, replace the `tags:` line and its comment with:

```yaml
    programmes:                # programme id → stage id (see data/programmes.yaml)
      nist-onramp: round-3     # omit entirely for reference schemes (pre-quantum)
```

Replace the "Post-round-3 updates should have NO tags" guidance with: "Post-round-3 spec updates are new versions **without** a `nist-onramp` entry. Round views show the pinned submission, and Latest shows the newest version." Add a `### Programmes` subsection that describes `data/programmes.yaml` and the `dataFiles.test.ts` integrity check. Add `programmes:` to the KEM schema block.

Prepend to `data/history.yaml`:

```yaml
- date: '2026-10-07'
  type: update
  description: "Scheme data now records which standardisation programmes each scheme belongs to (NIST PQC, NIST on-ramp, KpqC, ISO), in preparation for tracking China's NGCC programme."
```

- [ ] **Step 16: Commit**

```bash
git add scripts/migrate_programmes.py data/schemes data/kems data/history.yaml src/lib/types.ts src/lib/data.ts src/lib/kemData.ts src/lib/roundStore.ts src/routes/+page.ts src/routes/+page.svelte src/routes/advanced/+page.ts src/lib/__tests__/data.test.ts src/lib/__tests__/kemData.test.ts src/lib/__tests__/dataFiles.test.ts CLAUDE.md
git commit -m "Replace round tags with programme memberships"
```

---

### Task 3: Security-level scales, recommended sets and implementation findings (data model)

This task changes the data model only: no NGCC data exists yet, and plan 2 adds the UI controls. It makes the YAML schema, the processed rows and the existing components accept NGCC-scale parameter sets safely. Plan 3 can then import data without touching code, and today's pages keep working.

**Files:**
- Modify: `src/lib/types.ts` (`NgccLevel`; new optional YAML fields; new row fields; `level` becomes nullable)
- Modify: `src/lib/data.ts` (`processYamlSchemes`, legacy `parseParameterSets`)
- Modify: `src/lib/kemData.ts` (`processKemSchemes`, `filterKemRows`)
- Modify: `src/lib/filterStore.ts:181` (level filter)
- Modify: `src/lib/components/SchemeTable.svelte:116`, `src/lib/components/KemTable.svelte:180`, `src/lib/components/ScatterPlot.svelte:75-78`, `src/lib/components/KemScatterPlot.svelte:57-60`
- Test: `src/lib/__tests__/data.test.ts`, `src/lib/__tests__/kemData.test.ts`, `src/lib/__tests__/filterStore.test.ts`, `src/lib/__tests__/dataFiles.test.ts`
- Docs: `CLAUDE.md`

**Interfaces:**
- Consumes: `processYamlSchemes`, `processKemSchemes` (Task 2).
- Produces (types, in `src/lib/types.ts`):
  ```ts
  /** China NGCC claimed classical security, in bits. Not a NIST category. */
  export type NgccLevel = 128 | 256 | 384 | 512;
  ```
  - `ParameterSetYaml` and `KemParameterSetYaml`: `level` becomes optional. They gain `ngcc_level?: NgccLevel`, `recommended?: boolean` and `implementation?: string`.
  - `VersionYaml` and `KemSchemeYaml`: gain `implementation?: string` and `findings_url?: string`.
  - `ParameterSet` and `KemParameterSet`: `level: NistLevel | null`, plus `ngccLevel: NgccLevel | null`, `recommended: boolean`, `implementation: false | string`, `findingsUrl: string | null`.
- Semantics that plan 2 relies on:
  - **`recommended`:** if no parameter set of a version (a KEM scheme, for KEMs) sets `recommended`, all of them are recommended, so today's data shows unchanged. If any parameter set sets `recommended: true`, only those are recommended.
  - **`implementation`:** falls back from the parameter set to the version (the scheme, for KEMs), like `broken`, `warning` and `info` do. It never sets `broken` or `classical`.
  - **Level filters:** rows with `level === null` never match a NIST level filter.

- [ ] **Step 1: Write the failing tests in `src/lib/__tests__/data.test.ts`**

Add inside the `describe('processYamlSchemes', …)` block:

```ts
	it('reads an NGCC level and leaves the NIST level empty', () => {
		const ps = { name: 'BiT-128', ngcc_level: 128 as const, pk: 1048, sig: 1504 };
		const data = [scheme({ versions: [{ version: 'Round 1', date: '2026-09-20', status: 'NGCC', programmes: { 'china-ngcc': 'round-1' }, parametersets: [ps] }] })];
		const [row] = processYamlSchemes(data).parameterSets;
		expect(row.level).toBeNull();
		expect(row.ngccLevel).toBe(128);
	});

	it('marks every parameter set recommended when the version sets no flags', () => {
		const data = [scheme({ versions: [{ version: 'v1', date: '2024-01-01', status: 'On-ramp', parametersets: [BASE_PS, { ...BASE_PS, name: 'Test-256' }] }] })];
		expect(processYamlSchemes(data).parameterSets.map((r) => r.recommended)).toEqual([true, true]);
	});

	it('marks only flagged parameter sets recommended when any are flagged', () => {
		const data = [scheme({ versions: [{ version: 'v1', date: '2024-01-01', status: 'On-ramp', parametersets: [
			{ ...BASE_PS, name: 'fast', recommended: true },
			{ ...BASE_PS, name: 'small' }
		] }] })];
		expect(processYamlSchemes(data).parameterSets.map((r) => [r.parameterset, r.recommended])).toEqual([['fast', true], ['small', false]]);
	});

	it('keeps implementation findings separate from design breaks', () => {
		const data = [scheme({ versions: [{
			version: 'v1', date: '2024-01-01', status: 'NGCC', implementation: 'Stack write on malformed hints',
			findings_url: 'https://ngcc.dev/reports/sign-01.html', parametersets: [BASE_PS]
		}] })];
		const [row] = processYamlSchemes(data).parameterSets;
		expect(row.implementation).toBe('Stack write on malformed hints');
		expect(row.broken).toBe(false);
		expect(row.classical).toBe(false);
		expect(row.findingsUrl).toBe('https://ngcc.dev/reports/sign-01.html');
	});

	it('lets a parameter set override the version implementation flag', () => {
		const data = [scheme({ versions: [{
			version: 'v1', date: '2024-01-01', status: 'NGCC', implementation: 'version-level',
			parametersets: [{ ...BASE_PS, implementation: 'ps-level' }]
		}] })];
		expect(processYamlSchemes(data).parameterSets[0].implementation).toBe('ps-level');
	});
```

- [ ] **Step 2: Write the failing filter tests**

Append to `src/lib/__tests__/filterStore.test.ts`:

```ts
import { get } from 'svelte/store';
import { createFilterStore } from '$lib/filterStore';
import { processYamlSchemes } from '$lib/data';
import type { SchemeYaml } from '$lib/types';

describe('NGCC-scale rows and the NIST level filter', () => {
	it('never match a NIST level, even with every level selected', () => {
		const data: SchemeYaml[] = [
			{ name: 'Nist', website: '', category: 'Lattices', assumption: '', versions: [{ version: 'v1', date: '2024-01-01', status: 'On-ramp', parametersets: [{ name: 'L1', level: 1, pk: 100, sig: 200, signing_cycles: 1000, verification_cycles: 1000 }] }] },
			{ name: 'Ngcc', website: '', category: 'Lattices', assumption: '', versions: [{ version: 'R1', date: '2026-09-20', status: 'NGCC', parametersets: [{ name: 'X-128', ngcc_level: 128, pk: 100, sig: 200, signing_cycles: 1000, verification_cycles: 1000 }] }] }
		];
		const { parameterSets, ranges } = processYamlSchemes(data);
		const { filteredRows } = createFilterStore(parameterSets, ranges, new URLSearchParams());
		expect(get(filteredRows).map((r) => r.scheme)).toEqual(['Nist']);
	});

	it('sort by level keeps NGCC-scale rows last without throwing', () => {
		const data: SchemeYaml[] = [
			{ name: 'A', website: '', category: 'Lattices', assumption: '', versions: [{ version: 'v', date: '2026-01-01', status: 'x', parametersets: [
				{ name: 'n', ngcc_level: 256, pk: 1, sig: 1, signing_cycles: 1, verification_cycles: 1 },
				{ name: 'l', level: 3, pk: 1, sig: 1, signing_cycles: 1, verification_cycles: 1 }
			] }] }
		];
		const { parameterSets } = processYamlSchemes(data);
		const sorted = [...parameterSets].sort((a, b) => sortRows(a, b, 'level', 'asc'));
		expect(sorted.map((r) => r.parameterset)).toEqual(['l', 'n']);
	});
});
```

Add `sortRows` to the existing `import { buildUrlParams } from '$lib/filterStore';` line.

Add to `src/lib/__tests__/kemData.test.ts`, inside the `describe` that tests `filterKemRows`:

```ts
	it('never lets an NGCC-scale KEM match a NIST level filter', () => {
		const ngcc = { ...mlkem, name: 'Aigis-Enc+', parametersets: [{ name: 'Aigis-enc1', ngcc_level: 128 as const, pk: 656, ct: 896 }] };
		const { schemes, parameterSets } = processKemSchemes([mlkem, ngcc]);
		const filter = defaultKemFilter(computeKemRanges(parameterSets), schemes.map((s) => s.scheme));
		expect(filterKemRows(parameterSets, filter).map((r) => r.scheme)).not.toContain('Aigis-Enc+');
		expect(parameterSets.find((r) => r.scheme === 'Aigis-Enc+')!.ngccLevel).toBe(128);
	});
```

- [ ] **Step 3: Add the data-file rule to `src/lib/__tests__/dataFiles.test.ts`**

Add a new `describe` block:

```ts
describe('security levels in data', () => {
	const NGCC = new Set([128, 256, 384, 512]);
	type Ps = { name: string; level?: unknown; ngcc_level?: unknown };
	function check(file: string, sets: Ps[]): string[] {
		return sets.flatMap((ps) => {
			const hasNist = ps.level !== undefined;
			const hasNgcc = ps.ngcc_level !== undefined;
			if (hasNist === hasNgcc) return [`${file} ${ps.name}: needs exactly one of level / ngcc_level`];
			if (hasNgcc && !NGCC.has(ps.ngcc_level as number)) return [`${file} ${ps.name}: ngcc_level must be 128, 256, 384 or 512`];
			return [];
		});
	}
	it('every signature parameter set states exactly one level scale', () => {
		const dir = join(DATA, 'schemes');
		const errors = readdirSync(dir).filter((f) => f.endsWith('.yaml'))
			.flatMap((f) => load<SchemeYaml>(join(dir, f)).versions.flatMap((v) => check(f, v.parametersets as Ps[])));
		expect(errors).toEqual([]);
	});
	it('every KEM parameter set states exactly one level scale', () => {
		const dir = join(DATA, 'kems');
		const errors = readdirSync(dir).filter((f) => f.endsWith('.yaml'))
			.flatMap((f) => check(f, load<KemSchemeYaml>(join(dir, f)).parametersets as Ps[]));
		expect(errors).toEqual([]);
	});
});
```

- [ ] **Step 4: Run the tests to verify they fail**

Run: `npm run test`
Expected: FAIL. TypeScript errors on `ngcc_level`, `recommended`, `implementation` and `findings_url`, and the new assertions fail. The `dataFiles` level rule passes already, because all current data has `level`.

- [ ] **Step 5: Update the types in `src/lib/types.ts`**

Add near the top, after `NistLevel`:

```ts
/** China NGCC claimed classical security, in bits. Not a NIST category; never converted to one. */
export type NgccLevel = 128 | 256 | 384 | 512;
```

- In `ParameterSet`: change `level: NistLevel;` to `level: NistLevel | null;`, and add after `perfSource`:
  ```ts
  	ngccLevel: NgccLevel | null;
  	recommended: boolean;
  	implementation: false | string;
  	findingsUrl: string | null;
  ```
- In `ParameterSetYaml`: change `level: number | 'Pre-Quantum';` to `level?: number | 'Pre-Quantum';` and add:
  ```ts
  	ngcc_level?: NgccLevel;
  	recommended?: boolean;
  	implementation?: string;
  ```
- In `VersionYaml`: add `implementation?: string;` and `findings_url?: string;`.
- In `KemParameterSetYaml`: make `level` optional the same way and add the same three fields.
- In `KemSchemeYaml`: add `implementation?: string;` and `findings_url?: string;`.
- In `KemParameterSet`: change `level` to `NistLevel | null` and add the same four fields as `ParameterSet`.

- [ ] **Step 6: Update `processYamlSchemes` in `src/lib/data.ts`**

Replace the `level` computation inside the parameter-set loop with:

```ts
			const level: NistLevel | null =
				ps.level === undefined
					? null
					: ps.level === 'Pre-Quantum'
						? 'Pre-Quantum'
						: (ps.level as 1 | 2 | 3 | 4 | 5);
```

Before the parameter-set loop (`for (const ps of latest.parametersets)`), add:

```ts
		const anyRecommended = latest.parametersets.some((p) => p.recommended);
```

Inside the loop, after the `info` fallback line, add:

```ts
			const implementation = ps.implementation ?? latest.implementation ?? false;
```

and add to the object pushed to `rows` (after `programmes,`):

```ts
				ngccLevel: ps.ngcc_level ?? null,
				recommended: anyRecommended ? ps.recommended === true : true,
				implementation,
				findingsUrl: latest.findings_url ?? null,
```

In the legacy CSV `parseParameterSets`, add `ngccLevel: null, recommended: true, implementation: false, findingsUrl: null,` after `programmes: [],`.

- [ ] **Step 7: Update `src/lib/kemData.ts`**

In `processKemSchemes`, before `for (const ps of yaml.parametersets)`, add `const anyRecommended = yaml.parametersets.some((p) => p.recommended);`. Replace the `level` computation with the same nullable form as Step 6. In the loop add `const implementation = ps.implementation ?? yaml.implementation ?? false;`. Add to the pushed row (after `programmes,`):

```ts
				ngccLevel: ps.ngcc_level ?? null,
				recommended: anyRecommended ? ps.recommended === true : true,
				implementation,
				findingsUrl: yaml.findings_url ?? null,
```

In `filterKemRows`, replace `if (!filter.levels.has(row.level)) return false;` with:

```ts
		if (row.level === null || !filter.levels.has(row.level)) return false;
```

- [ ] **Step 8: Handle `level === null` in the store and components**

`src/lib/filterStore.ts:181`: replace `if (!$state.levels.has(row.level)) return false;` with:

```ts
					if (row.level === null || !$state.levels.has(row.level)) return false;
```

The level sort (`levelOrder[String(a.level)] ?? 99`) already places `null` last. Leave it.

`src/lib/components/ScatterPlot.svelte` and `src/lib/components/KemScatterPlot.svelte`: replace `levelLabel` with:

```ts
	function levelLabel(level: NistLevel | null): string {
		if (level === null) return 'Not a NIST level';
		if (level === 'Pre-Quantum') return 'N/A';
		return `Level ${level}`;
	}
```

`src/lib/components/SchemeTable.svelte:116` and `src/lib/components/KemTable.svelte:180`: replace `{row.level === 'Pre-Quantum' ? 'N/A' : row.level}` with:

```svelte
{row.level === null ? (row.ngccLevel ? `${row.ngccLevel}-bit` : '—') : row.level === 'Pre-Quantum' ? 'N/A' : row.level}
```

- [ ] **Step 9: Run tests and type-check**

Run: `npm run test && npm run check`
Expected: all PASS, 0 errors. If `svelte-check` reports another `NistLevel` / `NistLevel | null` mismatch, handle `null` at that site the same way. Treat it as "not a NIST level"; never map it to a NIST number.

- [ ] **Step 10: Run e2e to confirm nothing visible changed**

Run: `npm run test:e2e`
Expected: all PASS. The current data has no NGCC-scale rows and no `recommended` flags.

- [ ] **Step 11: Document the fields in `CLAUDE.md`**

In the signature YAML schema block, add under the parameter-set fields:

```yaml
        level: 2               # NIST category 1-5 or "Pre-Quantum" …
        ngcc_level: 128        # … OR China NGCC classical bits (128/256/384/512); never both
        recommended: true      # optional; if any set in a version has it, only those show by default
        implementation: "…"    # optional; bugs in submitted code (never sets broken)
```

and under the version fields `findings_url: https://ngcc.dev/reports/sign-02.html   # optional link to third-party findings`. Add the same fields to the KEM block (scheme level for `implementation` and `findings_url`). Add one sentence under "Performance Data": "Never import third-party benchmark numbers; timings come from `bench/` and `bench-kem/` only."

- [ ] **Step 12: Commit**

```bash
git add src/lib/types.ts src/lib/data.ts src/lib/kemData.ts src/lib/filterStore.ts src/lib/components/ScatterPlot.svelte src/lib/components/KemScatterPlot.svelte src/lib/components/SchemeTable.svelte src/lib/components/KemTable.svelte src/lib/__tests__/data.test.ts src/lib/__tests__/kemData.test.ts src/lib/__tests__/filterStore.test.ts src/lib/__tests__/dataFiles.test.ts CLAUDE.md
git commit -m "Add NGCC security levels, recommended sets and implementation findings to the data model"
```

---

### Task 4: Site-wide trailing slash and site name constant

**Files:**
- Create: `src/routes/+layout.ts`
- Modify: `src/lib/constants.ts`
- Modify: `src/routes/+layout.svelte` (brand label), `src/routes/history/+page.svelte` (`<title>`)

**Interfaces:**
- Produces: `SITE_NAME: string` from `src/lib/constants.ts`.

- [ ] **Step 1: Add the constant to `src/lib/constants.ts`**

```ts
/** Display name of the site. Plan 5 (rename) changes it here only. */
export const SITE_NAME = 'NIST Signatures Zoo';
```

- [ ] **Step 2: Create `src/routes/+layout.ts`**

```ts
export const prerender = true;
// Every page is written as <route>/index.html, so /signatures/ and /advanced/ resolve on GitHub Pages.
export const trailingSlash = 'always';
```

- [ ] **Step 3: Use `SITE_NAME`**

In `src/routes/+layout.svelte`, import it (`import { SITE_NAME } from '$lib/constants';`) and replace the literal `NIST Signatures Zoo` in the brand badge with `{SITE_NAME}`. In `src/routes/history/+page.svelte`, change the title to `<title>History — {SITE_NAME}</title>` and add the same import. Run `grep -rn "NIST Signatures Zoo" src/` and replace every other literal the same way.

- [ ] **Step 4: Build and confirm the output layout**

Run: `npm run build && ls dist/advanced/index.html dist/kems/index.html dist/history/index.html`
Expected: all three files exist.

- [ ] **Step 5: Run the full test suites**

Run: `npm run test && npm run check && npm run test:e2e`
Expected: all PASS.

- [ ] **Step 6: Commit**

```bash
git add src/routes/+layout.ts src/lib/constants.ts src/routes/+layout.svelte src/routes/history/+page.svelte
git commit -m "Add SITE_NAME constant and site-wide trailing slashes"
```

(If Step 3's grep touched other files, add them by name too.)

---

### Task 5: Legacy URL rewriting (pure functions)

**Files:**
- Create: `src/lib/legacy.ts`
- Test: `src/lib/__tests__/legacy.test.ts`

**Interfaces:**
- Produces:
  - `SIGNATURE_PARAMS: readonly string[]`: every query key the old signature home page understood.
  - `legacyHomeRedirect(search: string, hash: string, base: string): string | null`: the `/signatures/` URL when `search` carries any signature param, otherwise `null`.
  - `legacyPathRedirect(target: string, search: string, hash: string, base: string): string`: `base + '/' + target + search + hash`, where `target` is something like `'signatures/advanced/'`.

- [ ] **Step 1: Write the failing tests `src/lib/__tests__/legacy.test.ts`**

```ts
import { describe, it, expect } from 'vitest';
import { legacyHomeRedirect, legacyPathRedirect } from '$lib/legacy';

describe('legacyHomeRedirect', () => {
	it('forwards a round + filter link to /signatures/ unchanged', () => {
		expect(legacyHomeRedirect('?r=2&s=MAYO,UOV&sort=sig', '', '')).toBe('/signatures/?r=2&s=MAYO,UOV&sort=sig');
	});
	it('keeps the hash', () => {
		expect(legacyHomeRedirect('?l=1,2', '#table', '')).toBe('/signatures/?l=1,2#table');
	});
	it('prefixes the deploy base path', () => {
		expect(legacyHomeRedirect('?r=1', '', '/nist-sigs-zoo')).toBe('/nist-sigs-zoo/signatures/?r=1');
	});
	it('forwards unknown round values rather than dropping them', () => {
		expect(legacyHomeRedirect('?r=7', '', '')).toBe('/signatures/?r=7');
	});
	it('returns null for a plain home visit', () => {
		expect(legacyHomeRedirect('', '', '')).toBeNull();
	});
	it('returns null for unrelated params such as analytics', () => {
		expect(legacyHomeRedirect('?utm_source=x', '', '')).toBeNull();
	});
});

describe('legacyPathRedirect', () => {
	it('moves /advanced/ with its axis params', () => {
		expect(legacyPathRedirect('signatures/advanced/', '?x=pkPlusSig&ys=linear', '', '')).toBe(
			'/signatures/advanced/?x=pkPlusSig&ys=linear'
		);
	});
	it('moves /history/ with base and hash', () => {
		expect(legacyPathRedirect('timeline/', '', '#2026-09-22', '/nist-sigs-zoo')).toBe('/nist-sigs-zoo/timeline/#2026-09-22');
	});
});
```

- [ ] **Step 2: Run them to verify they fail**

Run: `npm run test -- legacy`
Expected: FAIL with `Failed to resolve import "$lib/legacy"`.

- [ ] **Step 3: Implement `src/lib/legacy.ts`**

```ts
/** Query keys of the pre-reorganisation signature page (filterStore.ts URL codec + round). */
export const SIGNATURE_PARAMS = [
	'r', 's', 'l',
	'pkMin', 'pkMax', 'sigMin', 'sigMax', 'psMin', 'psMax',
	'scMin', 'scMax', 'vcMin', 'vcMax', 'suMin', 'suMax', 'vuMin', 'vuMax',
	'sort', 'dir'
] as const;

export function legacyPathRedirect(target: string, search: string, hash: string, base: string): string {
	return `${base}/${target}${search}${hash}`;
}

export function legacyHomeRedirect(search: string, hash: string, base: string): string | null {
	const params = new URLSearchParams(search);
	const keys = [...params.keys()];
	if (!keys.some((k) => (SIGNATURE_PARAMS as readonly string[]).includes(k))) return null;
	return legacyPathRedirect('signatures/', search, hash, base);
}
```

- [ ] **Step 4: Run the tests to verify they pass**

Run: `npm run test -- legacy`
Expected: PASS (8 tests).

- [ ] **Step 5: Commit**

```bash
git add src/lib/legacy.ts src/lib/__tests__/legacy.test.ts
git commit -m "Add legacy URL rewriting helpers"
```

---

### Task 6: Move routes and add redirect stubs

**Files:**
- Move: `src/routes/+page.ts`, `src/routes/+page.svelte` → `src/routes/signatures/`
- Move: `src/routes/advanced/` → `src/routes/signatures/advanced/`
- Move: `src/routes/history/` → `src/routes/timeline/`
- Create: `src/lib/components/LegacyRedirect.svelte`
- Create: `src/routes/advanced/+page.svelte`, `src/routes/history/+page.svelte`
- Create (temporary, replaced in Task 8): `src/routes/+page.svelte`
- Modify: `src/routes/signatures/+page.svelte` (internal links to `advanced/`)
- Move: `e2e/main.spec.ts` → `e2e/signatures.spec.ts`
- Modify: `e2e/advanced.spec.ts`, `e2e/kems.spec.ts`
- Create: `e2e/redirects.spec.ts`

**Interfaces:**
- Consumes: `legacyHomeRedirect`, `legacyPathRedirect` (Task 5).
- Produces: `<LegacyRedirect to="signatures/advanced/" title="Advanced graph" />`. The `to` prop is relative to `base`, with no leading slash.

- [ ] **Step 1: Write the failing e2e test `e2e/redirects.spec.ts`**

```ts
import { test, expect } from '@playwright/test';

test.describe('Legacy URLs', () => {
	test('old home link with filters lands on /signatures/ with the same query', async ({ page }) => {
		await page.goto('/?r=2&sort=sig');
		await page.waitForURL(/\/signatures\/\?r=2&sort=sig$/, { timeout: 15_000 });
		await expect(page.locator('h1')).toHaveText('Post-Quantum Signature Schemes');
	});

	test('/advanced/ keeps its axis params', async ({ page }) => {
		await page.goto('/advanced/?x=pkPlusSig&ys=linear');
		await page.waitForURL(/\/signatures\/advanced\/\?x=pkPlusSig&ys=linear$/, { timeout: 15_000 });
		await expect(page.locator('h1')).toHaveText('Advanced Graph');
	});

	test('/history/ moves to /timeline/', async ({ page }) => {
		await page.goto('/history/');
		await page.waitForURL(/\/timeline\/$/, { timeout: 15_000 });
		await expect(page.locator('h1')).toHaveText('History');
	});

	test('redirect stubs declare the canonical URL for search engines', async ({ request }) => {
		const html = await (await request.get('/advanced/')).text();
		expect(html).toContain('rel="canonical"');
		expect(html).toContain('/signatures/advanced/');
	});
});
```

- [ ] **Step 2: Run it to verify it fails**

Run: `npx playwright test e2e/redirects.spec.ts`
Expected: FAIL. `/` does not navigate to `/signatures/`.

- [ ] **Step 3: Move the routes with git**

```bash
mkdir -p src/routes/signatures src/routes/timeline
git mv src/routes/+page.ts src/routes/signatures/+page.ts
git mv src/routes/+page.svelte src/routes/signatures/+page.svelte
git mv src/routes/advanced src/routes/signatures/advanced
git mv src/routes/history/+page.ts src/routes/timeline/+page.ts
git mv src/routes/history/+page.svelte src/routes/timeline/+page.svelte
git mv e2e/main.spec.ts e2e/signatures.spec.ts
```

- [ ] **Step 4: Fix relative imports in the moved files**

`src/routes/timeline/+page.ts` globs `'../../../data/history.yaml'`, and the depth is unchanged. Moving `+page.svelte` one level down does not affect `$lib` imports. Fix the links:

- In `src/routes/signatures/+page.svelte`, change `href="{base}/advanced/"` to `href="{base}/signatures/advanced/"`.
- In `src/routes/signatures/advanced/+page.svelte`, change any link back to `{base}/` to `{base}/signatures/`.

Check with `grep -n 'base}/' src/routes/signatures -r`.

- [ ] **Step 5: Create `src/lib/components/LegacyRedirect.svelte`**

```svelte
<script lang="ts">
	import { onMount } from 'svelte';
	import { base } from '$app/paths';
	import { legacyPathRedirect } from '$lib/legacy';
	import { SITE_NAME } from '$lib/constants';

	interface Props {
		/** Target path relative to base, no leading slash, e.g. "signatures/advanced/". */
		to: string;
		title: string;
	}
	let { to, title }: Props = $props();

	const href = $derived(`${base}/${to}`);

	onMount(() => {
		location.replace(legacyPathRedirect(to, location.search, location.hash, base));
	});
</script>

<svelte:head>
	<title>{title} — {SITE_NAME}</title>
	<link rel="canonical" {href} />
	<meta http-equiv="refresh" content="0; url={href}" />
	<meta name="robots" content="noindex" />
</svelte:head>

<div class="mx-auto max-w-screen-md px-6 py-10">
	<p class="text-sm">This page has moved to <a class="text-pqs-steel underline dark:text-pqs-apricot" {href}>{title}</a>.</p>
</div>
```

The meta refresh is the fallback when JavaScript is off: it drops the query string, but it still reaches the right page. The JS redirect keeps the query and hash.

- [ ] **Step 6: Create the stubs**

`src/routes/advanced/+page.svelte`:

```svelte
<script lang="ts">
	import LegacyRedirect from '$lib/components/LegacyRedirect.svelte';
</script>

<LegacyRedirect to="signatures/advanced/" title="Advanced graph" />
```

`src/routes/history/+page.svelte`:

```svelte
<script lang="ts">
	import LegacyRedirect from '$lib/components/LegacyRedirect.svelte';
</script>

<LegacyRedirect to="timeline/" title="Timeline" />
```

- [ ] **Step 7: Create a temporary home `src/routes/+page.svelte` that forwards legacy links**

Task 8 replaces the body with the real home page and keeps the `onMount` block.

```svelte
<script lang="ts">
	import { onMount } from 'svelte';
	import { base } from '$app/paths';
	import { legacyHomeRedirect } from '$lib/legacy';

	onMount(() => {
		const target = legacyHomeRedirect(location.search, location.hash, base);
		if (target) location.replace(target);
	});
</script>

<div class="mx-auto max-w-screen-2xl px-6 py-8">
	<a href="{base}/signatures/">Signatures</a>
</div>
```

- [ ] **Step 8: Point the existing e2e specs at the new paths**

```bash
sed -i "s#page.goto('/advanced/#page.goto('/signatures/advanced/#g" e2e/advanced.spec.ts
sed -i "s#page.goto('/')#page.goto('/signatures/')#g" e2e/signatures.spec.ts
```

In `e2e/signatures.spec.ts`, rename `test.describe('Main page'` to `test.describe('Signatures page'`. In `e2e/kems.spec.ts`, in the test `'reachable via the header nav link'`, change `await page.goto('/');` to `await page.goto('/signatures/');`.

- [ ] **Step 9: Run every test**

Run: `npm run test && npm run check && npm run test:e2e`
Expected: all PASS, including `redirects.spec.ts`.

- [ ] **Step 10: Update `CLAUDE.md`**

In the Architecture tree, move the `+page.*` and `advanced/` entries under `signatures/`, rename `history/` to `timeline/`, add `advanced/` and `history/` as "redirect stubs (LegacyRedirect)", and add `legacy.ts`, `programmes.ts` and `programmeData.ts` to `src/lib`. In the Testing section, rename `main.spec.ts` to `signatures.spec.ts` and add `redirects.spec.ts`.

- [ ] **Step 11: Commit**

```bash
git add src/routes src/lib/components/LegacyRedirect.svelte e2e/signatures.spec.ts e2e/advanced.spec.ts e2e/kems.spec.ts e2e/redirects.spec.ts CLAUDE.md
git commit -m "Move signatures to /signatures/ and timeline to /timeline/ with redirects"
```

---

### Task 7: Layout navigation and round selector on the signatures page

**Files:**
- Create: `src/lib/components/RoundSelect.svelte`
- Modify: `src/routes/+layout.svelte` (remove both round button groups and `isHome`/`isKems`; new link list)
- Modify: `src/routes/signatures/+page.svelte` (render `RoundSelect` in the hero)
- Modify: `e2e/signatures.spec.ts` (round selector test), `e2e/kems.spec.ts` (Signatures link test)

**Interfaces:**
- Consumes: `roundStore`, `Round` (Task 2).
- Produces: `<RoundSelect />` writes to `roundStore`, with the accessible name "Dataset".

- [ ] **Step 1: Update the e2e expectations (failing first)**

In `e2e/signatures.spec.ts`, replace the test `'round selector shows Latest/Round 3/Round 2/Round 1'` with:

```ts
	test('dataset selector offers Latest and rounds 3–1', async ({ page }) => {
		await page.goto('/signatures/');
		const select = page.getByLabel('Dataset');
		await expect(select).toBeVisible();
		await expect(select.locator('option')).toHaveText(['Latest specs', 'Round 3', 'Round 2', 'Round 1']);
	});

	test('choosing Round 2 updates the URL', async ({ page }) => {
		await page.goto('/signatures/');
		await page.waitForLoadState('networkidle');
		await page.getByLabel('Dataset').selectOption('round-2');
		await page.waitForURL(/[?&]r=2/, { timeout: 15_000 });
	});
```

Add a nav test to `e2e/kems.spec.ts`, replacing `'has a Signatures link back to the home page'`:

```ts
	test('header links to Signatures, KEMs and Timeline', async ({ page }) => {
		await page.goto('/kems/');
		const nav = page.getByRole('navigation').first();
		await expect(nav.getByRole('link', { name: 'Signatures' })).toHaveAttribute('href', /\/signatures\/$/);
		await expect(nav.getByRole('link', { name: 'KEMs' })).toHaveAttribute('aria-current', 'page');
		await expect(nav.getByRole('link', { name: 'Timeline' })).toHaveAttribute('href', /\/timeline\/$/);
	});
```

Run: `npx playwright test e2e/signatures.spec.ts e2e/kems.spec.ts`
Expected: FAIL. There is no "Dataset" control, and the nav shows "History".

- [ ] **Step 2: Create `src/lib/components/RoundSelect.svelte`**

```svelte
<script lang="ts">
	import { roundStore, type Round } from '$lib/roundStore';

	const OPTIONS: { value: Round; label: string }[] = [
		{ value: 'latest', label: 'Latest specs' },
		{ value: 'round-3', label: 'Round 3' },
		{ value: 'round-2', label: 'Round 2' },
		{ value: 'round-1', label: 'Round 1' }
	];
</script>

<label class="inline-flex items-center gap-2 font-heading text-sm text-pqs-steel dark:text-pqs-bluegray">
	Dataset
	<select
		class="rounded border border-pqs-bluegray bg-white px-2 py-1 text-pqs-midnight dark:border-pqs-steel dark:bg-pqs-midnight-mid dark:text-pqs-smoke"
		value={$roundStore}
		onchange={(e) => roundStore.set(e.currentTarget.value as Round)}
	>
		{#each OPTIONS as opt (opt.value)}
			<option value={opt.value}>{opt.label}</option>
		{/each}
	</select>
</label>
```

- [ ] **Step 3: Render it on the signatures page**

In `src/routes/signatures/+page.svelte`, import it (`import RoundSelect from '$lib/components/RoundSelect.svelte';`) and add it as the first child after the hero `<p>` lines, inside the `border-l-4` hero div:

```svelte
		<div class="mt-3"><RoundSelect /></div>
```

- [ ] **Step 4: Rewrite the nav in `src/routes/+layout.svelte`**

- Delete the `isHome`, `isKems` and `setRound` declarations, the `roundStore` import, and both `{#if isHome}…{/if}` round button blocks (desktop and mobile).
- Add `const path = $derived($page.url.pathname.slice(base.length));`
- Add `const NAV = [{ href: 'signatures/', label: 'Signatures' }, { href: 'kems/', label: 'KEMs' }, { href: 'timeline/', label: 'Timeline' }];`
- Replace the desktop `{#if isKems}…{/if}` link pair and the History link with:

```svelte
				{#each NAV as item (item.href)}
					<a
						href="{base}/{item.href}"
						aria-current={path.startsWith('/' + item.href) ? 'page' : undefined}
						class="transition-colors hover:text-pqs-apricot {path.startsWith('/' + item.href) ? 'text-white' : 'text-pqs-bluegray'}"
					>
						{item.label}
					</a>
				{/each}
```

- In the mobile dropdown, replace the KEMs/Signatures `{#if}` and the History link with the same `{#each NAV}` block, adding `onclick={closeMenu}` to the `<a>`.

- [ ] **Step 5: Run every test**

Run: `npm run test && npm run check && npm run test:e2e`
Expected: all PASS.

- [ ] **Step 6: Commit**

```bash
git add src/lib/components/RoundSelect.svelte src/routes/+layout.svelte src/routes/signatures/+page.svelte e2e/signatures.spec.ts e2e/kems.spec.ts
git commit -m "Move round selector into the signatures page and simplify nav"
```

---

### Task 8: Home page

**Files:**
- Create: `src/routes/+page.ts`
- Rewrite: `src/routes/+page.svelte` (keep the `onMount` legacy redirect from Task 6)
- Create: `e2e/home.spec.ts`

**Interfaces:**
- Consumes: `allProgrammes` (Task 1), `programmeSummaries`, `ProgrammeSummary` (Task 1), `allSchemeData`, `lastUpdated` (`src/lib/schemeData.ts`), `allKemData` (`src/lib/kemSchemeData.ts`), `HistoryEntry` (`src/lib/types.ts`), `legacyHomeRedirect` (Task 5).
- Produces: the page data `{ summaries: ProgrammeSummary[]; signatureCount: number; kemCount: number; latest: HistoryEntry[] }`.

- [ ] **Step 1: Write the failing e2e test `e2e/home.spec.ts`**

```ts
import { test, expect } from '@playwright/test';

test.describe('Home page', () => {
	test('lists every programme', async ({ page }) => {
		await page.goto('/');
		const list = page.getByRole('region', { name: 'Programmes' });
		for (const name of ['NIST', 'On-ramp', 'NGCC', 'KpqC', 'ISO']) {
			await expect(list.getByText(name, { exact: false }).first()).toBeVisible();
		}
	});

	test('shows KpqC signature picks', async ({ page }) => {
		await page.goto('/');
		await expect(page.getByRole('region', { name: 'Programmes' })).toContainText('AIMer, HAETAE');
	});

	test('links to the signatures and KEM comparisons', async ({ page }) => {
		await page.goto('/');
		await expect(page.getByRole('link', { name: 'Compare signatures' })).toHaveAttribute('href', /\/signatures\/$/);
		await expect(page.getByRole('link', { name: 'Compare KEMs' })).toHaveAttribute('href', /\/kems\/$/);
	});

	test('shows the three latest history entries', async ({ page }) => {
		await page.goto('/');
		await expect(page.getByRole('region', { name: 'Latest changes' }).getByRole('listitem')).toHaveCount(3);
	});

	test('plain visit does not redirect', async ({ page }) => {
		await page.goto('/');
		await page.waitForLoadState('networkidle');
		expect(new URL(page.url()).pathname).toBe('/');
	});
});
```

Run: `npx playwright test e2e/home.spec.ts`
Expected: FAIL. There is no "Programmes" region.

- [ ] **Step 2: Create `src/routes/+page.ts`**

```ts
import { allProgrammes } from '$lib/programmeData';
import { programmeSummaries } from '$lib/programmes';
import { allSchemeData } from '$lib/schemeData';
import { allKemData } from '$lib/kemSchemeData';
import type { HistoryEntry } from '$lib/types';

const raw = import.meta.glob('../../data/history.yaml', { eager: true }) as Record<
	string,
	{ default: HistoryEntry[] }
>;
const history: HistoryEntry[] = Object.values(raw)[0]?.default ?? [];

export function load() {
	const pq = (cat: string) => cat !== 'Pre-Quantum';
	return {
		summaries: programmeSummaries(allProgrammes, allSchemeData, allKemData),
		signatureCount: allSchemeData.filter((s) => pq(s.category)).length,
		kemCount: allKemData.filter((k) => pq(k.category)).length,
		latest: [...history].sort((a, b) => b.date.localeCompare(a.date)).slice(0, 3)
	};
}
```

- [ ] **Step 3: Write `src/routes/+page.svelte`**

```svelte
<script lang="ts">
	import { onMount } from 'svelte';
	import { base } from '$app/paths';
	import { legacyHomeRedirect } from '$lib/legacy';
	import { SITE_NAME } from '$lib/constants';
	import { lastUpdated } from '$lib/schemeData';
	import type { PageData } from './$types';

	let { data }: { data: PageData } = $props();

	onMount(() => {
		const target = legacyHomeRedirect(location.search, location.hash, base);
		if (target) location.replace(target);
	});

	const list = (xs: string[]) => (xs.length ? xs.join(', ') : '—');
</script>

<svelte:head>
	<title>{SITE_NAME}</title>
</svelte:head>

<section class="bg-pqs-midnight text-pqs-smoke">
	<div class="mx-auto max-w-screen-2xl px-6 py-16">
		<h1 class="max-w-[18ch] font-heading text-4xl font-bold leading-tight md:text-5xl">
			Post-quantum schemes, compared across standards programmes.
		</h1>
		<p class="mt-5 max-w-[58ch] text-lg text-pqs-bluegray">
			Sizes, benchmarks and security status for signatures and KEMs from NIST, China's ICCS,
			Korea's KpqC and ISO, updated as specs change and attacks land.
		</p>
		<div class="mt-8 flex flex-wrap gap-3 font-heading font-semibold">
			<a href="{base}/signatures/" class="rounded bg-pqs-apricot px-5 py-3 text-pqs-midnight">Compare signatures</a>
			<a href="{base}/kems/" class="rounded border border-pqs-bluegray px-5 py-3 text-white">Compare KEMs</a>
		</div>
		<dl class="mt-12 grid grid-cols-2 gap-6 border-t border-pqs-steel/40 pt-6 font-heading sm:grid-cols-4">
			<div class="flex flex-col-reverse"><dt class="text-sm text-pqs-bluegray">signature schemes</dt><dd class="text-3xl font-bold">{data.signatureCount}</dd></div>
			<div class="flex flex-col-reverse"><dt class="text-sm text-pqs-bluegray">KEMs</dt><dd class="text-3xl font-bold">{data.kemCount}</dd></div>
			<div class="flex flex-col-reverse"><dt class="text-sm text-pqs-bluegray">programmes</dt><dd class="text-3xl font-bold">{data.summaries.length}</dd></div>
			<div class="flex flex-col-reverse"><dt class="text-sm text-pqs-bluegray">last data change</dt><dd class="text-3xl font-bold">{lastUpdated}</dd></div>
		</dl>
	</div>
</section>

<div class="mx-auto max-w-screen-2xl space-y-14 px-6 py-14">
	<section aria-labelledby="programmes-h">
		<h2 id="programmes-h" class="mb-4 font-heading text-2xl font-bold">Programmes</h2>
		<div class="overflow-x-auto rounded border border-pqs-ashgray bg-white dark:border-pqs-steel dark:bg-pqs-midnight-mid">
			<table class="min-w-full text-sm">
				<thead>
					<tr class="bg-pqs-steel font-heading text-xs text-white">
						<th scope="col" class="px-3 py-2.5 text-left font-semibold">Programme</th>
						<th scope="col" class="px-3 py-2.5 text-left font-semibold">Signatures</th>
						<th scope="col" class="px-3 py-2.5 text-left font-semibold">KEMs</th>
					</tr>
				</thead>
				<tbody class="divide-y divide-pqs-ashgray dark:divide-pqs-steel">
					{#each data.summaries as s (s.programme.id)}
						<tr>
							<td class="px-3 py-2">
								<span class="font-heading font-semibold">{s.programme.name}</span>
								<span class="block text-xs text-pqs-steel dark:text-pqs-bluegray">{s.programme.short} · {s.programme.organiser} · {s.programme.region}</span>
							</td>
							<td class="px-3 py-2">{s.programme.primitives.includes('signature') ? list(s.signatures) : 'Not in scope'}</td>
							<td class="px-3 py-2">{s.programme.primitives.includes('kem') ? list(s.kems) : 'Not in scope'}</td>
						</tr>
					{/each}
				</tbody>
			</table>
		</div>
	</section>

	<section aria-labelledby="latest-h" class="mx-auto max-w-[880px]">
		<div class="mb-2 flex items-baseline justify-between">
			<h2 id="latest-h" class="font-heading text-2xl font-bold">Latest changes</h2>
			<a href="{base}/timeline/" class="font-heading text-sm font-semibold text-pqs-steel dark:text-pqs-apricot">Full timeline</a>
		</div>
		<ol class="divide-y divide-pqs-ashgray border-y border-pqs-ashgray dark:divide-pqs-steel dark:border-pqs-steel">
			{#each data.latest as e (e.date + e.description)}
				<li class="grid gap-1 py-4 sm:grid-cols-[112px_1fr] sm:gap-6">
					<div class="font-heading text-sm">
						<time class="block text-pqs-steel dark:text-pqs-bluegray">{e.date}</time>
						<span class="font-semibold capitalize">{e.type}</span>
					</div>
					<p class="text-sm [&_a]:text-pqs-steel [&_a]:underline dark:[&_a]:text-pqs-apricot">{@html e.description}</p>
				</li>
			{/each}
		</ol>
	</section>
</div>
```

`aria-labelledby` makes each `<section>` a named region, which the e2e test looks up by name. NGCC shows "—" until plan 3 adds its data, which is the honest state.

Note on `{@html}`: `history.yaml` is repo-controlled and already rendered with `{@html}` on the timeline page. Do not use this pattern for anything user-supplied.

- [ ] **Step 4: Run every test**

Run: `npm run test && npm run check && npm run test:e2e`
Expected: all PASS, including `home.spec.ts` and the plain-visit no-redirect test.

- [ ] **Step 5: Check by hand at phone and desktop width**

Run `npm run dev`. Open `http://localhost:5173/` at 390 px and at 1440 px wide, in light and dark theme. Check that the programme table scrolls only on the phone and that the hero text doesn't overflow. Then open `http://localhost:5173/?r=2&s=MAYO` and confirm it lands on `/signatures/?r=2&s=MAYO` with only MAYO selected.

- [ ] **Step 6: Update `CLAUDE.md` and commit**

Add `+page.ts` / `+page.svelte` (home: programme summaries, latest changes, legacy redirect) to the Architecture tree and `home.spec.ts` to Testing.

```bash
git add src/routes/+page.ts src/routes/+page.svelte e2e/home.spec.ts CLAUDE.md
git commit -m "Add programme overview home page"
```

---

### Task 9: Preview deploy check

**Files:** none changed unless the check fails.

- [ ] **Step 1: Build with the production base path**

Run: `BASE_PATH=/nist-sigs-zoo npm run build && grep -o 'nist-sigs-zoo/signatures/advanced/' dist/advanced/index.html | head -1`
Expected: prints `nist-sigs-zoo/signatures/advanced/`. This shows the canonical link and meta refresh carry the base path.

- [ ] **Step 2: Push the branch and open a PR (confirm with Thom first)**

Pushing is outward-facing, so ask before running:

```bash
git push -u origin feat/programmes-foundation
gh pr create --title "Programme data model, route reorganisation and home page" --body "$(cat <<'EOF'
Part 1 of the site reorganisation (plan: docs/superpowers/plans/2026-10-07-site-reorganization-foundation.md).

- data/programmes.yaml registry; scheme versions and KEMs record programme memberships instead of round tags
- parameter sets can state an NGCC level (classical bits) instead of a NIST level, a recommended flag, and implementation findings separate from design breaks (no NGCC data yet)
- signatures move to /signatures/, history to /timeline/; old URLs redirect and keep their query strings
- new home page listing programmes and latest changes
- round selector moves from the nav into the signatures page

🤖 Generated with [Claude Code](https://claude.com/claude-code)
EOF
)"
```

- [ ] **Step 3: Check the Cloudflare preview**

Open the `https://pr-N.nist-sigs-zoo.pages.dev` link from the sticky PR comment. Repeat Task 8 Step 5's checks there, plus `/advanced/?x=sig` and `/history/`.
