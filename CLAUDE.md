# CLAUDE.md

This file provides guidance to Claude Code when working with code in this repository.

## Overview

Static website comparing post-quantum signature schemes submitted to the NIST on-ramp.
Built with SvelteKit (adapter-static) + Tailwind CSS v4 + TypeScript.
Deploy: `npm run build` → `dist/`. GitHub Actions deploys to GitHub Pages.

## Stack

- **Framework**: SvelteKit 2 + Svelte 5 (runes mode), adapter-static → `dist/`
- **CSS**: Tailwind CSS v4 via `@tailwindcss/vite`
- **Charts**: Vega-Lite (log-log scatter of pk vs sig size)
- **Language**: TypeScript throughout

## Development

```bash
npm run dev       # dev server at http://localhost:5173
npm run build     # build to dist/
npm run preview   # preview dist/ at http://localhost:4173
npm run check     # type-check with svelte-check
```

## Testing

```bash
npm run test          # unit tests (Vitest, fast, no browser)
npm run test:watch    # unit tests in watch mode
npm run test:e2e      # E2E tests (Playwright, builds site first)
```

### Unit tests — `src/lib/__tests__/*.test.ts`

Vitest with node environment. Tests pure TypeScript functions only — no Svelte components, no browser.

- `data.test.ts` — `processYamlSchemes()`: tag filtering, version selection, field computation, flag propagation
- `filterStore.test.ts` — `buildUrlParams()`: URL encoding of filter state

**Adding unit tests:** create `src/lib/__tests__/<module>.test.ts`. Import directly from `$lib/...`.
Pass mock `SchemeYaml[]` objects to `processYamlSchemes` — no fixture files needed.
Do not import from `$app/*` or `$lib/schemeData` (these need the Vite/SvelteKit runtime).

### E2E tests — `e2e/*.spec.ts`

Playwright against a built static site. The playwright config runs `npm run build && npm run preview -- --port 4175` automatically (`reuseExistingServer: true`, so a running preview server is reused for speed).

- `main.spec.ts` — main page: heading, Vega chart render, advanced link, round selector, table
- `advanced.spec.ts` — advanced page: axis controls, heading updates, URL encoding/restoration, filter panel

**Adding E2E tests:** add to `e2e/main.spec.ts` or `e2e/advanced.spec.ts`, or create a new `e2e/<feature>.spec.ts`.
Use `page.waitForFunction(() => [...document.querySelectorAll('svg')].some(s => s.querySelector('g')), { timeout: 15_000 })` to wait for the Vega chart to render before asserting on it.
Axis URL params: `x`, `y` (field name), `xs`, `ys` (scale: `log`|`linear`). Default values omitted from URL.

## Data

Source of truth is `data/schemes/*.yaml` — one YAML file per scheme.

Schema per file:
```yaml
name: SchemeName
website: https://...
category: Lattice | Multivariate | Hash-based | ...
assumption: ...
versions:
  - version: "FIPS 206"        # human-readable version label
    date: "2024-08-13"         # ISO date, used to pick latest
    status: Standardized | On-ramp
    tags: [round-2]            # round-1, round-2, round-3, or omit for reference/standardized schemes
                               # post-round-3 updates should have NO tags
    broken: false              # or string description / "classical"
    warning: false             # or string description
    info: false                # or string description
    parametersets:
      - name: ML-DSA-44
        level: 2               # 1-5 or "Pre-Quantum"
        pk: 1312
        sig: 2420
        signing_cycles: 1234567
        verification_cycles: 234567
        signing_us: null       # microseconds; use cycles OR us, not both
        verification_us: null
        notes: null
```

Security flags (`broken`/`warning`/`info`) can be set at version level (applies to all
parametersets) or overridden per-parameterset.

The YAML files are bundled at build time via a Vite plugin (`vite.config.ts`) and
`import.meta.glob` in `src/lib/schemeData.ts`.

### History

`data/history.yaml` — chronological log of notable events shown in the site's history panel.
**Update this file whenever scheme data changes** (spec updates, new attacks, milestone events).

```yaml
- date: '2026-05-26'          # ISO date
  type: update                 # update | attack | milestone
  description: "HTML string. Link tags allowed."
  schemes: [SNOVA]             # optional; list of affected scheme names
```

Types: `update` (spec/data change), `attack` (new cryptanalysis result), `milestone` (NIST process event).

### KEMs

`data/kems/*.yaml` — one YAML file per KEM, source of truth for the standalone KEM size
comparison at `/kems/`. This is a **one-off** page, not tied to a NIST round: no version
history, no round tags, no benchmarks — just sizes (pk, ct).

```yaml
name: ML-KEM
website: https://...
category: Lattices | Code-based | Pre-Quantum | ...
assumption: ...
status: FIPS | To be standardized | Classic cryptography | ...
broken: false        # or "classical" (pre-quantum schemes) / description
warning: false
info: false
parametersets:
  - name: ML-KEM-768
    level: 3           # 1-5 or "Pre-Quantum"
    pk: 1184
    ct: 1088
    notes: null
```

Flags (`broken`/`warning`/`info`) set at scheme level apply to all parameter sets, overridable
per parameter set. Loaded via `import.meta.glob` in `src/lib/kemSchemeData.ts`; flattened by
`processKemSchemes()` in `src/lib/kemData.ts` (pure, unit-tested). The page is self-contained
and does **not** reuse the signature `filterStore` singleton: `kemData.ts` also exports pure
`computeKemRanges()` / `defaultKemFilter()` / `filterKemRows()`, the page holds filter state in a
`$state` rune and derives `filteredRows`, and `KemFilterPanel` (scheme/level/size, `bind:filter`),
`KemTable` (local sort) and `KemScatterPlot` (pk vs ct, shape by category, colour by level) all
take plain props.

## Architecture

```
data/
├── schemes/          # one .yaml per signature scheme (source of truth)
├── kems/             # one .yaml per KEM (source of truth for /kems/)
└── history.yaml      # chronological event log (attacks, updates, milestones)

src/
├── lib/
│   ├── types.ts          # Scheme, ParameterSet, SchemeYaml, VersionYaml, FilterState, Kem* types
│   ├── constants.ts      # CPUSPEED, NIST_LEVELS
│   ├── data.ts           # processYamlSchemes() — tag-filtered YAML → Scheme[] + ParameterSet[]
│   ├── kemData.ts        # processKemSchemes() — KEM YAML → KemScheme[] + KemParameterSet[] (pure)
│   ├── filterStore.ts    # Svelte writable store + derived filteredRows + URL codec
│   ├── roundStore.ts     # writable<'round-1'|'round-2'|'round-3'|'latest'> — drives dataset selection
│   ├── schemeData.ts     # import.meta.glob loader → allSchemeData: SchemeYaml[]
│   ├── kemSchemeData.ts  # import.meta.glob loader → allKemData: KemSchemeYaml[]
│   ├── themeStore.ts     # dark/light/system theme store → localStorage
│   ├── yaml.d.ts         # TypeScript module declaration for *.yaml imports
│   └── components/
│       ├── SecurityBadge.svelte  # broken/warning/info badges with aria-labels
│       ├── FilterPanel.svelte    # filter controls (categories, levels, ranges)
│       ├── RangeField.svelte     # reusable number input
│       ├── SchemeTable.svelte    # sortable table (one row per parameter set)
│       ├── ScatterPlot.svelte    # Vega-Lite scatter plot; accepts xField/yField/xScale/yScale props
│       ├── KemTable.svelte       # KEM size table (rows prop, local sort)
│       ├── KemScatterPlot.svelte # KEM pk-vs-ct Vega-Lite scatter (rows prop)
│       └── KemFilterPanel.svelte # KEM scheme/level/size filters (bind:filter)
└── routes/
    ├── +layout.svelte    # nav (logo, round selector, Signatures/KEMs/History links, dark toggle), footer
    ├── +page.ts          # load: processYamlSchemes('round-3', {useLatestVersion:true}), createFilterStore
    ├── +page.svelte      # page composition, round switching, URL state sync
    ├── advanced/
    │   ├── +page.ts      # same load as main page
    │   └── +page.svelte  # axis selectors, scale toggles, ScatterPlot, FilterPanel, SchemeTable
    └── kems/
        ├── +page.ts      # load: processKemSchemes(allKemData); trailingSlash 'always'
        └── +page.svelte  # KEM hero, KemScatterPlot, KemTable

tests/
├── src/lib/__tests__/   # Vitest unit tests (vitest.config.ts)
│   ├── data.test.ts
│   ├── kemData.test.ts
│   └── filterStore.test.ts
└── e2e/                 # Playwright E2E tests (playwright.config.ts)
    ├── main.spec.ts
    ├── advanced.spec.ts
    └── kems.spec.ts
```

### Data Processing

`processYamlSchemes(allSchemeData, tagFilter?)` in `src/lib/data.ts`:
- With `tagFilter` (e.g. `'round-2'`): picks the latest version whose `tags` includes that value.
- Fallback: schemes with no tags on any version are always included (reference schemes like ML-DSA).
- Schemes that have tags but none matching `tagFilter` are excluded.

### Filter Store

`src/lib/filterStore.ts` uses stable module-level store references. `_store` and `_filteredRows`
are created once; `createFilterStore()` on subsequent calls (round changes) updates `_allRows`
and resets `_store` in place. This means components calling `getFilterStore()` at init always
hold valid references — no stale subscription bugs.

Category checkbox state is **derived** from the scheme set, not stored separately.

URL state: filter params encoded as query params, applied client-side in `onMount`.
Round encoded as `?r=1` for round-1, `?r=2` for round-2. No param = latest (default).

### Round Selector

Nav shows Latest / Round 2 / Round 1 toggle (only on home page). Clicking updates `roundStore`,
which triggers `applyRound()` in `+page.svelte`, which calls `createFilterStore()` with new data.

### Latest View

"Latest" (default) and "Round 3" both use `tagFilter='round-3'` with `useLatestVersion=true`.
Inclusion: scheme must have at least one version tagged `round-3` (or be an untagged reference
scheme). Data shown: `sorted[0]` (newest version by date, regardless of tags).

Round 2 view uses `tagFilter='round-2'` without `useLatestVersion` — shows the pinned
round-2 submission version.

Post-round-3 spec updates should be added as new version entries **without** any tags. This way:
- Round 2/3 views show pinned submission data.
- Latest view shows the most current specs.

### Performance Data

Cycles extrapolated from ms values use `CPUSPEED = 2_500_000_000` (2.5 GHz).
Extrapolated values shown with wavy red underline (`decoration-wavy decoration-red-500`).

## Deploy

GitHub Actions workflow at `.github/workflows/deploy.yml`:
1. `npm ci && npm run build` → `dist/`
2. Copy `round-1/` into `dist/round-1/` (untouched static snapshot)
3. Copy `.nojekyll` into `dist/`
4. Deploy `dist/` to GitHub Pages

## round-1/

Contains a standalone snapshot of round-1 data. **Do not modify.** Served at `/round-1/`.
