@echo off
setlocal
set PATH=C:\tools\strawberry\c\bin;%PATH%
cd /d "%~dp0engine"
gcc -O2 -Wall -Wno-misleading-indentation -Wno-implicit-fallthrough -Wno-format-truncation -Wno-unused-function -o ..\vaxvms.exe main.c mem.c cpu.c image.c vms.c dis.c -lm
