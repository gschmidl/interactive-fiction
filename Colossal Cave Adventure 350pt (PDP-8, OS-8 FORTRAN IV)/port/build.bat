@echo off
rem Build the native Adventure with the Strawberry Perl mingw-w64 toolchain.
setlocal
cd /d "%~dp0"

if not exist src\rk05image.h (
    python tools\mkimage.py ..\src_original\advent-work.rk05 src\rk05image.h || exit /b 1
)

gcc -O2 -Wall -Wextra -o adventure.exe src\pdp8.c -Isrc -static -lz || exit /b 1
echo built adventure.exe
