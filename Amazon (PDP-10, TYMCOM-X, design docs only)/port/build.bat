@echo off
rem AMAZON -- Windows port.  Needs MinGW-w64 gcc on PATH.
setlocal
if not exist bin mkdir bin
gcc -std=c99 -O2 -Wall -Wextra -o bin\amazon.exe ^
    src\main.c src\game.c src\world.c src\parse.c src\riddles.c src\share.c
if errorlevel 1 (
    echo BUILD FAILED
    exit /b 1
)
echo Built bin\amazon.exe
endlocal
