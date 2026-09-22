@echo off
rem Requires a MinGW-w64 gcc and an sh (Git for Windows / MSYS2) on PATH,
rem and ctangle (native, or in WSL Debian).
sh "%~dp0build.sh" %*
