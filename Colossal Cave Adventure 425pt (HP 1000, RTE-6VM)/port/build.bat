@echo off
REM Build the HP 1000 "Adventure II" 2.2 port with Strawberry Perl's
REM bundled MinGW gfortran.  Produces build\adventure.exe, build\abuild.exe
REM and a freshly built build\ADVENTURE.DAT.

setlocal
set GFORTRAN=C:\tools\strawberry\c\bin\gfortran.exe

if not exist "%GFORTRAN%" (
  echo gfortran not found at %GFORTRAN%
  echo Install Strawberry Perl, or edit this script to point at your own
  echo MinGW-w64 gfortran.
  exit /b 1
)

set FFLAGS=-std=legacy -ffixed-form -ffixed-line-length-none ^
 -fd-lines-as-comments -fno-range-check -fallow-argument-mismatch -w -O2 -I.

if not exist "%~dp0build" mkdir "%~dp0build"

pushd "%~dp0src"
"%GFORTRAN%" %FFLAGS% -c advent.f ainit.f amain.f asub.f aiosub.f abuild.f hprte.f
if errorlevel 1 goto :failed
"%GFORTRAN%" -o "%~dp0build\adventure.exe" advent.o ainit.o amain.o asub.o aiosub.o hprte.o
if errorlevel 1 goto :failed
"%GFORTRAN%" -o "%~dp0build\abuild.exe" abuild.o asub.o aiosub.o hprte.o
if errorlevel 1 goto :failed
popd

copy /y "%~dp0..\src_original\disc\ADVENTURE.TXT" "%~dp0build\" >nul
copy /y "%~dp0..\src_original\disc\MESSAGE.TXT"   "%~dp0build\" >nul
copy /y "%~dp0..\src_original\disc\CHEAT.TXT"     "%~dp0build\" >nul

REM ABUILD compiles ADVENTURE.TXT into the binary data base the game reads.
REM It ends in MAGIC (maintenance) mode, which is guarded by a password
REM puzzle; feeding it no input fails that guard, which is what we want here
REM -- the data base is written either way.
pushd "%~dp0build"
del /q ADVENTURE.DAT 2>nul
abuild.exe <nul >abuild.log 2>&1
if not exist ADVENTURE.DAT (
  echo Data base build failed -- see build\abuild.log
  popd
  exit /b 1
)
popd

echo Build OK: build\adventure.exe  (run it from the build\ folder)
exit /b 0

:failed
popd
echo Build failed.
exit /b 1
