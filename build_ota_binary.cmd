@echo off
REM Run OTA binary build script (bypasses PowerShell execution policy for this run only)
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build_ota_binary.ps1"
pause
