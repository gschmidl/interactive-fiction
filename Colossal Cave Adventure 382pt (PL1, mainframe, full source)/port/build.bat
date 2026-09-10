@echo off
REM Build the C transliteration of the PL/I Adventure with MinGW gcc.
setlocal
set GCC=C:\tools\strawberry\c\bin\gcc.exe
if not exist "%GCC%" (
  echo gcc not found at %GCC%
  exit /b 1
)
if not exist "%~dp0build" mkdir "%~dp0build"
REM -Wno-unused-label: the PL/I label L1 in PROGRAM is a landing point
REM that nothing branches to; it is kept so the C reads against the
REM original line for line.
"%GCC%" -std=c99 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-unused-label ^
  -o "%~dp0build\adventure.exe" ^
  "%~dp0src\advent.c" "%~dp0src\advsubs.c" "%~dp0src\advars.c" ^
  "%~dp0src\platform.c" "%~dp0src\plisup.c"
if errorlevel 1 (
  echo Build failed.
  exit /b 1
)
copy /y "%~dp0..\port\OBJECT" "%~dp0build\OBJECT" >nul
echo Build OK: build\adventure.exe
