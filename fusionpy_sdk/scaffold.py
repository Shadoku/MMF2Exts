from __future__ import annotations

import textwrap
from pathlib import Path


TEMPLATE_MANIFEST = """\
name: {name}
id: {identifier}
version: 0.1.0
entrypoint: {module}.MyExtension
runtime:
  fusion_versions: ["2.5"]
  python: "3.11"
capabilities:
  - filesystem
actions:
  - id: show_greeting
    name: Show greeting
    parameters:
      - name: target
        type: string
conditions:
  - id: player_in_area
    name: Player in area
expressions:
  - id: get_score
    name: Get score
properties:
  - id: greeting
    label: Greeting
    type: string
resources:
  include:
    - assets/**
dependencies:
  wheels: []
sandbox:
  mode: venv
"""


TEMPLATE_EXTENSION = '''\
from fusionpy_sdk import Extension, action, condition, expression, property_field


class MyExtension(Extension):
    @property_field(label="Greeting", default="Hello Fusion devs!")
    def greeting(self):
        return "Hello Fusion devs!"

    @action("Show greeting")
    def show(self, target: str = "Debugger"):
        self.runtime.log(f"{self.greeting} (target: {target})")

    @condition("Player in area")
    def player_in_area(self, x: int, y: int) -> bool:
        return self.runtime.check_overlap(x, y)

    @expression("Get score")
    def get_score(self) -> int:
        return int(self.runtime.variables.get("score", 0))

    def on_create(self):
        self.runtime.log("MyExtension created", level=20)
'''


def init_extension(name: str, target_dir: Path) -> None:
    target_dir.mkdir(parents=True, exist_ok=True)
    identifier = name.lower().replace(" ", "_")
    module_name = identifier
    manifest_path = target_dir / "fusion_extension.yml"
    module_path = target_dir / f"{module_name}.py"
    assets_dir = target_dir / "assets"
    assets_dir.mkdir(exist_ok=True)

    if not manifest_path.exists():
        manifest_path.write_text(TEMPLATE_MANIFEST.format(name=name, identifier=identifier, module=module_name), encoding="utf-8")

    if not module_path.exists():
        module_path.write_text(TEMPLATE_EXTENSION, encoding="utf-8")
