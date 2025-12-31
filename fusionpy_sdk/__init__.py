"""
Fusion Python Extension SDK (fusionpy_sdk)
==========================================

This package provides authoring tools for writing Fusion extensions in
pure Python while keeping compatibility with Fusion's native extension
model. The public surface intentionally mirrors the proposed developer
workflow:

* Decorators and a lightweight ``Extension`` base class for defining ACE
  tables and lifecycle hooks.
* Manifest utilities to validate ``fusion_extension`` descriptors.
* Build helpers to freeze Python sources, vendor resources, and emit a
  distributable archive ready for the host bridge to consume.
* A CLI entry point (``fusionpy``) that exposes ``init``, ``validate``,
  ``build``, ``run``, and ``publish`` commands.

The code here is platform-neutral but geared toward the Windows-first
distribution model outlined in the proposal.
"""

from .decorators import action, condition, expression, property_field
from .extension import Extension
from .manifest import Manifest, load_manifest, validate_manifest

__all__ = [
    "Extension",
    "Manifest",
    "action",
    "condition",
    "expression",
    "property_field",
    "load_manifest",
    "validate_manifest",
]

__version__ = "0.1.0"
