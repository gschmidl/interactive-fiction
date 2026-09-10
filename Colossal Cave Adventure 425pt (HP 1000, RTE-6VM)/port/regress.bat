@echo off
REM Replay each recorded walkthrough through the port and compare it with the
REM transcript the original produced on the emulated HP 1000.  The .simh.out
REM files in tests\ were captured from ADVENT.RUN running under SIMH off the
REM original RTE-6/VM disc image; see the README for how to recapture them.

setlocal
set PY=python
set RC=0

if not exist "%~dp0build\adventure.exe" (
  echo build\adventure.exe not found -- run build.bat first.
  exit /b 1
)

pushd "%~dp0build"
for %%W in (walk1 walk2) do (
  adventure.exe < "%~dp0tests\%%W.in" > "%~dp0tests\%%W.port.out" 2>&1
  echo|set /p="%%W: "
  %PY% "%~dp0tools\cmpout.py" "%~dp0tests\%%W.in" ^
       "%~dp0tests\%%W.simh.out" "%~dp0tests\%%W.port.out"
  if errorlevel 1 set RC=1
)
popd

if %RC%==0 (echo All walkthroughs match the original.) else (echo MISMATCH.)
exit /b %RC%
