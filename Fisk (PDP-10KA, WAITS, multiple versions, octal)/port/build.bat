@echo off
rem Build FisK for Windows.  Needs gcc (MinGW / Strawberry Perl's) and, only
rem if you want to regenerate src\image.c from the octal transfer files,
rem Python 3.
setlocal
cd /d "%~dp0"

if not exist src\image.c (
    echo regenerating src\image.c ...
    python tools\mkimage.py src\image.c ^
        "jfp=..\dump_original\rjb.jfp\fisk.dmp\775,..\dump_original\rjb.jfp\fisk.txt\637" ^
        "v31=..\dump_original\3.1\fisk.dmp\775,..\dump_original\rjb.1\fisk.txt\551" || exit /b 1
)

if not exist bin mkdir bin
gcc -O2 -Wall -o bin\fisk.exe src\cpu.c src\image.c src\main.c src\waits.c -lm
if errorlevel 1 exit /b 1
echo built bin\fisk.exe
