"""Compile real adapter/controller/backend/USB code and run host-only model assertions."""
from pathlib import Path
import argparse, ctypes, hashlib, json, os, shutil, subprocess

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--cc',default=shutil.which('clang'))
    args=parser.parse_args()
    if not args.cc:raise SystemExit('Host clang is required')
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    sources=[ROOT/'driver_adapter/adapter.c',ROOT/'driver_adapter/board_template.c',HERE/'smoke.c',
        ROOT/'runtime/kbl_control.c',ROOT/'runtime/kbl_elf.c',ROOT/'runtime/kbl_sha256.c',
        ROOT/'runtime/native/kbl_native.c',ROOT/'runtime/native/kbl_efi_bootloader.c',
        ROOT/'runtime/native/tests/fs_unavailable.c',ROOT/'runtime/uefi/kbl_usb.c',ROOT/'runtime/uefi/kbl_usb_native.c']
    target=out/('adapter-smoke.dll' if os.name=='nt' else 'adapter-smoke.so')
    command=[args.cc,'-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function','-fno-builtin','-shared',*map(str,sources),'-o',str(target)]
    if os.name!='nt':command.insert(1,'-fPIC')
    result=subprocess.run(command,capture_output=True,text=True)
    (out/'compile.log').write_text(result.stdout+result.stderr,encoding='utf8')
    if result.returncode:raise RuntimeError(result.stderr)
    dll=ctypes.CDLL(str(target));run=dll.kbl_adapter_smoke;run.argtypes=[ctypes.POINTER(ctypes.c_uint)];run.restype=ctypes.c_int
    checks=ctypes.c_uint();failed=run(ctypes.byref(checks))
    report=dict(passed=failed==0,checks=checks.value,failedLine=failed,hostOnly=True,deviceOperations=False,
        command=command,artifact=str(target),sha256=hashlib.sha256(target.read_bytes()).hexdigest(),
        sources={p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in sources},
        filesystemModel='existing runtime/native/tests/fs_unavailable.c; smoke never claims a filesystem or native ARM start')
    (out/'smoke-result.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf8')
    print(json.dumps({k:report[k] for k in ('passed','checks','failedLine','artifact')}))
    return 0 if report['passed'] else 1

if __name__=='__main__':raise SystemExit(main())
