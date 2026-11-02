#!/usr/bin/env python3
"""Verify the packaged source manifest. Build products do not affect this check."""
from pathlib import Path, PurePosixPath
import hashlib
import json

ROOT = Path(__file__).resolve().parent


def main():
    manifest = json.loads((ROOT / "SOURCE-SHA256.json").read_text(encoding="utf-8"))
    failures = []
    for name, expected in manifest["files"].items():
        relative = PurePosixPath(name)
        if relative.is_absolute() or ".." in relative.parts:
            failures.append({"path": name, "error": "invalid manifest path"})
            continue
        path = ROOT.joinpath(*relative.parts)
        if not path.resolve().is_relative_to(ROOT) or not path.is_file():
            failures.append({"path": name, "error": "missing source"})
        elif hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            failures.append({"path": name, "error": "source changed"})
    print(json.dumps({"passed": not failures, "checked": len(manifest["files"]), "failures": failures}, ensure_ascii=False))
    return bool(failures)


if __name__ == "__main__":
    raise SystemExit(main())
