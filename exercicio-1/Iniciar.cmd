@echo off
if "%~1"=="" (
    powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\executar.ps1"
) else (
    powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\executar.ps1" -Video "%~1"
)
if errorlevel 1 pause
