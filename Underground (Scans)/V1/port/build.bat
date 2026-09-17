@echo off
rem Build the UNDERGROUND V1 interpreter.
rem Needs gcc on PATH (Strawberry Perl ships a suitable MinGW-w64 one).
gcc -O2 -o underg.exe bplus.c -lm
if errorlevel 1 goto fail
echo Built underg.exe
goto :eof
:fail
echo Build FAILED
