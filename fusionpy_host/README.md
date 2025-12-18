# fusionpy host bridge

This folder contains a minimal Fusion-compatible host bridge that loads a packaged Python runtime.

## Building
- **CMake**: `cmake -S fusionpy_host -B fusionpy_host/build -DFUSIONPY_BUILD_XP=ON` followed by `cmake --build fusionpy_host/build --config Release`.
- **MSBuild**: open `fusionpy_host.sln` in Visual Studio 2019/2022 and build `Release|Win32` or `Release|x64`.

Artifacts are emitted to `fusionpy_host/build/<arch>/Release/bin` and can be packaged via `scripts/package_host.py --version <ver>`.
