@echo off
rem Requires MinGW-w64 gcc + gfortran and an sh (Git for Windows / MSYS2) on PATH.
sh "%~dp0build.sh" %*
