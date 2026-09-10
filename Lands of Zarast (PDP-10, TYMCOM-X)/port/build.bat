@echo off
rem Build both LANDS OF ZARAST binaries.  Needs a C99 compiler on the
rem PATH; this was built with the mingw-w64 gcc that ships with
rem Strawberry Perl.
setlocal
if not exist bin mkdir bin
set COMMON=src\main.c src\cpu.c src\monitor.c src\load.c
gcc -std=c99 -Wall -Wextra -O2 -o bin\zarast.exe   %COMMON% src\images_84.c -lm
if errorlevel 1 (echo BUILD FAILED & exit /b 1)
gcc -std=c99 -Wall -Wextra -O2 -o bin\zarast87.exe %COMMON% src\images_87.c -lm
if errorlevel 1 (echo BUILD FAILED & exit /b 1)
echo built bin\zarast.exe and bin\zarast87.exe
