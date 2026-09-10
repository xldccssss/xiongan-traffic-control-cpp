@echo off
setlocal
set "VS_ROOT=F:\DevTools\VisualStudio\2022\Community"
set "QT_ROOT=F:\Qt\5.15.2\msvc2019_64"
set "TESSNG_ROOT=F:\TESSNG_4.1.3"
set "PATH=%~dp0runtime;%QT_ROOT%\bin;%PATH%"
"%VS_ROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --preset release --output-on-failure
exit /b %errorlevel%
