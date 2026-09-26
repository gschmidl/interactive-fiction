@echo off
rem Eric Roberts' Wellesley Adventure (newadv, 2010).  SAVE and RESTORE use
rem saves\newadv.sav.
if not exist "%~dp0saves" mkdir "%~dp0saves"
pushd "%~dp0saves"
"%~dp0newadv.exe" %*
popd
