@echo off
rem Build ADVENTURE/3000 with gcc (MinGW) or cl.
where gcc >nul 2>&1 && (
    gcc -std=c99 -O2 -Wall -Wextra -o adventure3000.exe src\hp3kbasic.c -lm
) || (
    cl /nologo /O2 /Fe:adventure3000.exe src\hp3kbasic.c
)
