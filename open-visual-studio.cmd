@echo off
setlocal
set "VS_ROOT=F:\DevTools\VisualStudio\2022\Community"
if not exist "%VS_ROOT%\Common7\IDE\devenv.exe" (
    echo Visual Studio Community 2022 was not found at %VS_ROOT%.
    exit /b 2
)
for %%I in ("%~dp0.") do set "PROJECT_DIR=%%~fI"

rem Official CMake workflow: open the source folder that contains
rem CMakeLists.txt. Visual Studio configures the project from
rem CMakePresets.json; no generated .sln file is opened.
start "Visual Studio - TESSNG CMake" "%VS_ROOT%\Common7\IDE\devenv.exe" "%PROJECT_DIR%"
