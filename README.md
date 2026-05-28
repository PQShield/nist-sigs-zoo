# NIST PQC Signatures Zoo

Interactive comparison of post-quantum signature schemes submitted to the [NIST Additional Signatures on-ramp](https://csrc.nist.gov/projects/pqc-dig-sig/).

Data reflects scheme specifications as per the last version that we could find. Schemes may have been updated since; consult the individual scheme websites for current specifications. If you do find more up-to-date information, please let us know by creating an issue!

Parameter sizes and performance timings (up to and including round 2) are copied from the individual scheme submission documents — be aware of potential errors. From round 3 on we do our own benchmarking.

## Development

```sh
npm install
npm run dev       # dev server at http://localhost:5173
npm run build     # production build → dist/
npm run preview   # preview dist/ at http://localhost:4173
npm run check     # type-check
```

## Testing

```sh
npm run test        # unit tests (Vitest) — fast, no browser required
npm run test:e2e    # E2E tests (Playwright) — builds site then runs in headless Chromium
```

Unit tests in `src/lib/__tests__/` cover data processing and URL-encoding logic.
E2E tests in `e2e/` exercise the main page and advanced graph page in a real browser.

## Contributing

Suggestions and improvements welcome — open an issue or pull request.

## Stack

SvelteKit · Svelte 5 · TypeScript · Tailwind CSS v4 · Observable Plot

## License

Data: [CC BY-SA 4.0](LICENSE.md)
