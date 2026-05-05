# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

Static website providing an interactive comparison of post-quantum signature schemes submitted to the NIST on-ramp. No build system — open `index.html` directly in a browser or serve with any static HTTP server (e.g. `python3 -m http.server`).

## Data

All scheme data lives in two CSV files under `data/`:

- `data/schemes.csv` — one row per scheme: name, NIST status, website, category, security flags (`Broken`, `Warning`, `Info`), cryptographic assumption
- `data/parametersets.csv` — one row per parameter set: scheme name, parameterset name, security level, pk/sig sizes (bytes), performance (cycles or ms)

**Source of truth is Google Sheets** (linked in README). Edits to CSVs should come from there. The `data/convert.py` script converts a Numbers-exported semicolon-delimited CSV (`numbers.csv`) into the standard `parametersets.csv` — run it from the `data/` directory.

Security flags in `schemes.csv`:
- `Broken` — scheme is broken or classically insecure (value is a description or `"classical"`)
- `Warning` — known attack weakens the scheme
- `Info` — informational note about a security concern

## Architecture

`js/app.js` is the entire application. It runs as a top-level `await` ES module (loaded via `<script type="module">` in `index.html`). On load it:

1. Fetches and parses both CSVs via D3
2. Renders three filterable/sortable tables: schemes overview, parameter set sizes, performance
3. Renders a log-log scatter plot of pk size vs. sig size via Observable Plot

All filtering and sorting state is kept in module-level variables; `updateTable()` re-renders everything from current filter state.

Vendored libraries in `js/vendor/` and `static/`: D3 v7, Observable Plot v0.6.9, Foundation CSS framework, jQuery. Do not upgrade these without testing the full UI.

`wide.html` + `js/wide.js` is a standalone wide-layout variant with the same data.

`round-1/` contains a snapshot of the round-1 data and pages (separate from the current on-ramp data).
