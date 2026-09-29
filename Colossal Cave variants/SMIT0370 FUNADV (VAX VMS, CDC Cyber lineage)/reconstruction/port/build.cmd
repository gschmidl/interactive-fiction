@echo off
setlocal
rem gcc must be on PATH (any MinGW-w64 build).
cd /d "%~dp0engine"
gcc -O2 -Wall -Wno-misleading-indentation -Wno-implicit-fallthrough -Wno-format-truncation -Wno-unused-function -o ..\vaxvms.exe main.c mem.c cpu.c image.c vms.c dis.c -lm
