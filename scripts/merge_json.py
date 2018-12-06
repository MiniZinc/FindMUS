import sys
import json

all_parts = []

with open(sys.argv[1]) as f:
    raw_lines = []
    for line in f.readlines():
        if line.startswith("%%%mzn-json-start"):
            raw_lines = []
        elif line.startswith("%%%mzn-json-end"):
            raw_str = "".join(raw_lines)
            obj = json.loads(raw_str)
            all_parts.append(obj)
        elif line.startswith("%%%mzn-progress"):
            continue
        elif line.startswith("Total"):
            continue
        elif line.startswith("Intermediate"):
            continue
        else:
            raw_lines.append(line)

print all_parts
