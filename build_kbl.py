#!/usr/bin/env python3
"""Standalone KBL build entry. Creates local artifacts; never accesses a device."""
from pathlib import Path
import argparse
import json
import os
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parent


def llvm_directory(explicit):
    configured = explicit or os.environ.get("LLVM_BIN")
    if configured:
        directory = Path(configured).expanduser().resolve()
    else:
        found = shutil.which("clang") or shutil.which("clang.exe")
        if not found:
            raise SystemExit("Set LLVM_BIN or pass --llvm-bin with Clang/LLD's bin directory")
        directory = Path(found).resolve().parent
    for name in ("clang", "ld.lld"):
        if not any((directory / (name + suffix)).is_file() for suffix in ("", ".exe")):
            raise SystemExit(f"Missing {name} in LLVM directory: {directory}")
    return directory


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    choices = ("driver", "parent", "recovery", "sdk-demo", "sdk-port", "efi-apps", "bundle", "all")
    parser.add_argument("targets", nargs="*", choices=choices, default=["driver"])
    parser.add_argument("--llvm-bin", type=Path, help="Clang/LLD bin directory; otherwise LLVM_BIN or PATH")
    parser.add_argument("--output", type=Path, default=ROOT / "build")
    args = parser.parse_args()
    llvm = llvm_directory(args.llvm_bin)
    output = args.output.expanduser().resolve()
    targets = list(dict.fromkeys(args.targets or ["driver"]))
    if "all" in targets:
        targets = ["driver", "parent", "recovery", "sdk-demo", "sdk-port", "efi-apps", "bundle"]
    # Bundling always uses a driver freshly built by this invocation.
    if "bundle" in targets and "driver" not in targets:
        targets.insert(0, "driver")
    elif "bundle" in targets:
        targets.remove("driver")
        targets.insert(0, "driver")
    output.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, LLVM_BIN=str(llvm))
    commands = []
    for target in targets:
        if target == "driver":
            script, more = "runtime/uefi/build_driver.py", ["--output", output / "kbl-driver"]
        elif target == "parent":
            script, more = "runtime/uefi/build_parent_shim.py", ["--output", output / "kbl-parent"]
        elif target == "recovery":
            script, more = "runtime/uefi/build_boot_recovery.py", ["--output", output / "boot-recovery"]
        elif target == "sdk-demo":
            script, more = "runtime/sdk/build_demo.py", ["--output", output / "kbl-sdk"]
        elif target == "sdk-port":
            script, more = "runtime/sdk/build_port.py", ["--source", ROOT / "runtime/examples/port_text.c", "--output", output / "kbl-port"]
        elif target == "efi-apps":
            script, more = "runtime/uefi/build_apps.py", ["--output", output / "kbl-efi"]
        else:
            script, more = "runtime/uefi/bundle_driver.py", ["--driver", output / "kbl-driver/kbl_driver.efi", "--output-dir", output / "kbl-resources"]
        command = [sys.executable, str(ROOT / script)]
        if target != "bundle":
            command += ["--llvm-bin", str(llvm)]
        command += list(map(str, more))
        print("BUILD " + target, flush=True)
        subprocess.run(command, check=True, cwd=ROOT, env=env)
        commands.append(command)
    report = {"passed": True, "targets": targets, "llvmBin": str(llvm), "commands": commands, "deviceOperations": False}
    (output / "standalone-build.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
