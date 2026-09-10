@echo off
rem The native performance project is Release-only. Keep this compatibility
rem entry point from launching the stale bin\Debug executable.
call "%~dp0run.cmd"
exit /b %errorlevel%
