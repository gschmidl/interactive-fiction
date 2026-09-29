@echo off
rem Builds adv462.exe in the default WSL distribution, which needs the
rem MinGW-w64 cross compilers (Debian: gfortran-mingw-w64, gcc-mingw-w64) and
rem python3.  The program it makes is a native Windows one: it does not need
rem WSL to run.  In an MSYS2 or Git Bash shell, run build.sh directly.
wsl --cd "%~dp0." -- sh ./build.sh %*
