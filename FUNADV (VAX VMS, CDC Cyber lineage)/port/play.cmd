@echo off
rem  FUNADV, run from its original VAX/VMS image
cd /d "%~dp0"
vaxvms.exe -L lib -D . FUNADV.EXE
