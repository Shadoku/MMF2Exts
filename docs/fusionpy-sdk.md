# fusionpy SDK Quickstart

## One-step installer
1. Download the latest `fusionpy-bundle-<version>.zip` from `releases/`.
2. Extract and run `installer/fusionpy_installer.ps1`:
   ```powershell
   powershell -ExecutionPolicy Bypass -File installer\fusionpy_installer.ps1 -FusionPath "C:\\Program Files\\Clickteam\\Fusion 2.5\\Fusion.exe" -AddToPath
   ```
   The script deploys the host bridge (x86/x64), a minimal Python runtime, and the `fusionpy_sdk` wheel. It also drops `fusionpy.cmd` into the install folder so `fusionpy init|build|run|publish` is available immediately.

## CLI usage
```powershell
fusionpy init --path fusionpy.json
fusionpy build --manifest fusionpy.json --output dist
fusionpy run --fusion "C:\\...\\Fusion.exe" --manifest fusionpy.json
fusionpy publish --manifest fusionpy.json --output releases
```

## Manifest fields
- `name`, `version`, `entry_point`: Basic metadata for the extension and host loader.
- `runtime`: `isolation` (`venv` or `shared`) and `python_version` guidance for the packaged runtime.
- `wheels`: Each wheel path plus `sha256` checksum. Hashes are enforced during build.
- `host_binaries`: Paths to the host bridge DLL/EXE per architecture.
- `resources`: Optional files bundled into the `.mfx` archive.

## Packaging outputs
`fusionpy build` produces a `.mfx` archive containing:
- Packaged venv with the required wheels
- Host bridge binaries for Win32/Win64
- Manifest metadata and resources

These artifacts are staged under `dist/` by default and mirrored under `releases/` when using the publish workflow or CI.
