@echo off
rem Rebuilds KingsMission.exe from the original lesson files and dataset.
set WORK=D:\tools\IFBackup\_PLATO_work
python "%WORK%\tools\mkgame.py" --title "The King's Mission Game (Kent A. Wendler, PLATO)" --out "%~dp0." --exe KingsMission.exe --lesson "%~dp0..\src_original\2tkm.words" --lesson "%~dp0..\src_original\2tkmb.words" --dataset "2tkmds=%~dp0..\src_original\2tkmds.dataset"
