@echo off
rem Builds googleadventure.exe from src\googleadventure.c using a native
rem Windows gcc (MinGW-w64). Tested with Strawberry Perl's bundled gcc at
rem C:\tools\strawberry\c\bin\gcc.exe (GCC 15.2) - adjust PATH if you use
rem a different MinGW-w64 install.

setlocal
if not exist bin mkdir bin
gcc -std=c99 -Wall -Wextra -O2 -o bin\googleadventure.exe src\googleadventure.c
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)
echo Built bin\googleadventure.exe
endlocal
