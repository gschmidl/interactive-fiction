@echo off
rem Build the HP 2000 Access Time-Shared BASIC interpreter and game launcher.
rem Any C99 compiler will do; this is what was used.
setlocal
if "%CC%"=="" set CC=gcc
%CC% -std=c99 -O2 -Wall -Wextra -o "%~dp0advent.exe" "%~dp0src\hptsb.c" -lm
if errorlevel 1 (echo BUILD FAILED & exit /b 1)
echo Built %~dp0advent.exe
