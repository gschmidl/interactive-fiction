@echo off
rem Mystery Mansion revision 9: the disc image from the tape, the SIMH
rem command file, and SIMH's hp2100.exe (the one HP2100 names, else PATH's).
setlocal
cd /d "%~dp0"
if not exist .build mkdir .build
python src\mkdisc.py .build\disc0.img || exit /b 1
copy /y src\rte.sim .build\rte.sim >nul
if "%HP2100%"=="" for %%p in (hp2100.exe) do set "HP2100=%%~$PATH:p"
if not exist .build\hp2100.exe if not "%HP2100%"=="" copy /y "%HP2100%" .build\hp2100.exe >nul
if not exist .build\hp2100.exe (
  echo build: no hp2100.exe - set HP2100 to SIMH's hp2100.exe ^(V3.12^), or put it on PATH, and run again
  exit /b 1
)
echo Built .build\disc0.img; play.bat starts the game.
