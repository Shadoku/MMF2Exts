from __future__ import annotations

import compileall
import json
import shutil
import zipfile
from pathlib import Path
from typing import Iterable, List, Optional, Tuple

from .manifest import Manifest, load_manifest


DEFAULT_BUILD_DIR = Path("fusionpy_build")


def _copy_resources(manifest_dir: Path, resource_root: Path, include: Iterable[str], exclude: Iterable[str]) -> List[Path]:
    copied: List[Path] = []
    for pattern in include:
        for path in (manifest_dir / pattern).parent.glob((manifest_dir / pattern).name):
            if any(path.match(ex_pat) for ex_pat in exclude):
                continue
            dest = resource_root / path.relative_to(manifest_dir)
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, dest)
            copied.append(dest)
    return copied


def _freeze_sources(source_root: Path) -> None:
    compileall.compile_dir(source_root, force=True, quiet=1)


def build_extension(manifest_path: Path, *, output_dir: Optional[Path] = None, target: str = "win32", mode: str = "debug") -> Path:
    """
    Validate the manifest, freeze Python sources, bundle resources, and emit an archive.
    The resulting file is a zip-based payload intended to be consumed by the host bridge.
    """

    manifest_path = manifest_path.resolve()
    manifest_dir = manifest_path.parent
    manifest = load_manifest(manifest_path)
    build_root = (output_dir or DEFAULT_BUILD_DIR).resolve()
    build_root.mkdir(parents=True, exist_ok=True)

    staging_dir = build_root / f"{manifest.id}-{manifest.version}-{mode}"
    if staging_dir.exists():
        shutil.rmtree(staging_dir)
    staging_dir.mkdir(parents=True, exist_ok=True)

    # Copy sources
    src_dest = staging_dir / "src"
    shutil.copytree(manifest_dir, src_dest, dirs_exist_ok=True)

    # Freeze bytecode for faster startup
    _freeze_sources(src_dest)

    # Bundle resources explicitly listed
    resources = manifest.resources or {}
    include = resources.get("include", [])
    exclude = resources.get("exclude", [])
    resource_root = staging_dir / "resources"
    resource_root.mkdir(exist_ok=True)
    _copy_resources(manifest_dir, resource_root, include, exclude)

    # Write build metadata
    build_info = {
        "target": target,
        "mode": mode,
        "manifest": manifest.__dict__,
    }
    (staging_dir / "build.json").write_text(json.dumps(build_info, indent=2), encoding="utf-8")

    # Emit distributable archive
    archive_name = build_root / f"{manifest.id}-{manifest.version}-{target}-{mode}.zip"
    if archive_name.exists():
        archive_name.unlink()
    with zipfile.ZipFile(archive_name, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for path in staging_dir.rglob("*"):
            if path.is_file():
                zf.write(path, arcname=path.relative_to(staging_dir))
    return archive_name
