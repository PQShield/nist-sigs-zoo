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
    tags: [round-2]            # round-1, round-2, or omit for reference/standardized schemes
                               # post-round-2 updates should have NO tags
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
        signing_ms: null       # use cycles OR ms, not both
        verification_ms: null
        notes: null
```

Security flags (`broken`/`warning`/`info`) can be set at version level (applies to all
parametersets) or overridden per-parameterset.

The YAML files are bundled at build time via a Vite plugin (`vite.config.ts`) and
`import.meta.glob` in `src/lib/schemeData.ts`.

## Architecture

```
data/
└── schemes/          # one .yaml per scheme (source of truth)

src/
├── lib/
│   ├── types.ts          # Scheme, ParameterSet, SchemeYaml, VersionYaml, FilterState types
│   ├── constants.ts      # CPUSPEED, NIST_LEVELS
│   ├── data.ts           # processYamlSchemes() — tag-filtered YAML → Scheme[] + ParameterSet[]
│   ├── filterStore.ts    # Svelte writable store + derived filteredRows + URL codec
│   ├── roundStore.ts     # writable<'round-1'|'round-2'|'latest'> — drives dataset selection
│   ├── schemeData.ts     # import.meta.glob loader → allSchemeData: SchemeYaml[]
│   ├── themeStore.ts     # dark/light/system theme store → localStorage
│   ├── yaml.d.ts         # TypeScript module declaration for *.yaml imports
│   └── components/
│       ├── SecurityBadge.svelte  # broken/warning/info badges with aria-labels
│       ├── FilterPanel.svelte    # filter controls (categories, levels, ranges)
│       ├── RangeField.svelte     # reusable number input
│       ├── SchemeTable.svelte    # sortable table (one row per parameter set)
│       └── ScatterPlot.svelte    # Vega-Lite scatter plot
└── routes/
    ├── +layout.svelte    # nav (logo, round selector, dark toggle), footer
    ├── +page.ts          # load: processYamlSchemes('round-2', {useLatestVersion:true}), createFilterStore
    └── +page.svelte      # page composition, round switching, URL state sync
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

"Latest" is the default view. It shows the newest version of each scheme that participated in
round 2 (or is an untagged reference scheme). Inclusion is determined by whether any version has
a `round-2` tag; the data shown comes from `sorted[0]` (newest by date, regardless of tags).

Post-round-2 spec updates should be added as new version entries **without** any tags. This way:
- Round 2 view shows the pinned round-2 submission data.
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
