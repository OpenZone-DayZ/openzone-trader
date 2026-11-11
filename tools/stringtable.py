# Expands <mod>/strings.csv (key, Ukrainian, English) into the game's
# <mod>/stringtable.csv: fifteen columns, every field quoted, a comma after the
# last one, a BOM in front -- the shape the rest of the series ships. Ukrainian
# is the original and stands in every column but the English one.
#
# Usage: python -I tools/stringtable.py <path to the mod's folder>
import csv
import io
import os
import sys

COLUMNS = ["Language", "original", "english", "czech", "german", "russian", "polish",
           "hungarian", "italian", "spanish", "french", "chinese", "japanese",
           "portuguese", "chinesesimp"]

mod = sys.argv[1]
rows = []
with io.open(os.path.join(mod, "strings.csv"), encoding="utf-8-sig", newline="") as f:
    for line in csv.reader(f):
        if not line or line[0].startswith("#"):
            continue
        if len(line) != 3:
            raise SystemExit("strings.csv: three fields a line, got %r" % (line,))
        key, ukrainian, english = line
        row = [key, ukrainian, english] + [ukrainian] * (len(COLUMNS) - 3)
        rows.append(row)

keys = [r[0] for r in rows]
twice = sorted(set(k for k in keys if keys.count(k) > 1))
if twice:
    raise SystemExit("strings.csv: listed twice: %s" % ", ".join(twice))


def quoted(fields):
    return ",".join('"' + x.replace('"', '""') + '"' for x in fields) + ","


text = "\n".join([quoted(COLUMNS)] + [quoted(r) for r in rows]) + "\n"
with io.open(os.path.join(mod, "stringtable.csv"), "w", encoding="utf-8-sig", newline="\n") as f:
    f.write(text)
print("stringtable.csv: %d strings" % len(rows))
