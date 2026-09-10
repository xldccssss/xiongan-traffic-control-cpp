@echo off
setlocal
set "QT_ROOT=F:\Qt\5.15.2\msvc2019_64"
set "TESSNG_RUNTIME_DIR=F:\TESSNG_4.1.3"
set "QT_PLUGIN_PATH=%~dp0bin\Release\plugins"
set "QML2_IMPORT_PATH=%~dp0bin\Release"
set "PATH=%~dp0bin\Release;%TESSNG_RUNTIME_DIR%;%QT_ROOT%\bin;%PATH%"
set "SMOKE_DURATION=%~1"
if "%SMOKE_DURATION%"=="" set "SMOKE_DURATION=6"
set "SMOKE_TARGET_MULTIPLE=%~2"
if "%SMOKE_TARGET_MULTIPLE%"=="" set "SMOKE_TARGET_MULTIPLE=1"
set "SMOKE_BRANCH=%~3"
if "%SMOKE_BRANCH%"=="" set "SMOKE_BRANCH=IMPROVED_MAX_PRESSURE"
if not exist "%~dp0bin\Release\XionganTrafficNativeCpp.exe" (
    echo Release executable not found. Run build-release.cmd first.
    exit /b 2
)
del /q "%~dp0generated\native_runtime_smoke_report.json" 2>nul
start "" /wait /D "%~dp0" "%~dp0bin\Release\XionganTrafficNativeCpp.exe" --runtime-smoke-test --runtime-smoke-duration=%SMOKE_DURATION% --runtime-smoke-target-multiple=%SMOKE_TARGET_MULTIPLE% --runtime-smoke-branch=%SMOKE_BRANCH%
set "APP_EXIT=%ERRORLEVEL%"
if not exist "%~dp0generated\native_runtime_smoke_report.json" (
    echo Runtime verification report was not created. Process exit code: %APP_EXIT%
    exit /b 3
)
"%SystemRoot%\System32\findstr.exe" /C:"success" "%~dp0generated\native_runtime_smoke_report.json" | "%SystemRoot%\System32\findstr.exe" /C:"true" >nul
if errorlevel 1 (
    type "%~dp0generated\native_runtime_smoke_report.json"
    exit /b 4
)
type "%~dp0generated\native_runtime_smoke_report.json"
exit /b 0
