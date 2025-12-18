from __future__ import annotations

import json
from dataclasses import dataclass, field
from pathlib import Path
from typing import List, Optional
import hashlib


@dataclass
class Wheel:
    path: Path
    sha256: str


@dataclass
class Runtime:
    isolation: str = "venv"
    python_version: str = "3.11"
    shared: bool = False


@dataclass
class HostBinary:
    arch: str
    path: Path


@dataclass
class Manifest:
    name: str
    version: str
    entry_point: str
    wheels: List[Wheel] = field(default_factory=list)
    runtime: Runtime = field(default_factory=Runtime)
    host_binaries: List[HostBinary] = field(default_factory=list)
    resources: List[Path] = field(default_factory=list)

    @staticmethod
    def from_file(path: Path) -> "Manifest":
        data = json.loads(path.read_text())
        wheels = [Wheel(Path(w["path"]), w["sha256"]) for w in data.get("wheels", [])]
        runtime_data = data.get("runtime", {})
        runtime = Runtime(
            isolation=runtime_data.get("isolation", "venv"),
            python_version=runtime_data.get("python_version", "3.11"),
            shared=runtime_data.get("shared", False),
        )
        host_binaries = [HostBinary(h["arch"], Path(h["path"])) for h in data.get("host_binaries", [])]
        resources = [Path(r) for r in data.get("resources", [])]
        return Manifest(
            name=data["name"],
            version=data["version"],
            entry_point=data["entry_point"],
            wheels=wheels,
            runtime=runtime,
            host_binaries=host_binaries,
            resources=resources,
        )

    def validate_hashes(self, base_dir: Path) -> None:
        for wheel in self.wheels:
            wheel_path = (base_dir / wheel.path).resolve()
            if not wheel_path.exists():
                raise FileNotFoundError(wheel_path)
            digest = hashlib.sha256(wheel_path.read_bytes()).hexdigest()
            if digest.lower() != wheel.sha256.lower():
                raise ValueError(f"Wheel hash mismatch for {wheel.path}: {digest} != {wheel.sha256}")

    def to_json(self) -> str:
        return json.dumps(
            {
                "name": self.name,
                "version": self.version,
                "entry_point": self.entry_point,
                "wheels": [{"path": str(w.path), "sha256": w.sha256} for w in self.wheels],
                "runtime": {
                    "isolation": self.runtime.isolation,
                    "python_version": self.runtime.python_version,
                    "shared": self.runtime.shared,
                },
                "host_binaries": [{"arch": h.arch, "path": str(h.path)} for h in self.host_binaries],
                "resources": [str(r) for r in self.resources],
            },
            indent=2,
        )
