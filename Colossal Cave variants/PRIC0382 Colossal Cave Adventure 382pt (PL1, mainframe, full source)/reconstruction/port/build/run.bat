@echo off
rem Adventure, PL/1 Version 4.0 (Greg Price).  -u, given here, lets a
rem suspended game be restored at once instead of an hour later; run
rem adventure.exe directly for the wait.  OBJECT and STORAGE are used in
rem the current folder.
pushd "%~dp0"
adventure.exe -u %*
popd
