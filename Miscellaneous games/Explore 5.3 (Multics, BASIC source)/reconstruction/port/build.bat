@echo off
rem Requires MinGW-w64 gcc, Strawberry Perl (set SPERL to its perl.exe unless
rem it is the first perl on PATH), and an sh (Git for Windows or MSYS2) on PATH
sh "%~dp0build.sh" %*
