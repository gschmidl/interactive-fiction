@echo off
rem Eric Roberts' Starter Adventure (240 points), the browser edition's compiled
rem program run by a C implementation of his SVM.  SAVE and RESTORE use
rem saves\newadv.txt.
if not exist "%~dp0saves" mkdir "%~dp0saves"
pushd "%~dp0saves"
"%~dp0starter.exe" %*
popd
