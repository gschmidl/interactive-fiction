@echo off
rem Adventure 448 (ITS).  -u, given here, lifts prime time (on weekdays
rem from 12:00 to 16:59 only wizards may play); run adv448.exe directly
rem for the original hours.  FT01.DAT is read from the current folder.
pushd "%~dp0"
adv448.exe -u %*
popd
