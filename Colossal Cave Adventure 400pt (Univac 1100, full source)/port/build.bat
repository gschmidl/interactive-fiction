@echo off
REM Build the Adventure port with Strawberry Perl's bundled MinGW gfortran.
REM Produces build\adventure.exe and copies the game database into place.

set GFORTRAN=C:\tools\strawberry\c\bin\gfortran.exe

if not exist "%GFORTRAN%" (
  echo gfortran not found at %GFORTRAN%
  echo Install Strawberry Perl, or edit this script to point at your own
  echo MinGW-w64 gfortran.
  exit /b 1
)

if not exist "%~dp0build" mkdir "%~dp0build"

pushd "%~dp0src"
"%GFORTRAN%" -std=legacy -ffixed-form -ffixed-line-length-none -fno-range-check -O2 -I. ^
  -o ..\build\adventure.exe ^
  main.f liqdark.f upperc.f bug.f ciao.f datime.f getcls.f getin.f hours.f motd.f move.f ^
  poof.f ran.f savefile.f shift.f speak.f start.f vocab.f wizard.f yes.f
set BUILD_RC=%ERRORLEVEL%
popd

if not %BUILD_RC%==0 (
  echo Build failed.
  exit /b %BUILD_RC%
)

copy /y "%~dp0..\DATA.TXT" "%~dp0build\ADV.DAT" >nul
echo Build OK: build\adventure.exe (with build\ADV.DAT alongside it)
