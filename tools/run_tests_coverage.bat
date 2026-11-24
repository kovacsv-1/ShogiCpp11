@echo off
REM Wrapper: prefer PowerShell script (reliable on Windows).
cd /d "%~dp0\.."
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0run_tests_coverage.ps1"
exit /b %ERRORLEVEL%
