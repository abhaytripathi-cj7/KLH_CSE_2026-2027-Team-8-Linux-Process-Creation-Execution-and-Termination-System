@echo off
title PipeTalk - Linux IPC Chat
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\start_windows.ps1"
if errorlevel 1 (
  echo.
  echo PipeTalk stopped. Read the message above or see README.md.
  pause
)
