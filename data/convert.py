import csv

from pathlib import Path
from pprint import pprint


class NumbersDialect(csv.Dialect):
    delimiter: str = ";"
    quoting = csv.QUOTE_NONE
    lineterminator = "\n"


with Path("numbers.csv").open("r") as fh:
    reader = csv.DictReader(fh, dialect=NumbersDialect)
    fields = reader.fieldnames
    assert fields is not None
    print(fields)
    with Path("parametersets.csv").open("w") as fh:
        writer = csv.DictWriter(fh, fieldnames=fields)
        writer.writeheader()
        for row in reader:
            for field in fields[3:-2]:
                if row[field]:
                    row[field] = int(row[field].replace(",", ""))
            for field in fields[-2:]:
                if row[field]:
                    row[field] = float(row[field].replace(",", "."))
            writer.writerow(row)
