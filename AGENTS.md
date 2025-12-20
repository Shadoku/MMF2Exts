DarkEdif Windows-only extension guide for agents
================================================

This repository ships several SDKs, but DarkEdif is the most featureful and is already wired for Windows targets. Follow the notes below when adding new Windows-only DarkEdif extensions.

DarkEdif at a glance
--------------------
* DarkEdif extends the older Edif SDK with multi-language JSON metadata, smart properties, runtime JSON validation, a JSON minifier for runtimes, runtime crash helpers, Fusion debugger hooks, and an optional SDK/update checker tool. 【F:README.md†L100-L117】
* All projects share a central Visual Studio props file in `DarkEdif/Lib` that auto-applies configuration defaults; new configurations inherit the right Unicode/debug flags automatically. 【F:README.md†L76-L83】
* DarkEdif requires a modern MSVC in C++17 mode; builds with older Visual Studio releases or non-C++17 modes are rejected. 【F:DarkEdif/Inc/Shared/AllPlatformDefines.hpp†L3-L30】

Where DarkEdif is used
----------------------
Use existing Windows project files as references:
* Templates: `DarkEdif Template.Windows.vcxproj` (paired with shared items and JSON metadata). 【F:AllExts VS2019.sln†L88-L95】
* Shipping examples: `DarkSocket`, `DebugObject`, `UTF16 Object`, `Klystrack`, and `Ext Menus` all target Windows via DarkEdif. 【F:AllExts VS2019.sln†L56-L75】
* Multi-target suites (Windows plus other platforms): Bluewing Client/Server, Phi Object, DarkScript, and AndroidManifestMod show how shared items feed multiple platform-specific projects; you can still focus on the `*.Windows.vcxproj` variants. 【F:AllExts VS2019.sln†L96-L147】

Windows build and targeting rules
---------------------------------
* Recommended IDE: Visual Studio 2019; 2017 works with reduced platform reach, and 2022 drops Windows XP support. 【F:README.md†L8-L48】
* XP compatibility: enabled by default when the VS XP targeting pack is installed; turn it off via `FusionSDKConfig.ini` (`WindowsXPCompatibility = false`) if you need Vista+ APIs, and align WINVER/_WIN32_WINNT and linker minimum version accordingly. 【F:README.md†L16-L40】
* Toolset floor: for XP-safe binaries, stick to MSVC v14.27 (VS 2019 16.7) to avoid CRT changes that silently use Vista-only primitives. 【F:DarkEdif/#MFAs and documentation/DarkEdif MultiTargeting.md†L41-L66】
* If builds complain about missing `WindowsSDKDir`, install the right Windows SDK (7.0 for XP, 8.1 for Vista+, or 10/11 if you drop older OSes). 【F:DarkEdif/#MFAs and documentation/DarkEdif MultiTargeting.md†L17-L31】

Common Windows runtime pitfalls
-------------------------------
* “Cannot load ExtName.mfx” usually means an unsupported OS call, missing redistributable (when linking against DLL CRT), or absent third-party DLLs; ensure static CRT where possible and keep dependencies beside Fusion’s editor/runtime paths. 【F:DarkEdif/#MFAs and documentation/DarkEdif MultiTargeting.md†L88-L115】
* On XP targets, function-scope statics with non-trivial constructors can misinitialize unless `/Zc:threadSafeInit-` is set; prefer file-scope statics or guard code with `ThreadSafeStaticInitIsSafe`. 【F:DarkEdif/#MFAs and documentation/DarkEdif MultiTargeting.md†L118-L154】

Authoring a Windows-only DarkEdif extension
-------------------------------------------
1) Start from `DarkEdif/DarkEdif Template`:
   * Use the Windows project (`DarkEdif Template.Windows.vcxproj`) and keep the shared items so headers/source stay consistent. 【F:AllExts VS2019.sln†L88-L95】
   * Define object metadata, menus, ACEs, and properties in `DarkExt.json`; the template shows identifier requirements, multi-language About info, menus, and diverse property types (folders, checkboxes, text, edit boxes). 【F:DarkEdif/DarkEdif Template/DarkExt.json†L2-L136】
2) Keep builds Windows-only by:
   * Removing or ignoring non-Windows configurations in Solution Explorer (Android/iOS/Mac) and ensuring `FusionSDKConfig.ini` leaves only Windows toolchains enabled.
   * Verifying WINVER/_WIN32_WINNT and linker minimum version match your chosen floor (XP by default). 【F:README.md†L16-L40】
3) Implement extension code under the shared items:
   * C++17 is mandatory; avoid backporting language features. 【F:DarkEdif/Inc/Shared/AllPlatformDefines.hpp†L3-L30】
   * Use the DarkEdif property helpers instead of manual EDITDATA layout unless you intentionally opt into `NOPROPS`.
4) Test loading in Fusion editor and built EXEs on the oldest OS you intend to support; rely on static CRT to avoid redistributable dependencies and use `Dependency Walker`/`Dependencies` for missing API checks. 【F:DarkEdif/#MFAs and documentation/DarkEdif MultiTargeting.md†L88-L115】

Quick references
----------------
* Root README: high-level SDK comparison and tooling expectations. 【F:README.md†L8-L134】
* Multi-target guide: Windows SDK/toolset nuances, XP caveats, and load-error triage. 【F:DarkEdif/#MFAs and documentation/DarkEdif MultiTargeting.md†L17-L154】
* Template JSON: starter ACE/property definitions to copy. 【F:DarkEdif/DarkEdif Template/DarkExt.json†L2-L136】
