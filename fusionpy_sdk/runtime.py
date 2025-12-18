from __future__ import annotations

import logging
from pathlib import Path
from typing import Any, Dict


class RuntimeBridge:
    """
    Placeholder runtime surface that matches Fusion services exposed by the host bridge.

    In production, the host bridge provides concrete implementations wired to Fusion's
    ABI. During local testing, this stub can be used to simulate debugger output and
    simple storage access.
    """

    def __init__(self, resource_root: Path | None = None) -> None:
        self.log_channel = logging.getLogger("fusionpy.runtime")
        self.resource_root = resource_root or Path.cwd()
        self.variables: Dict[str, Any] = {}

    def log(self, message: str, level: int = logging.INFO) -> None:
        self.log_channel.log(level, message)

    def get_resource(self, relative_path: str) -> Path:
        candidate = self.resource_root / relative_path
        if not candidate.exists():
            raise FileNotFoundError(f"Resource '{relative_path}' not found at {candidate}")
        return candidate

    def check_overlap(self, x: int, y: int) -> bool:
        """
        Placeholder for collision/overlap checks against the Fusion scene.
        The host bridge replaces this with engine-backed logic.
        """

        self.log(f"Simulated overlap check at ({x}, {y})", logging.DEBUG)
        return False
