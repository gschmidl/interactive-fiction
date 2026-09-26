@echo off
rem Builds googleadventure.exe from src\googleadventure.c using a native
rem Windows gcc (MinGW-w64) taken from PATH; tested with the gcc 15.2 that
rem Strawberry Perl bundles.

setlocal
if not exist bin mkdir bin
gcc -std=c99 -Wall -Wextra -O2 -o bin\googleadventure.exe src\googleadventure.c
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)
echo Built bin\googleadventure.exe
endlocal
