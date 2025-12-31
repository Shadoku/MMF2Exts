from pathlib import Path
import hashlib
import os

from fusionpy_sdk.builder import build_extension


def create_dummy_wheel(tmp_path: Path) -> tuple[Path, str]:
    wheel = tmp_path / "demo-0.0.0-py3-none-any.whl"
    wheel.write_text("placeholder wheel content")
    digest = hashlib.sha256(wheel.read_bytes()).hexdigest()
    return wheel, digest


def test_build_creates_mfx(tmp_path: Path, monkeypatch):
    wheel, digest = create_dummy_wheel(tmp_path)
    manifest = tmp_path / "fusionpy.json"
    host_binary = tmp_path / "fusionpy_host.dll"
    host_binary.write_text("host")
    manifest.write_text(
        f"""
{{
  "name": "demo",
  "version": "0.0.1",
  "entry_point": "demo:main",
  "wheels": [{{"path": "{wheel.name}", "sha256": "{digest}"}}],
  "host_binaries": [{{"arch": "x64", "path": "{host_binary.name}"}}],
  "resources": []
}}
"""
    )
    monkeypatch.setenv("FUSIONPY_SKIP_PIP", "1")
    output_dir = tmp_path / "dist"
    result = build_extension(manifest, output_dir)
    assert result.exists()
    assert result.suffix == ".mfx"
