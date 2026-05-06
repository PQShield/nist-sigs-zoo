# CLAUDE.md

This file provides guidance to Claude Code when working with code in this repository.

## Overview

Static website comparing post-quantum signature schemes submitted to the NIST on-ramp.
Built with SvelteKit (adapter-static) + Tailwind CSS v4 + TypeScript.
Deploy: `npm run build` → `dist/`. GitHub Actions deploys to GitHub Pages.

## Stack

- **Framework**: SvelteKit 2 + Svelte 5 (runes mode), adapter-static → `dist/`
- **CSS**: Tailwind CSS v4 via `@tailwindcss/vite`
- **Charts**: `@observablehq/plot` (log-log scatter of pk vs sig size)
- **Language**: TypeScript throughout

## Development

```bash
npm run dev       # dev server at http://localhost:5173
npm run build     # build to dist/
npm run preview   # preview dist/ at http://localhost:4173
npm run check     # type-check with svelte-check
```

## Data

Source of truth is Google Sheets (linked in README). CSV files live in two places:

- `data/schemes.csv` and `data/parametersets.csv` — **edit here**
- `static/data/schemes.csv` and `static/data/parametersets.csv` — **copy from data/ after editing**

The `data/convert.py` script converts a Numbers-exported semicolon-delimited CSV (`numbers.csv`)
into the standard `parametersets.csv` — run it from the `data/` directory.

Security flags in `schemes.csv`:
- `Broken` — scheme is broken or classically insecure (value is a description or `"classical"`)
- `Warning` — known attack weakens the scheme
- `Info` — informational note about a security concern

## Architecture

```
src/
├── lib/
│   ├── types.ts          # Scheme, ParameterSet, FilterState types
│   ├── constants.ts      # CPUSPEED, NIST_LEVELS
│   ├── data.ts           # CSV parsing (parseSchemes, parseParameterSets)
│   ├── filterStore.ts    # Svelte writable store + derived filteredRows + URL codec
│   ├── themeStore.ts     # dark/light/system theme store → localStorage
│   ├── plotHelpers.ts    # dotColor, dotSymbol, dotTitle for Observable Plot
│   └── components/
│       ├── SecurityBadge.svelte  # 💣🧨⚠️ℹ️ badges with aria-labels
│       ├── FilterPanel.svelte    # all filter controls (categories, levels, ranges)
│       ├── RangeField.svelte     # reusable number input
│       ├── SchemeTable.svelte    # unified sortable table (one row per parameter set)
│       └── ScatterPlot.svelte    # Observable Plot wrapper
└── routes/
    ├── +layout.svelte    # nav bar (logo, dark toggle), footer
    ├── +page.ts          # load: fetch CSVs, parse, initialize filter store
    └── +page.svelte      # page composition, URL state sync in onMount
```

### Filter Store

`src/lib/filterStore.ts` is the core state module. A singleton `writable<FilterState>` is
initialized by `createFilterStore()` (called from `+page.ts` load function). Components call
`getFilterStore()` to get the store and `filteredRows` derived store.

Category checkbox "checked/indeterminate/unchecked" state is **derived** from the scheme set —
not stored separately. This eliminates the sync-bug class from the old codebase.

URL state: filter params are encoded as query params (see URL codec in `filterStore.ts`).
Applied client-side in `onMount` (prerender cannot access `url.searchParams`).

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
