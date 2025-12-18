Fusion Python Extension SDK (Prototype)
=======================================

This repository now includes a Python-first SDK prototype (`fusionpy_sdk`) that mirrors the proposed Fusion SDK design.
It focuses on developer experience while acknowledging the host bridge and Windows-first distribution model.

Architecture highlights
-----------------------

* **Host bridge boundary** – The Python package assumes a C++ host that embeds CPython and forwards Fusion callbacks.
  The `RuntimeBridge` class defines the minimal surface that the host is expected to implement (logging, resource access,
  overlap checks). Native shims can attach a concrete implementation at runtime.
* **Manifest validation** – `fusion_extension.yml`/`.json` files are parsed and validated for required fields,
  ACE signatures, resource lists, dependency metadata, sandboxing hints, and capabilities.
* **Python adapter layer** – Decorators (`@action`, `@condition`, `@expression`, `@property_field`) and an `Extension`
  base class gather ACE metadata from type hints and docstrings, wiring lifecycle hooks (`on_create`, `on_frame`,
  `on_save`, `on_load`).
* **Resource loader** – Build tooling copies declared assets into the package bundle alongside frozen Python bytecode.
* **Sandboxing hooks** – Manifest fields for dependency isolation (`dependencies`) and execution policy (`sandbox`)
  are parsed and carried into build metadata for future host enforcement.

Developer workflow
------------------

1. `python -m fusionpy_sdk.cli init MyExtension --path my_extension`  
   Generates `fusion_extension.yml`, a sample module, and an assets folder.
2. `python -m fusionpy_sdk.cli validate my_extension/fusion_extension.yml`  
   Ensures required metadata and ACE declarations are present.
3. `python -m fusionpy_sdk.cli build my_extension/fusion_extension.yml --mode release`  
   Validates, freezes bytecode, copies resources, and emits a zip-based extension payload in `fusionpy_build/`.
4. `python -m fusionpy_sdk.cli run --fusion-path "C:\\Program Files\\Fusion\\Fusion.exe"`  
   Placeholder that documents expected launch semantics; host integration is required for injection/hot-reload.
5. `python -m fusionpy_sdk.cli publish my_extension/fusion_extension.yml --feed <path or url>`  
   Stub that records intent; signing/upload flows can be added later.

Manifest schema (summary)
-------------------------

* `name`, `id`, `version`, `entrypoint` – required identifiers.
* `runtime` – Fusion version and Python runtime expectations.
* `capabilities` – permissions such as `filesystem`, `network`, `registry`.
* `actions`, `conditions`, `expressions` – ACE definitions with `id` and `name` plus optional parameters.
* `properties` – editor-visible properties.
* `resources.include/exclude` – glob patterns for bundling assets.
* `dependencies` – vendored wheels or shared-runtime hints.
* `sandbox` – isolation preferences (e.g., `mode: venv`, timeouts, memory ceilings).
* `metadata` – free-form data for future host features.

Extending the prototype
-----------------------

* Replace `RuntimeBridge` with a host-provided implementation that proxies Fusion services.
* Augment `build_extension` to inject compiled host binaries, sign packages, and produce `.mfx` outputs.
* Implement `run`/`publish` subcommands to integrate with Fusion installations and feed infrastructure.
* Add Cython build paths for performance-sensitive ACEs; leverage the manifest `dependencies` section to pull wheels.
