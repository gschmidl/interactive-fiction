@echo off
rem Requires MinGW-w64 gcc, python, and an sh (Git for Windows or MSYS2) on
rem PATH
sh "%~dp0build.sh" %*
