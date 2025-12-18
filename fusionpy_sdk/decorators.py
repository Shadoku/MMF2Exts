from __future__ import annotations

import functools
from dataclasses import dataclass, field
from typing import Any, Callable, Dict, List, Optional


@dataclass
class ACEParameter:
    name: str
    type_hint: Optional[str] = None
    description: Optional[str] = None


@dataclass
class ACEEntry:
    kind: str
    title: str
    method_name: str
    display: Optional[str] = None
    parameters: List[ACEParameter] = field(default_factory=list)
    returns: Optional[str] = None
    flags: Dict[str, Any] = field(default_factory=dict)


@dataclass
class PropertyField:
    label: str
    default: Any = None
    description: Optional[str] = None
    editor: Optional[str] = None
    flags: Dict[str, Any] = field(default_factory=dict)


def _build_ace_entry(kind: str, title: str, display: Optional[str], flags: Dict[str, Any]) -> Callable:
    def decorator(func: Callable) -> Callable:
        annotations = getattr(func, "__annotations__", {})
        params = [
            ACEParameter(name=name, type_hint=type_hint.__name__ if hasattr(type_hint, "__name__") else str(type_hint))
            for name, type_hint in annotations.items()
            if name != "return"
        ]
        returns = annotations.get("return")
        entry = ACEEntry(
            kind=kind,
            title=title,
            method_name=func.__name__,
            display=display,
            parameters=params,
            returns=returns.__name__ if hasattr(returns, "__name__") else (str(returns) if returns else None),
            flags=flags or {},
        )
        setattr(func, "__fusion_ace__", entry)
        return functools.wraps(func)(func)

    return decorator


def action(title: str, *, display: Optional[str] = None, **flags: Any) -> Callable:
    """
    Mark a method as a Fusion action.
    The decorator captures type hints for parameter marshaling and editor UI.
    """

    return _build_ace_entry("action", title, display, flags)


def condition(title: str, *, display: Optional[str] = None, **flags: Any) -> Callable:
    """
    Mark a method as a Fusion condition (returns a boolean).
    """

    return _build_ace_entry("condition", title, display, flags)


def expression(title: str, *, display: Optional[str] = None, **flags: Any) -> Callable:
    """
    Mark a method as a Fusion expression.
    """

    return _build_ace_entry("expression", title, display, flags)


def property_field(label: str, default: Any = None, description: Optional[str] = None, *, editor: Optional[str] = None, **flags: Any) -> Callable:
    """
    Decorate an attribute or method to describe an editor-visible property.
    For attribute-style properties, decorate a zero-argument method that returns the default value.
    """

    def decorator(func: Callable) -> Callable:
        field_def = PropertyField(label=label, default=default, description=description, editor=editor, flags=flags)
        setattr(func, "__fusion_property__", field_def)
        return functools.wraps(func)(func)

    return decorator
