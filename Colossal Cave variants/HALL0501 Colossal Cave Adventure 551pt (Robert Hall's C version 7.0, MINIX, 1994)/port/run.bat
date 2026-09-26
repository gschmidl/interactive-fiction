@echo off
rem Robert R. Hall's Generic Adventure 7.0 (MINIX, 1994).  SAVE and RESTORE use
rem saves\advent.sav; "play advent.sav" restores it at start.
if not exist "%~dp0saves" mkdir "%~dp0saves"
pushd "%~dp0saves"
"%~dp0advent.exe" %*
popd
