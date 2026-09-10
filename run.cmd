@echo off
setlocal
set "QT_ROOT=F:\Qt\5.15.2\msvc2019_64"
set "TESSNG_ROOT=F:\TESSNG_4.1.3"
set "TESSNG_RUNTIME_DIR=F:\TESSNG_4.1.3"
set "QT_PLUGIN_PATH=%~dp0bin\Release\plugins"
set "QML2_IMPORT_PATH=%~dp0bin\Release"
set "PATH=%~dp0bin\Release;%TESSNG_RUNTIME_DIR%;%QT_ROOT%\bin;%PATH%"
if not exist "%~dp0bin\Release\XionganTrafficNativeCpp.exe" (
    echo Release executable not found. Run build-release.cmd first.
    exit /b 2
)
start "雄安路网原生C++仿真" /D "%~dp0" "%~dp0bin\Release\XionganTrafficNativeCpp.exe"
