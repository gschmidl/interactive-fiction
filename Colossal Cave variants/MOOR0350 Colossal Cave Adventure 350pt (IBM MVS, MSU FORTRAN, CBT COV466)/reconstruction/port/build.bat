@echo off
rem Requires MinGW-w64 gcc and gfortran, python, and an sh (Git for Windows
rem or MSYS2) on PATH.  "build.bat --tests" also builds the test drivers.
sh "%~dp0build.sh" %*
