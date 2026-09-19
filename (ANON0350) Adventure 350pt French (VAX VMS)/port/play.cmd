@echo off
rem  Aventure - the French Adventure, run from its original VAX/VMS image
cd /d "%~dp0"
vaxvms.exe -L lib -D . ADVENT.EXE
