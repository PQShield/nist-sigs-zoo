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

- `data.test.ts` — `processYamlSchemes()`: stage filtering, version selection, field computation, flag propagation
- `kemData.test.ts` — `processKemSchemes()`, KEM ranges/filter helpers
- `filterStore.test.ts` — `buildUrlParams()`: URL encoding of filter state
- `compareFilters.test.ts` — programme groups, `CommonFilter` predicate, common URL codec
- `badges.test.ts` — badge derivation and programme chips
- `chartData.test.ts`, `chartLegend.test.ts` — chart axis values; legend + colour/shape scales
- `timing.test.ts` — displayed (extrapolated) times, shared by sorting, filters and tables
- `legacy.test.ts` — legacy URL forwarding; `programmes.test.ts` — programme definitions
- `programmePage.test.ts` — programme-page helpers (members, stage views, NGCC targets, findings summary, compare links, facts)
- `programmeRegistry.test.ts` — page fields in `data/programmes.yaml` (dates, links, level tables, NGCC values)
- `slugs.test.ts` — slugs and the cross-primitive index
- `versionCompare.test.ts` — version comparison (sets matched by name, percentage changes, flags per version, perf sources)
- `schemeHistory.test.ts`, `dates.test.ts` — history slice; date format (`fmtDate`, the single formatter, partial dates)
- `schemePage.test.ts` — scheme page model
- `chartNav.test.ts` — chart click vs. drag
- `timeline.test.ts` — entry programmes, timeline filter and URL codec; `urlSync.test.ts` — when the URL sync is skipped
- `html.test.ts` — HTML sanitising; `dataFiles.test.ts` — validates `data/**/*.yaml` (incl. history dates/types/programmes and programme primitives)

**Adding unit tests:** create `src/lib/__tests__/<module>.test.ts`. Import directly from `$lib/...`.
Pass mock `SchemeYaml[]` objects to `processYamlSchemes` — no fixture files needed.
Do not import from `$app/*` or `$lib/schemeData` (these need the Vite/SvelteKit runtime).

### E2E tests — `e2e/*.spec.ts`

Playwright against a built static site. The playwright config runs `npm run build && npm run preview -- --port 4175` automatically (`reuseExistingServer: true`, so a running preview server is reused for speed).

- `signatures.spec.ts` — signatures page (`/signatures/`): heading, Vega chart render, advanced link, round selector, table, shared-link filters
- `advanced.spec.ts` — signatures advanced page: axis controls, heading updates, URL encoding/restoration, filter panel
- `kems.spec.ts` — KEM compare page: filters, table, badges, chart (expected counts read from `data/kems/*.yaml`)
- `redirects.spec.ts` — legacy URLs (`/?filters`, `/advanced/`, `/history/`) forward to their new homes
- `home.spec.ts` — home page: programme table, KpqC picks, compare links, latest changes, no redirect on plain visit
- `schemes.spec.ts` — scheme pages, version comparison, links from tables and chart points
- `timeline.spec.ts` — timeline filters (type, programme), URL round trip, empty state
- `programmes.spec.ts` — `/programmes/` index and every `/programmes/<id>/` page: breadcrumb, facts, timeline, level table, findings box, stage / primitive toggles, NGCC empty state (reads `data/schemes`, so it passes with or without NGCC data), nav and home links

**Adding E2E tests:** add to `e2e/signatures.spec.ts` or `e2e/advanced.spec.ts`, or create a new `e2e/<feature>.spec.ts`.
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
slug: ntru-plus        # optional; URL slug for /schemes/<slug>/. Default: slugify(name). Needed only
                       # when two schemes (signature or KEM) clash or the name has no ASCII letters/digits
versions:
  - version: "FIPS 206"        # human-readable version label
    date: "2024-08-13"         # ISO date, used to pick latest
    status: Standardized | On-ramp
    programmes:                # programme id → stage id (see data/programmes.yaml)
      nist-onramp: round-3     # omit entirely for reference schemes (pre-quantum)
                               # a stage may be a list, e.g. [round-2, round-3], when one version spans several stages
    broken: false              # or string description / "classical"
    warning: false             # or string description
    info: false                # or string description
    findings_url: https://ngcc.dev/reports/sign-02.html   # optional link to third-party findings
    implementation: "..."      # optional; bugs in submitted code (never sets broken); ps-level overrides
    parametersets:
      - name: ML-DSA-44
        level: 2               # NIST category 1-5 or "Pre-Quantum" ...
        ngcc_level: 128        # ... OR China NGCC classical bits (128/256/384/512); never both
        recommended: true      # optional; if any set in a version has it, only those show by default
        implementation: "..."  # optional; bugs in submitted code (never sets broken)
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

`data/history.yaml` — chronological log of notable events, shown on `/timeline/` (filterable) and
in the home page's "Latest changes". **Update this file whenever scheme data changes** (spec
updates, new attacks, milestone events).

```yaml
- date: '2026-05-26'          # ISO date, quoted (unquoted dates become Date objects and fail the data test)
  type: update                 # update | attack | milestone | standardization
  description: "HTML string. Link tags allowed."
  schemes: [SNOVA]             # optional; signature or KEM `name:`s — each must exist (checked in dataFiles.test.ts; the build fails via slugOf)
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
`goto(…, { replaceState: true })`. Scheme names link to `/schemes/<slug>/`
(`schemeHref(base, slugOf(name))`); programme chips link to `/programmes/<id>/`. Dates are shown
with `fmtDate`.

### KEMs

`data/kems/*.yaml` — one YAML file per KEM, source of truth for the standalone KEM
comparison at `/kems/`. A single-snapshot page: no version history, but each KEM carries
scheme-level `programmes` memberships (programme -> stage, scalar or list; the last stage
is reported). Carries sizes (pk, ct) and benchmark timings.

```yaml
name: ML-KEM
website: https://...
category: Lattices | Code-based | Pre-Quantum | ...
assumption: ...
slug: ntru-plus        # optional; URL slug for /schemes/<slug>/. Default: slugify(name). Needed only
                       # when two schemes (signature or KEM) clash or the name has no ASCII letters/digits
status: FIPS | To be standardized | Classic cryptography | ...
programmes:          # programme id → stage id (see data/programmes.yaml); optional
  nist-pqc: standardised
broken: false        # or "classical" (pre-quantum schemes) / description
warning: false
info: false
implementation: "..."   # optional; bugs in submitted code (never sets broken)
findings_url: https://...   # optional link to third-party findings
parametersets:
  - name: ML-KEM-768
    level: 3           # NIST category 1-5 or "Pre-Quantum" ...
    ngcc_level: 128    # ... OR China NGCC classical bits (128/256/384/512); never both
    recommended: true  # optional; if any set has it, only those show by default
    implementation: "..."  # optional
    pk: 1184
    ct: 1088
    keygen_cycles: 20016    # optional; written by bench-kem/update_scheme_data.py
    encaps_cycles: 20067
    decaps_cycles: 21264
    keygen_us: 13.4
    encaps_us: 13.4
    decaps_us: 14.2
    notes: null
```

Benchmark fields are optional and produced by `bench-kem/update_scheme_data.py` from a
`bench-kem/results/*.txt` run; that script also writes `data/kem_benchmark_env.yaml`
(loaded as `kemBenchmarkEnv`, shown via `BenchmarkEnvInfo` on the page). The table shows
keygen/encaps/decaps µs; when only cycles are present the time is extrapolated (wavy
underline) at `KEM_CPUSPEED` (2.5 GHz).

Flags (`broken`/`warning`/`info`) set at scheme level apply to all parameter sets, overridable
per parameter set. Loaded via `import.meta.glob` in `src/lib/kemSchemeData.ts`; flattened by
`processKemSchemes()` in `src/lib/kemData.ts` (pure, unit-tested). The page is self-contained
and does **not** reuse the signature `filterStore` singleton: `kemData.ts` also exports pure
`computeKemRanges()` / `defaultKemFilter()` / `filterKemRows()`, the page holds filter state in a
`$state` rune and derives `filteredRows`, and `KemFilterPanel` (scheme/level/size/time, `bind:filter`),
`KemTable` (local sort) and `KemScatterPlot` (pk vs ct) all take plain props.

Both compare pages share programme, NIST/NGCC level, Recommended/All and "Hide …" filters
(`CommonFilter`), the table layout (Programmes column, badge detail rows, `TableLegend`) and the
chart (HTML legend, Sizes/Speed toggle). NGCC targets have their own filter group and purple
colour scale and are never mapped onto NIST levels.

### China NGCC

`data/ngcc/candidates.yaml` holds the curated claims for the 34 signature and 41 KEM
round-1 candidates: family, assumption, official NICCS page, spec reference, and per
instance (`ALGORITHM_INSTANCE`) the claimed `ngcc_level`, `recommended`, or `omit: reason`.
It is the source for the generated `data/schemes/<file>.yaml` / `data/kems/<file>.yaml`
(`programmes: {china-ngcc: round-1}`, status `NGCC round 1`). Never edit those by hand:

```bash
uv run bench/update_scheme_data.py --ngcc-sizes bench/results/ngcc_sizes.tsv
uv run bench-kem/update_scheme_data.py --ngcc-sizes bench-kem/results/ngcc_sizes.tsv
```

Sizes come from our builds of the official archives (`./bench --sizes NGCC`, run through
`scripts/ngcc-run`); per-instance notes record the implementation (reference/optimised),
ICCS placeholder-hash use and KAT status from `<suite>/schemes/ngcc/status.tsv`.
`src/lib/__tests__/ngccCandidates.test.ts` validates the claims and their agreement with the
generated files. ngcc.dev data is cross-check only; findings links/flags are added only with
Thom's sign-off. No NGCC timings exist yet (they come from our own AWS runs).

## Architecture

```
data/
├── schemes/          # one .yaml per signature scheme (source of truth)
├── kems/             # one .yaml per KEM (source of truth for /kems/)
├── ngcc/candidates.yaml  # curated NGCC round-1 claims (source for generated NGCC YAML)
└── history.yaml      # chronological event log (attacks, updates, milestones)

src/
├── lib/
│   ├── types.ts          # Scheme, ParameterSet, SchemeYaml, VersionYaml, FilterState, Kem* types
│   ├── constants.ts      # CPUSPEED, NIST_LEVELS
│   ├── data.ts           # processYamlSchemes() — tag-filtered YAML → Scheme[] + ParameterSet[]
│   ├── kemData.ts        # processKemSchemes() — KEM YAML → KemScheme[] + KemParameterSet[] (pure)
│   ├── compareFilters.ts # programme groups, CommonFilter predicate + URL codec (both compare pages)
│   ├── badges.ts         # security badges (incl. implementation findings) + Programmes-column chips
│   ├── chartLegend.ts    # family shapes, NIST + NGCC colour scales, HTML legend groups
│   ├── chartData.ts      # axis values, leaving out unbenchmarked rows (Speed view); timeTooltip() marks estimated times
│   ├── timing.ts         # displayed µs (measured, else extrapolated from cycles)
│   ├── filterStore.ts    # Svelte writable store + derived filteredRows + URL codec
│   ├── roundStore.ts     # writable<'round-1'|'round-2'|'round-3'|'latest'> — drives dataset selection
│   ├── legacy.ts         # legacyHomeRedirect / legacyPathRedirect (old URL forwarding)
│   ├── programmes.ts     # programme data model
│   ├── programmeData.ts  # programme YAML loader
│   ├── programmePage.ts  # programme pages: members, stage views, NGCC targets, findings summary, facts, compare links
│   ├── slugs.ts          # slugify, buildSchemeIndex (fails on slug/name clashes), schemeHref
│   ├── schemeIndex.ts    # the built index over all signatures and KEMs (names ↔ slugs)
│   ├── versionCompare.ts # version comparison: sets matched by name, % changes, flags, perf sources
│   ├── schemeHistory.ts  # a scheme's slice of the history log (via `schemes:` names)
│   ├── historyData.ts    # single runtime loader for data/history.yaml (throws if missing)
│   ├── timeline.ts       # timeline: scheme→programme index, entry programmes, filter, URL codec (p, t)
│   ├── urlSync.ts        # urlSyncTarget(): debounced filter→URL sync skips no-op and in-flight navigations
│   ├── dates.ts          # fmtDate: the single date formatter (YYYY, YYYY-MM, YYYY-MM-DD)
│   ├── schemePage.ts     # scheme page model (pure): header, tables, flag notes, events
│   ├── chartNav.ts       # chart point click → scheme page; attachChartNav (capturing pointerdown tells click from drag)
│   ├── schemeData.ts     # import.meta.glob loader → allSchemeData: SchemeYaml[]
│   ├── kemSchemeData.ts  # import.meta.glob loader → allKemData: KemSchemeYaml[]
│   ├── themeStore.ts     # dark/light/system theme store → localStorage
│   ├── yaml.d.ts         # TypeScript module declaration for *.yaml imports
│   └── components/
│       ├── FilterSidebar.svelte  # single filter column; collapsed on phones, sticky on desktop
│       ├── ProgrammeFilter / LevelFilter / ParamSetFilter / StatusFilter.svelte  # shared filter sections
│       ├── BadgeIcon / ProgrammeChips / TableLegend.svelte  # shared table pieces; badge detail rows always rendered (hidden when closed) so aria-controls resolves
│       ├── ChartLegend.svelte    # HTML legend beside the chart (below on phones)
│       ├── ChartModeToggle.svelte # Sizes / Speed
│       ├── OmittedNote.svelte    # "N parameter sets are not shown: not yet benchmarked" under both scatter plots
│       ├── SegmentedToggle.svelte # generic fieldset of aria-pressed buttons (programme pages)
│       ├── scheme/               # scheme page: SchemeHeader, ParamSetTable, FlagNotes, VersionCompare, SchemeEvents
│       ├── programme/            # ProgrammeView, ProgrammeTimeline, SecurityLevelsTable, FindingsSummary, SignatureCandidates, KemCandidates
│       ├── FilterPanel.svelte    # filter controls (categories, levels, ranges)
│       ├── RangeField.svelte     # reusable number input
│       ├── SchemeTable.svelte    # sortable table (one row per parameter set); scroll wrapper is `relative overflow-x-auto`
│       ├── ScatterPlot.svelte    # Vega-Lite scatter plot; accepts xField/yField/xScale/yScale props
│       ├── RoundSelect.svelte    # "On-ramp round" <select> in the programme filter, writes roundStore
│       ├── LegacyRedirect.svelte # redirect stub body (meta refresh fallback + JS redirect keeping search/hash)
│       ├── KemTable.svelte       # KEM size table (rows prop, local sort)
│       ├── KemScatterPlot.svelte # KEM pk-vs-ct Vega-Lite scatter (rows prop)
│       └── KemFilterPanel.svelte # KEM scheme/level/size/time filters (bind:filter)
└── routes/
    ├── +layout.svelte    # nav (logo, Signatures, KEMs, Programmes, Timeline, Contribute, theme toggle; no round selector), footer
    ├── +page.ts          # load: programme summaries, scheme/KEM counts, 3 latest history entries
    ├── +page.svelte      # home: hero, programme table (names link to /programmes/<id>/), latest changes; forwards legacy /?filters links (legacyHomeRedirect)
    ├── signatures/
    │   ├── +page.ts      # load: processYamlSchemes('round-3', {useLatestVersion:true}), createFilterStore
    │   ├── +page.svelte  # page composition, round switching, URL state sync
    │   └── advanced/
    │       ├── +page.ts      # same load as signatures page
    │       └── +page.svelte  # axis selectors, scale toggles, ScatterPlot, FilterPanel, SchemeTable
    ├── programmes/       # index (+page.ts: programmeCards) and [id]/ (entries() per registry id; {#key} ProgrammeView)
    ├── schemes/[slug]/   # one prerendered page per signature or KEM (entries() from schemeIndex):
    │                     # header, parameter sets, security flags, version comparison (signatures), history slice
    ├── timeline/         # /timeline/: events filtered by programme and type (URL p, t), programme chips
    ├── advanced/         # redirect stub → /signatures/advanced/ (LegacyRedirect)
    ├── history/          # redirect stub → /timeline/ (LegacyRedirect)
    └── kems/
        ├── +page.ts      # load: processKemSchemes(allKemData); trailingSlash 'always'
        ├── +page.svelte  # KEM hero, KemScatterPlot, KemTable
        └── advanced/     # /kems/advanced/: axis selectors, scale toggles, KemScatterPlot, KemFilterPanel, KemTable

src/lib/__tests__/       # Vitest unit tests (vitest.config.ts): badges, chartData, chartLegend,
                         # compareFilters, data, dataFiles, filterStore, html, kemData, legacy,
                         # programmePage, programmeRegistry, programmes, timing, slugs, versionCompare, schemeHistory,
                         # dates, schemePage, chartNav, timeline, urlSync
e2e/                     # Playwright E2E tests (playwright.config.ts)
├── signatures.spec.ts
├── advanced.spec.ts
├── kems.spec.ts
├── home.spec.ts
├── programmes.spec.ts
├── schemes.spec.ts
├── timeline.spec.ts
└── redirects.spec.ts
```

### Data Processing

`processYamlSchemes(allSchemeData, filter?, { useLatestVersion })` in `src/lib/data.ts`, where
`filter` is a `StageFilter` (`{ programme, stage }`, e.g. `{ programme: 'nist-onramp', stage: 'round-2' }`):
- With a filter: picks the latest version whose normalised stage list for `filter.programme`
  `includes(filter.stage)` (a version spanning several stages uses list form, e.g. HAWK v1.1
  `nist-onramp: [round-2, round-3]`).
- Option `includeOtherProgrammes` (`roundToFilter()` sets it true only for `'latest'`): the stage
  only scopes that programme's entrants. A scheme with no memberships, or with a membership in any
  other programme (standards, KpqC, NGCC), is included at its newest version; the page's programme
  filter decides whether it is shown.
- Round 1/2/3 (option false) keep the narrower rule: on-ramp entrants at the stage plus reference
  schemes (`isReferenceScheme()` in `src/lib/programmes.ts`: only `nist-pqc` memberships or none, e.g. ML-DSA).
- Schemes whose memberships include `filter.programme` but never reached the stage are excluded.
- Every `Scheme` / `ParameterSet` (and KEM equivalent) carries `programmes: ProgrammeMembership[]`.

### Filter Store

`src/lib/filterStore.ts` uses stable module-level store references. `_store` and `_filteredRows`
are created once; `createFilterStore()` on subsequent calls (round changes) updates `_allRows`
and resets `_store` in place. This means components calling `getFilterStore()` at init always
hold valid references — no stale subscription bugs.

Category checkbox state is **derived** from the scheme set, not stored separately.

Cycle filters: rows with no timing (0 cycles, shown as "—") pass the signature cycle min/max
filters, mirroring KEM `outside()` in `kemData.ts`; the default minimums are computed over
benchmarked rows only. Without this, unbenchmarked sets would vanish by default.

URL state: filter params encoded as query params, applied client-side in `onMount`. The URL is
the source of truth on load: the round is applied from it first (no `r` = latest), defaults are
read at sync time, and the hash is kept.
Round encoded as `?r=1|2|3`. Common keys (both compare pages,
`encodeCommonFilters`/`decodeCommonFilters` in `compareFilters.ts`): `p` programme groups,
`l` NIST levels (`PQ` = pre-quantum), `nl` NGCC targets (`l` without `nl` = old link, no NGCC
targets), `ps=all`, `hb`/`hi`/`hw` hide broken / implementation findings / warnings, `v=speed`.
Every signature key must be in `SIGNATURE_PARAMS` (`src/lib/legacy.ts`). KEM-only:
`pkMin…pcMax` sizes and `kuMin kuMax euMin euMax duMin duMax` µs.
Changing the on-ramp round keeps the common filters and the sort.

### Round Selector

`RoundSelect.svelte` ("On-ramp round", visually hidden label) sits under "NIST on-ramp" in the
programme filter on `/signatures/` and is disabled while that is unticked (not in the nav). Choosing
a value updates `roundStore`, which triggers `applyRound()` in `signatures/+page.svelte`, which
calls `createFilterStore()` with new data.

### Latest View

"Latest" (default) and "Round 3" both use `roundToFilter()` -> `StageFilter { programme: 'nist-onramp', stage: 'round-3' }` with `useLatestVersion=true`.
Inclusion: scheme must have at least one version with `nist-onramp: round-3`, or (Latest only,
`includeOtherProgrammes`) belong to no or another programme. Data shown: `sorted[0]` (newest version by date, regardless of programmes).
`roundToFilter()` in `src/lib/roundStore.ts` maps a round to its filter.

Round 2 view uses the `nist-onramp` / `round-2` filter without `useLatestVersion` — shows the pinned
round-2 submission version.

Post-round-3 spec updates are new versions **without** a `nist-onramp` entry. Round views show the
pinned submission, and Latest shows the newest version.

### Programmes

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

### Scheme pages

`/schemes/<slug>/` serves signatures and KEMs from one route; `schemeIndex` (built by
`buildSchemeIndex()` in `src/lib/slugs.ts`) maps names to slugs and fails the build on a slug or
name clash across both primitives (fix with `slug:` in the YAML). Table scheme names and chart
points link there (`schemeHref(base, slugOf(name))`; chart clicks go through `attachChartNav`, which
reads the press position from a capturing `pointerdown` because Vega suppresses the mouse events).
The header's programme chips link to `/programmes/<id>/`: `Chip.programme` carries the id and
`ProgrammeChips` links it when given `link`. The version comparison matches parameter sets
by name; flags shown per version are today's assessment of that version (see the history slice for
when attacks were published). Cycle comparisons always state each version's `perf_source`: machines
differ between versions. History entries reach a scheme page through their `schemes:` names. All
dates are shown with `fmtDate` from `src/lib/dates.ts`. The standard chip (FIPS / Std pending)
links to `/programmes/nist-pqc/` like the others; the header omits an assumption equal to the
category, and KEM pages show their status once (no Parameter-sets subtitle).

### Page conventions

- Titles: `<page> · {SITE_NAME}` (" · " everywhere); `app.html`'s static `og:title` equals `SITE_NAME`.
- Every `target="_blank"` link carries `<span class="sr-only"> (opens in a new tab)</span>`.
- Debounced filter→URL syncs (`replaceState` `goto`) go through `urlSyncTarget()`, which skips a
  no-op URL and any sync while `navigating.to` is set; otherwise the mount-time sync cancels a
  click on a scheme link.

### Parameter-set labels

Both compare pages' Recommended/All radio reads "Recommended (where designers name them)"; it
shows only the sets flagged `recommended: true` in versions/KEMs that flag any. Table footers just
count parameter sets. Pre-quantum rows show level "Pre-quantum" (not "N/A"); the YAML/URL value
stays `Pre-Quantum`.

### Performance Data

Cycles extrapolated from ms values use `CPUSPEED = 2_500_000_000` (2.5 GHz).
Extrapolated values shown with wavy red underline (`decoration-wavy decoration-red-500`).
Never import third-party benchmark numbers; timings come from `bench/` and `bench-kem/` only.

### Cloud benchmark runs — `scripts/aws-bench.sh`

Runs `bench/` and/or `bench-kem/` on a throwaway EC2 spot instance and copies the
`results/*.txt` files back into the working copy. Import them afterwards with the
suite's `update_scheme_data.py`, after reviewing them.

```bash
aws sso login --profile bench
AWS_PROFILE=bench scripts/aws-bench.sh                          # both suites
AWS_PROFILE=bench scripts/aws-bench.sh --suite kems mlkem       # one suite, filtered
BENCH_ITER=5 AWS_PROFILE=bench scripts/aws-bench.sh --suite sigs  # quick smoke test
```

Options: `--suite sigs|kems|both`, `--instance-type` (default `c7i.4xlarge`),
`--ubuntu` (default `26.04`), `--ttl` minutes (default 60). Positional args are
`run_bench.sh` filters. `BENCH_ITER` / `BENCH_CYCLES` / `BENCH_CPU` are forwarded.
`--ngcc-src TARBALL` uploads a tar.gz of `bench-common/ngcc/src` (NGCC candidates) and
sets `NGCC_BUILD=1` on the VM; without it NGCC instances are skipped.

- **Builds the local `HEAD`**, cloned from the public GitHub repo. The script refuses
  to run with uncommitted changes. Commits not yet on origin are sent as a git bundle.
  Only top-level submodules are initialised, over anonymous HTTPS.
- **Lifetime:** the instance self-terminates at the TTL (`shutdown -h` in user-data +
  shutdown-behaviour `terminate`) even if the script dies. On exit, including Ctrl-C,
  the script fetches partial results, terminates the instance and deletes the
  per-run security group and key pair. All resources are tagged
  `Name=nist-sigs-zoo-bench-*`.
- **Credentials:** IAM Identity Center profile with the permission set from
  `scripts/aws-bench-iam-policy.json` (create/destroy limited to that tag). Region
  comes from the profile or `AWS_REGION`.
- **Cycle counter:** `c7i.4xlarge` exposes a virtual PMU, so results use `rdpmc`.
  The script warns if a results file fell back to `rdtsc`. Use a `*.metal*` type
  to avoid noise from other tenants on the host.
- **Smoke-test changes cheaply first** (`BENCH_ITER=5`, a filter). Those results
  files are junk: don't commit or import them.
- **New build dependency?** Add it to the user-data `apt-get` list in the script.
  The VM is a stock Ubuntu with GCC 15 (C23 by default) and multiarch OpenSSL. If a
  scheme breaks there, fix its Makefile, not the script (see `bench/schemes/sdith`).
- **NGCC runs** (Thom's plan: `c7i.4xlarge`, `--ttl 360`, signatures and KEMs as separate
  runs, no filter, **`BENCH_ITER` unset** because it overrides the tuned per-instance
  `iters` in `<suite>/schemes/ngcc/params.tsv`):
  ```bash
  tar -C bench-common/ngcc -czf /tmp/ngcc-src.tar.gz src   # ~2.4 GB (src/ is ~5 GB)
  AWS_PROFILE=bench scripts/aws-bench.sh --suite sigs --ttl 360 --ngcc-src /tmp/ngcc-src.tar.gz
  AWS_PROFILE=bench scripts/aws-bench.sh --suite kems --ttl 360 --ngcc-src /tmp/ngcc-src.tar.gz
  ```
  The VM never downloads from niccs.org.cn; the tarball is unpacked into
  `repo/bench-common/ngcc/` (disk is raised to 100 GB with `--ngcc-src`). `iters` caps
  each NGCC instance at about 30 s of timed work (floor 10, or 5 above 60 s per
  iteration). Estimated runtime on a laptop-class core: signatures ~225 min, KEMs ~130 min
  for the NGCC instances alone, plus build time and the non-NGCC schemes. QCTM512 and
  vistrutith-d3-512s take 10 min per iteration; the TTL fetches partial results.
  Smoke runs of NGCC code go through `scripts/ngcc-run` only (never the host).
- **`scripts/ngcc-run` tooling notes:** `NGCC_OUT=$PWD/.ngcc-out-<name>` lets several
  containers run concurrently (default `.ngcc-out/`); do not share an output dir. The
  host has 4 cores and ~4 GB: run `status.py` per candidate group (a single full run is
  OOM-killed; per-group `NGCC_KAT_TIMEOUT`s are recorded in `params.tsv`); its default
  timeout is 900 s. A full `make -C bench` in the container fails offline (faest,
  mceliece fetch git subprojects): use `make ALL_SOS= STAMPS= bench`.
- **Orphan check:** `aws ec2 describe-instances --filters 'Name=tag:Name,Values=nist-sigs-zoo-bench-*'`
  (also `describe-security-groups`, `describe-key-pairs`).

## Deploy

GitHub Actions workflow at `.github/workflows/deploy.yml`:
1. `npm ci && npm run build` → `dist/`
2. Copy `.nojekyll` into `dist/`
3. Deploy `dist/` to GitHub Pages

### PR previews

`.github/workflows/preview.yml` builds each same-repo PR (no `BASE_PATH`) and uploads
`dist/` to the Cloudflare Pages Direct Upload project `nist-sigs-zoo` with
`npx wrangler pages deploy --branch=pr-N` (the repo only allows GitHub-owned actions, so no
third-party actions). A sticky PR comment (`actions/github-script`) links
`https://pr-N.nist-sigs-zoo.pages.dev` and the per-commit URL. Needs repo secrets
`CLOUDFLARE_API_TOKEN` (Account → Cloudflare Pages → Edit) and `CLOUDFLARE_ACCOUNT_ID`.
Fork PRs are skipped (no secrets). Production stays on GitHub Pages; `main` is never
deployed to Cloudflare.

## round-1/

Contains a standalone snapshot of round-1 data. **Do not modify.** Served at `/round-1/`.
