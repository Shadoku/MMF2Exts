from __future__ import annotations

import inspect
from dataclasses import dataclass, field
from typing import Any, Dict, Iterable, List, Optional, Type

from .decorators import ACEEntry, PropertyField
from .runtime import RuntimeBridge


@dataclass
class ExtensionMetadata:
    """Container describing the ACE table and properties derived from decorators."""

    name: str
    actions: List[ACEEntry] = field(default_factory=list)
    conditions: List[ACEEntry] = field(default_factory=list)
    expressions: List[ACEEntry] = field(default_factory=list)
    properties: List[PropertyField] = field(default_factory=list)


class Extension:
    """
    Base class for Python-first Fusion extensions.

    Subclasses can implement lifecycle hooks:
    * on_create(self)
    * on_frame(self, delta_time: float)
    * on_save(self) -> dict
    * on_load(self, state: dict)
    """

    runtime: RuntimeBridge

    def __init__(self, runtime: Optional[RuntimeBridge] = None) -> None:
        self.runtime = runtime or RuntimeBridge()

    @classmethod
    def collect_metadata(cls: Type["Extension"]) -> ExtensionMetadata:
        actions: List[ACEEntry] = []
        conditions: List[ACEEntry] = []
        expressions: List[ACEEntry] = []
        properties: List[PropertyField] = []

        for _, member in inspect.getmembers(cls, predicate=inspect.isfunction):
            ace = getattr(member, "__fusion_ace__", None)
            if ace:
                if ace.kind == "action":
                    actions.append(ace)
                elif ace.kind == "condition":
                    conditions.append(ace)
                elif ace.kind == "expression":
                    expressions.append(ace)
            prop = getattr(member, "__fusion_property__", None)
            if prop:
                properties.append(prop)

        return ExtensionMetadata(
            name=cls.__name__,
            actions=actions,
            conditions=conditions,
            expressions=expressions,
            properties=properties,
        )

    def on_create(self) -> None:
        """Lifecycle hook invoked when the extension instance is created."""

    def on_frame(self, delta_time: float) -> None:
        """Lifecycle hook invoked every Fusion frame."""

    def on_save(self) -> Dict[str, Any]:
        """Return a serializable object representing extension state."""
        return {}

    def on_load(self, state: Dict[str, Any]) -> None:
        """Restore state previously returned by on_save."""
        _ = state
