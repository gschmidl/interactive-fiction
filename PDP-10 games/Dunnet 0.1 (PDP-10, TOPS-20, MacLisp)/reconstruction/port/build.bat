@echo off
rem Builds dunnet.exe through build.sh in WSL (needs mingw-w64 and python3 there).
cd /d "%~dp0"
wsl sh ./build.sh
