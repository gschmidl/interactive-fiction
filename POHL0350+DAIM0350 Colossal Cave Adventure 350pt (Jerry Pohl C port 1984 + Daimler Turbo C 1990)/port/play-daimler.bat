@echo off
rem Daimler's Turbo C 2.0 Adventure (June 1990).  Saved games (SUSPEND) go to
rem saves\daimler; restore one with:  play-daimler -r
if not exist "%~dp0saves\daimler" mkdir "%~dp0saves\daimler"
pushd "%~dp0saves\daimler"
"%~dp0daimler\advent.exe" %*
popd
