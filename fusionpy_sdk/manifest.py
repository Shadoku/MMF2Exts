from __future__ import annotations

import json
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

try:
    import yaml
except ModuleNotFoundError:  # pragma: no cover - optional dependency
    yaml = None


REQUIRED_KEYS = {"name", "id", "version", "entrypoint"}


@dataclass
class Manifest:
    name: str
    id: str
    version: str
    entrypoint: str
    runtime: Dict[str, Any] = field(default_factory=dict)
    capabilities: List[str] = field(default_factory=list)
    actions: List[Dict[str, Any]] = field(default_factory=list)
    conditions: List[Dict[str, Any]] = field(default_factory=list)
    expressions: List[Dict[str, Any]] = field(default_factory=list)
    properties: List[Dict[str, Any]] = field(default_factory=list)
    resources: Dict[str, Any] = field(default_factory=dict)
    dependencies: Dict[str, Any] = field(default_factory=dict)
    sandbox: Dict[str, Any] = field(default_factory=dict)
    metadata: Dict[str, Any] = field(default_factory=dict)


def _load_raw_manifest(path: Path) -> Dict[str, Any]:
    if not path.exists():
        raise FileNotFoundError(f"Manifest file not found: {path}")

    if path.suffix.lower() in {".yml", ".yaml"}:
        if yaml is None:
            raise RuntimeError("PyYAML is required to parse YAML manifests")
        data = yaml.safe_load(path.read_text(encoding="utf-8"))
    else:
        data = json.loads(path.read_text(encoding="utf-8"))

    if not isinstance(data, dict):
        raise ValueError("Manifest root must be a mapping/dictionary")
    return data


def _validate_required(data: Dict[str, Any]) -> None:
    missing = REQUIRED_KEYS - data.keys()
    if missing:
        raise ValueError(f"Manifest missing required fields: {', '.join(sorted(missing))}")
    for key in ["name", "id", "version", "entrypoint"]:
        if not isinstance(data.get(key), str) or not data[key].strip():
            raise ValueError(f"Manifest field '{key}' must be a non-empty string")


def _extract_ace_entries(data: Dict[str, Any], key: str) -> List[Dict[str, Any]]:
    entries = data.get(key, [])
    if entries is None:
        return []
    if not isinstance(entries, list):
        raise ValueError(f"Manifest field '{key}' must be a list")
    for entry in entries:
        if not isinstance(entry, dict):
            raise ValueError(f"Entries in '{key}' must be dictionaries")
        if "name" not in entry or "id" not in entry:
            raise ValueError(f"Entries in '{key}' must define 'id' and 'name'")
    return entries


def _validate_resources(resources: Dict[str, Any]) -> None:
    for list_key in ("include", "exclude"):
        value = resources.get(list_key, [])
        if value is None:
            continue
        if not isinstance(value, list):
            raise ValueError(f"resources.{list_key} must be a list of paths")
        if not all(isinstance(item, str) for item in value):
            raise ValueError(f"resources.{list_key} entries must be strings")


def _validate_dependencies(dependencies: Dict[str, Any]) -> None:
    wheels = dependencies.get("wheels", [])
    if wheels is None:
        return
    if not isinstance(wheels, list) or not all(isinstance(item, str) for item in wheels):
        raise ValueError("dependencies.wheels must be a list of wheel filenames")


def validate_manifest(data: Dict[str, Any]) -> Manifest:
    """
    Validate manifest data and return a normalized Manifest object.
    Raises ValueError when validation fails.
    """

    _validate_required(data)
    runtime = data.get("runtime", {})
    if runtime and not isinstance(runtime, dict):
        raise ValueError("runtime must be a mapping")

    capabilities = data.get("capabilities", [])
    if capabilities is None:
        capabilities = []
    if not isinstance(capabilities, list) or not all(isinstance(item, str) for item in capabilities):
        raise ValueError("capabilities must be a list of strings")

    actions = _extract_ace_entries(data, "actions")
    conditions = _extract_ace_entries(data, "conditions")
    expressions = _extract_ace_entries(data, "expressions")
    properties = data.get("properties", [])
    if properties and not isinstance(properties, list):
        raise ValueError("properties must be a list")

    resources = data.get("resources", {})
    if resources and not isinstance(resources, dict):
        raise ValueError("resources must be a mapping")
    _validate_resources(resources)

    dependencies = data.get("dependencies", {})
    if dependencies and not isinstance(dependencies, dict):
        raise ValueError("dependencies must be a mapping")
    _validate_dependencies(dependencies)

    sandbox = data.get("sandbox", {})
    if sandbox and not isinstance(sandbox, dict):
        raise ValueError("sandbox must be a mapping")

    metadata = data.get("metadata", {})
    if metadata and not isinstance(metadata, dict):
        raise ValueError("metadata must be a mapping")

    return Manifest(
        name=data["name"],
        id=data["id"],
        version=data["version"],
        entrypoint=data["entrypoint"],
        runtime=runtime or {},
        capabilities=capabilities,
        actions=actions,
        conditions=conditions,
        expressions=expressions,
        properties=properties or [],
        resources=resources or {},
        dependencies=dependencies or {},
        sandbox=sandbox or {},
        metadata=metadata or {},
    )


def load_manifest(path: str | Path) -> Manifest:
    raw = _load_raw_manifest(Path(path))
    return validate_manifest(raw)
