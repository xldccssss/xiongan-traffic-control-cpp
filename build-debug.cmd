@echo off
rem The native performance project is Release-only. Never rebuild the stale
rem bin\Debug executable; delegate to the supported Release build.
call "%~dp0build-release.cmd"
exit /b %errorlevel%
