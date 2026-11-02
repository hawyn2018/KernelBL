#!/usr/bin/env python3
"""Build reproducible source and host-wrapper ZIPs from the verified inventory."""
from pathlib import Path, PurePosixPath
import argparse
import hashlib
import json
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
STAMP = (2026, 11, 3, 0, 0, 0)


def archive(path, entries):
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as out:
        for name, content in sorted(entries.items()):
            info = zipfile.ZipInfo(name, STAMP)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            out.writestr(info, content, compresslevel=9)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "dist")
    args = parser.parse_args()
    subprocess.run([sys.executable, str(ROOT / "verify_source.py")], check=True)
    version = (ROOT / "VERSION").read_text(encoding="utf-8").strip()
    manifest = json.loads((ROOT / "SOURCE-SHA256.json").read_text(encoding="utf-8"))
    names = sorted(set(manifest["files"]) | {"SOURCE-SHA256.json"})
    source = {}
    for name in names:
        relative = PurePosixPath(name)
        path = ROOT.joinpath(*relative.parts).resolve()
        if relative.is_absolute() or ".." in relative.parts or not path.is_relative_to(ROOT):
            raise SystemExit("Invalid manifest path")
        source[f"KernelBL-{version}/{name}"] = path.read_bytes()
    console = ROOT / "tools/kbl_console"
    wrapper = {
        "fastboot.cmd": (console / "fastboot.cmd").read_bytes(),
        "kbl_console.py": (console / "kbl_console.py").read_bytes(),
        "README.md": (console / "QUICKSTART.md").read_bytes(),
        "KBL_PROTOCOL.md": (console / "KBL_PROTOCOL.md").read_bytes(),
        "LICENSE": (ROOT / "LICENSE").read_bytes(),
        "VERSION": (ROOT / "VERSION").read_bytes(),
    }
    wrapper["SHA256SUMS.txt"] = "".join(
        f"{hashlib.sha256(content).hexdigest()}  {name}\n"
        for name, content in sorted(wrapper.items())
    ).encode("utf-8")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    packages = [(f"KernelBL-v{version}-source.zip", source),
                ("run_kbl_fastboot_shell.zip", wrapper)]
    records = []
    for name, entries in packages:
        target = output / name
        archive(target, entries)
        records.append({"name": name, "bytes": target.stat().st_size,
                        "sha256": hashlib.sha256(target.read_bytes()).hexdigest(),
                        "files": len(entries)})
    (output / "SHA256SUMS.txt").write_text("".join(
        f"{record['sha256']}  {record['name']}\n" for record in records
    ), encoding="utf-8", newline="\n")
    print(json.dumps(records, indent=2))


if __name__ == "__main__":
    main()
