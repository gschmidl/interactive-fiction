@echo off
rem Adventure 350 as installed on a Prime 50-Series (PRIMOS FORTRAN, from the
rem SBD003 tape).  advcom.dat - the Prime's own initialised COMMON blocks -
rem and common, its database, must be here; build.sh puts them there.
"%~dp0advent.exe" %*
