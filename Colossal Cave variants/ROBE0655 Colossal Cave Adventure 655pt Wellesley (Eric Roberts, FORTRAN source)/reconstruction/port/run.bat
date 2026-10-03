@echo off
rem Eric Roberts' Wellesley Adventure (V6.2, 655 points): his FORTRAN source of
rem 3 March 2010, compiled.  SAVE and RESTORE use saves\newadv.sav.
if not exist "%~dp0saves" mkdir "%~dp0saves"
pushd "%~dp0saves"
"%~dp0newadv.exe" %*
popd
