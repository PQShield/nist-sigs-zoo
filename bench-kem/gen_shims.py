#!/usr/bin/env python3
"""Generate _shim.c files from a C template and a TSV params file.

Usage: gen_shims.py <template.c> <params.tsv> <outdir>

TSV format:
  - Lines starting with # are comments and are skipped.
  - First non-comment line is the header row with column names.
  - First column must be named 'file'; its value becomes <value>_shim.c.
  - In the template, @COLNAME@ (uppercase of the column name) is substituted.
"""
import sys
import os
import csv


def main():
    if len(sys.argv) != 4:
        sys.exit(f"Usage: {sys.argv[0]} template.c params.tsv outdir")
    template_path, params_path, out_dir = sys.argv[1:4]

    with open(template_path) as f:
        template = f.read()

    os.makedirs(out_dir, exist_ok=True)

    with open(params_path, newline='') as f:
        non_comment = [ln for ln in f if not ln.startswith('#')]
    reader = csv.reader(non_comment, delimiter='\t')
    headers = [h.strip() for h in next(reader)]

    for row in reader:
        row = [v.strip() for v in row]
        if not any(row):
            continue
        params = dict(zip(headers, row))
        content = template
        for key, val in params.items():
            content = content.replace(f'@{key.upper()}@', val)
        out_path = os.path.join(out_dir, params['file'] + '_shim.c')
        with open(out_path, 'w') as f:
            f.write(content)
        print(f"  {out_path}")


if __name__ == '__main__':
    main()
