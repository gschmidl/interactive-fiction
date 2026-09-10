@echo off
rem Build WANG 928 ADVENTURE for Windows. Needs gcc (MinGW-w64) on PATH;
rem Strawberry Perl's bundled compiler works: C:\Strawberry\c\bin.

setlocal
set CFLAGS=-O2 -Wall -Wextra -std=c99

echo Building wangadv.exe ...
gcc %CFLAGS% -o wangadv.exe src\wang.c src\z80.c src\con_win.c || goto :fail

echo Building wangtest.exe ...
gcc %CFLAGS% -o wangtest.exe src\wang.c src\z80.c src\con_test.c || goto :fail

echo.
echo Done.  Run wangadv.exe to play.
exit /b 0

:fail
echo.
echo Build failed.
exit /b 1
