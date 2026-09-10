@echo off
setlocal
set "VS_ROOT=F:\DevTools\VisualStudio\2022\Community"
set "QT_ROOT=F:\Qt\5.15.2\msvc2019_64"
set "TESSNG_ROOT=F:\TESSNG_4.1.3"
call "%VS_ROOT%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b %errorlevel%
"%VS_ROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --preset vs2022-release
if errorlevel 1 exit /b %errorlevel%
"%VS_ROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build --preset release --parallel
exit /b %errorlevel%
