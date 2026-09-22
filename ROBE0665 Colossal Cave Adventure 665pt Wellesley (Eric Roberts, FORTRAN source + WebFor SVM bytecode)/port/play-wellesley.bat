@echo off
rem The browser edition of the Wellesley Adventure (V6.4.2, 665 points): the
rem compiled Big.js run by a C implementation of Roberts' SVM.  SAVE and RESTORE
rem use saves\wellesley\newadv.txt.
if not exist "%~dp0saves\wellesley" mkdir "%~dp0saves\wellesley"
pushd "%~dp0saves\wellesley"
"%~dp0wellesley.exe" %*
popd
