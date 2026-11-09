@echo off
rem Requires MinGW-w64 gcc with 32-bit libraries (Strawberry Perl's has
rem them), python, and an sh (Git for Windows or MSYS2) on PATH
sh "%~dp0build.sh" %*
