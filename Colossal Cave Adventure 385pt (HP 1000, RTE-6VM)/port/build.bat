@echo off
REM Build the HP 1000 385-point Adventure port with Strawberry Perl's
REM bundled MinGW gfortran.  Produces build\adven.exe with the game
REM database #ADVZZ beside it.

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
"%GFORTRAN%" %FFLAGS% -c adv01.f adv03.f adv05.f advf4.f advx2.f advy2.f hprte.f
if errorlevel 1 goto :failed
"%GFORTRAN%" -o "%~dp0build\adven.exe" adv01.o adv03.o adv05.o advf4.o advx2.o advy2.o hprte.o
if errorlevel 1 goto :failed
popd

copy /y "%~dp0..\src_original\disc\#ADVZZ" "%~dp0build\" >nul

REM #ADVXX is the random-access message file the game builds from #ADVZZ on
REM its first run; drop any stale copy so a fresh build starts clean.
del /q "%~dp0build\#ADVXX" 2>nul

echo Build OK: build\adven.exe  (run it from the build\ folder)
exit /b 0

:failed
popd
echo Build failed.
exit /b 1
