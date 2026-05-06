# NIST PQC Signatures Zoo

Interactive comparison of post-quantum signature schemes submitted to the [NIST Additional Signatures on-ramp](https://csrc.nist.gov/projects/pqc-dig-sig/round-2-additional-signatures).

Data reflects scheme specifications as submitted at the start of Round 2. Schemes may have been updated since; consult the individual scheme websites for current specifications.

Parameter sizes and performance timings are copied from the individual scheme submission documents — be aware of potential errors.

## Data corrections

Data is maintained in [Google Sheets](https://docs.google.com/spreadsheets/d/1Ba8MWRJzcn3DaoAQsAiCnIdRbbVRow1mWf01LzXk-14/edit?usp=sharing).

To submit corrections: comment on the spreadsheet, or open an issue in this repository.

## Development

```sh
npm install
npm run dev       # dev server at http://localhost:5173
npm run build     # production build → dist/
npm run preview   # preview dist/ at http://localhost:4173
npm run check     # type-check
```

Updating data: edit `data/schemes.csv` or `data/parametersets.csv`, then copy to `static/data/`.

## Contributing

Suggestions and improvements welcome — open an issue or pull request.

## Stack

SvelteKit · Svelte 5 · TypeScript · Tailwind CSS v4 · Observable Plot

## License

Data: [CC BY-SA 4.0](LICENSE.md)
