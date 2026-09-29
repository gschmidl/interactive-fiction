@echo off
setlocal
set PATH=C:	ools\strawberry\cin;%PATH%
cd /d "%~dp0engine"
gcc -O2 -Wall -Wno-misleading-indentation -Wno-implicit-fallthrough -Wno-format-truncation -Wno-unused-function -o ..axvms.exe main.c mem.c cpu.c image.c vms.c dis.c -lm
