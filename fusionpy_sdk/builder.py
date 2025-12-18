from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path
import venv

from .manifest import Manifest


class FusionBuilder:
    def __init__(self, manifest_path: Path):
        self.manifest_path = manifest_path
        self.manifest = Manifest.from_file(manifest_path)

    def _create_venv(self, workdir: Path) -> Path:
        venv_dir = workdir / "venv"
        builder = venv.EnvBuilder(with_pip=True, clear=True)
        builder.create(venv_dir)
        return venv_dir

    def _install_wheels(self, venv_dir: Path, base_dir: Path):
        if os.environ.get("FUSIONPY_SKIP_PIP"):
            return
        pip_exe = venv_dir / ("Scripts" if sys.platform == "win32" else "bin") / "pip"
        for wheel in self.manifest.wheels:
            wheel_path = (base_dir / wheel.path).resolve()
            subprocess.check_call([str(pip_exe), "install", "--no-deps", str(wheel_path)])

    def build(self, output_dir: Path) -> Path:
        base_dir = self.manifest_path.parent
        self.manifest.validate_hashes(base_dir)
        output_dir.mkdir(parents=True, exist_ok=True)

        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            venv_dir = self._create_venv(tmp_path)
            self._install_wheels(venv_dir, base_dir)

            payload_dir = tmp_path / "payload"
            payload_dir.mkdir()
            # Copy host binaries
            host_dir = payload_dir / "host"
            host_dir.mkdir()
            for host in self.manifest.host_binaries:
                src = (base_dir / host.path).resolve()
                arch_dir = host_dir / host.arch
                arch_dir.mkdir(parents=True, exist_ok=True)
                shutil.copy2(src, arch_dir / src.name)

            # Copy resources
            res_dir = payload_dir / "resources"
            res_dir.mkdir()
            for res in self.manifest.resources:
                src = (base_dir / res).resolve()
                if src.is_file():
                    shutil.copy2(src, res_dir / src.name)

            # Embed manifest and runtime metadata
            manifest_json = payload_dir / "manifest.json"
            manifest_json.write_text(self.manifest.to_json())

            # Bundle venv
            bundle_venv = payload_dir / "venv"
            shutil.copytree(venv_dir, bundle_venv)

            # Create .mfx package
            mfx_path = output_dir / f"{self.manifest.name}-{self.manifest.version}.mfx"
            with zipfile.ZipFile(mfx_path, "w", zipfile.ZIP_DEFLATED) as zf:
                for file_path in payload_dir.rglob('*'):
                    zf.write(file_path, file_path.relative_to(payload_dir))
        return mfx_path


def build_extension(manifest: Path, output_dir: Path) -> Path:
    builder = FusionBuilder(manifest)
    return builder.build(output_dir)
