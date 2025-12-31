"""Package fusionpy_host artifacts.

This script is intended to be run on Windows build agents. It signs binaries
when a SIGNTOOL path is available and emits architecture-specific ZIP/NuGet
feeds under releases/.
"""
import argparse
import hashlib
import os
import shutil
import subprocess
import sys
from pathlib import Path
import zipfile

ROOT = Path(__file__).resolve().parents[1]
RELEASES = ROOT / "releases"


def hash_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(8192), b""):
            h.update(chunk)
    return h.hexdigest()


def sign_binary(binary: Path, signtool: Path | None):
    if signtool and signtool.exists():
        subprocess.check_call([str(signtool), "sign", "/fd", "sha256", "/a", str(binary)])


def package_architecture(arch: str, version: str, signtool: Path | None):
    bin_dir = ROOT / "fusionpy_host" / "build" / arch / "Release" / "bin"
    host_dll = bin_dir / "fusionpy_host.dll"
    injector = bin_dir / "fusionpy_injector.exe"
    if not host_dll.exists() or not injector.exists():
        raise SystemExit(f"Missing host outputs in {bin_dir}")

    sign_binary(host_dll, signtool)
    sign_binary(injector, signtool)

    arch_release = RELEASES / arch
    arch_release.mkdir(parents=True, exist_ok=True)
    package_name = arch_release / f"fusionpy_host-{version}-{arch}.zip"
    with zipfile.ZipFile(package_name, "w", zipfile.ZIP_DEFLATED) as zf:
        for artifact in (host_dll, injector):
            zf.write(artifact, artifact.name)
    manifest = arch_release / f"fusionpy_host-{version}-{arch}.sha256"
    manifest.write_text(f"{hash_file(host_dll)} {host_dll.name}\n{hash_file(injector)} {injector.name}\n")
    print(f"Packaged {package_name}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--version", required=True)
    parser.add_argument("--signtool", type=Path)
    args = parser.parse_args()

    signtool = args.signtool if args.signtool else None
    for arch in ("Win32", "x64"):
        package_architecture(arch, args.version, signtool)


if __name__ == "__main__":
    sys.exit(main())
