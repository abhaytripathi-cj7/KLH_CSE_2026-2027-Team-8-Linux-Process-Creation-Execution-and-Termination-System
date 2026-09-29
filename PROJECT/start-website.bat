@echo off
cd /d "%~dp0"
echo Starting ProcessPay at http://localhost:8000
echo Keep this window open. Press Ctrl+C to stop.
wsl.exe -d Ubuntu --cd "%~dp0." -- python3 server.py
pause
