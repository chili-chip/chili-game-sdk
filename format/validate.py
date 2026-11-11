"""Check .rpg game files against format/schema.json.

Usage: python format/validate.py FILE...

The schema checks the shape of the file. The SDK's loader also checks that
every id a file names exists; the tests load the demo game to cover that.
"""

import json
import sys
from pathlib import Path

from jsonschema import Draft202012Validator

SCHEMA = json.loads((Path(__file__).parent / "schema.json").read_text())


def main(paths: list[str]) -> int:
    validator = Draft202012Validator(SCHEMA)
    failed = 0
    for path in paths:
        game = json.loads(Path(path).read_text())
        errors = sorted(validator.iter_errors(game), key=lambda e: list(e.path))
        for error in errors:
            where = "/".join(str(p) for p in error.path) or "(top)"
            print(f"{path}: {where}: {error.message}")
        failed += bool(errors)
        if not errors:
            print(f"{path}: ok")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
