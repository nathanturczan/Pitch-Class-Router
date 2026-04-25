#!/usr/bin/env python3
import json
import re
import sys


def strip_trailing_commas(text: str) -> str:
    return re.sub(r",(\s*[}\]])", r"\1", text)


def main(path: str) -> int:
    with open(path, "r", encoding="utf-8") as f:
        original = f.read()
    cleaned = strip_trailing_commas(original)
    parsed = json.loads(cleaned)
    with open(path, "w", encoding="utf-8") as f:
        json.dump(parsed, f, indent=2)
        f.write("\n")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
