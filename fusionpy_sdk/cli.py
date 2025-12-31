from __future__ import annotations

import argparse
import json
import os
import subprocess
from pathlib import Path
import sys

from . import __version__
from .builder import build_extension
from .manifest import Manifest, Wheel, Runtime, HostBinary


def _default_manifest(base: Path) -> Manifest:
    wheels_dir = base / "wheels"
    wheels_dir.mkdir(exist_ok=True)
    return Manifest(
        name="sample_extension",
        version="0.1.0",
        entry_point="sample:main",
        wheels=[],
        runtime=Runtime(),
        host_binaries=[HostBinary("x64", Path("releases/x64/fusionpy_host.dll")), HostBinary("Win32", Path("releases/Win32/fusionpy_host.dll"))],
        resources=[],
    )


def cmd_init(args: argparse.Namespace) -> int:
    manifest_path = Path(args.path)
    if manifest_path.exists() and not args.force:
        print(f"{manifest_path} already exists. Use --force to overwrite.")
        return 1
    manifest = _default_manifest(manifest_path.parent)
    manifest_path.write_text(manifest.to_json())
    print(f"Wrote manifest to {manifest_path}")
    return 0


def cmd_build(args: argparse.Namespace) -> int:
    manifest_path = Path(args.manifest)
    output = Path(args.output)
    mfx = build_extension(manifest_path, output)
    print(f"Built {mfx}")
    return 0


def cmd_run(args: argparse.Namespace) -> int:
    fusion = Path(args.fusion)
    manifest_path = Path(args.manifest)
    injector = Path(args.injector or "releases/x64/fusionpy_injector.exe")
    cmd = [str(injector), str(fusion), str(manifest_path)]
    print(f"Launching Fusion with command: {' '.join(cmd)}")
    subprocess.check_call(cmd)
    return 0


def cmd_publish(args: argparse.Namespace) -> int:
    manifest_path = Path(args.manifest)
    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)
    mfx = build_extension(manifest_path, output_dir)
    # Additional publishing could be added here (NuGet push, etc.).
    print(f"Published artifact: {mfx}")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="fusionpy", description="FusionPy SDK CLI")
    parser.add_argument("--version", action="version", version=__version__)
    sub = parser.add_subparsers(dest="command", required=True)

    p_init = sub.add_parser("init", help="Create a sample manifest")
    p_init.add_argument("--path", default="fusionpy.json")
    p_init.add_argument("--force", action="store_true")
    p_init.set_defaults(func=cmd_init)

    p_build = sub.add_parser("build", help="Build a Fusion extension package")
    p_build.add_argument("--manifest", default="fusionpy.json")
    p_build.add_argument("--output", default="dist")
    p_build.set_defaults(func=cmd_build)

    p_run = sub.add_parser("run", help="Launch Fusion with hot reload")
    p_run.add_argument("--manifest", default="fusionpy.json")
    p_run.add_argument("--fusion", required=True, help="Path to Fusion.exe")
    p_run.add_argument("--injector", help="Path to injector executable")
    p_run.set_defaults(func=cmd_run)

    p_publish = sub.add_parser("publish", help="Build and stage artifacts for release")
    p_publish.add_argument("--manifest", default="fusionpy.json")
    p_publish.add_argument("--output", default="releases")
    p_publish.set_defaults(func=cmd_publish)
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":  # pragma: no cover
    sys.exit(main())
