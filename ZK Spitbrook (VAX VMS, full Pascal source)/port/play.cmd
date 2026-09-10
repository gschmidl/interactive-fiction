@echo off
rem  ZK: SPITBROOK Interactive Fiction, run from its original VAX/VMS image
rem  Needs an ANSI/VT100-capable console window (Windows Terminal is fine).
cd /d "%~dp0"
vaxvms.exe -L lib -D . ZK.EXE
