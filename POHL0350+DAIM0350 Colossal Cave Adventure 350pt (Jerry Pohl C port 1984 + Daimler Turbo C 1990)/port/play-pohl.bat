@echo off
rem Jaeger/Pohl C Adventure (12 June 1984).  Saved games (SUSPEND) go to saves\pohl;
rem restore one with:  play-pohl -r
if not exist "%~dp0saves\pohl" mkdir "%~dp0saves\pohl"
pushd "%~dp0saves\pohl"
"%~dp0pohl\advent.exe" %*
popd
