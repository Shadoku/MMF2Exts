from pathlib import Path
import json

from fusionpy_sdk.manifest import Manifest, Wheel


def test_manifest_roundtrip(tmp_path: Path):
    manifest_path = tmp_path / "manifest.json"
    data = {
        "name": "demo",
        "version": "1.0.0",
        "entry_point": "demo:main",
        "wheels": [{"path": "wheels/demo.whl", "sha256": "deadbeef"}],
    }
    manifest_path.write_text(json.dumps(data))
    manifest = Manifest.from_file(manifest_path)
    assert manifest.name == "demo"
    assert manifest.wheels[0].path.as_posix() == "wheels/demo.whl"
    serialized = manifest.to_json()
    assert "demo" in serialized
