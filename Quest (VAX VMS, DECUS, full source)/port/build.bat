@echo off
rem  Build QUEST for Windows.
rem
rem  Needs gfortran and gcc.  Strawberry Perl ships both; so do MSYS2 and
rem  TDM-GCC.  Put them on PATH, or set FC and CC below.
rem
rem  -fno-pad-source is NOT optional.  QUEST continues its message strings
rem  across source lines, and VMS text files are variable length records,
rem  so the literal resumes at the start of the continuation.  gfortran's
rem  default pads every fixed-form line out to the full line length first,
rem  which buries each continuation under sixty spaces.

setlocal
if "%FC%"=="" set FC=gfortran
if "%CC%"=="" set CC=gcc
cd /d "%~dp0"

set FFLAGS=-std=legacy -fno-pad-source -ffixed-line-length-132 -fdollar-ok -fno-automatic -fno-align-commons -O2 -w
set CFLAGS=-O2 -Wall -Wno-unused-result

if not exist build mkdir build
for %%f in (lib quest quest1 quest2 quest3 dndop vmsf) do (
    %FC% -c %FFLAGS% -Isrc -Jbuild -o build\%%f.o src\%%f.f || goto :fail
)
for %%f in (vmsrt keyed main) do (
    %CC% -c %CFLAGS% -Isrc -o build\%%f.o src\%%f.c || goto :fail
)
%FC% -o quest.exe build\*.o || goto :fail
echo Built %CD%\quest.exe
exit /b 0

:fail
echo BUILD FAILED
exit /b 1
