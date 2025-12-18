@echo off
setlocal
set SDK_ROOT=%~dp0..
set PY_HOME=%SDK_ROOT%\python
if not exist "%PY_HOME%\python.exe" (
    echo Bundled python runtime missing.>&2
    exit /b 1
)
set PYTHONHOME=%PY_HOME%
set PYTHONPATH=%PY_HOME%
"%PY_HOME%\python.exe" -m pip install --no-deps --upgrade "%SDK_ROOT%\wheels\fusionpy_sdk*.whl" >nul
"%PY_HOME%\python.exe" -m fusionpy_sdk.cli %*
