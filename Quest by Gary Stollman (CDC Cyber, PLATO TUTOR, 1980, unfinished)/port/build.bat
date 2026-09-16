@echo off
rem Rebuilds Quest.exe from the original lesson file.
set WORK=D:\tools\IFBackup\_PLATO_work
python "%WORK%\tools\mkgame.py" --title "Quest (Gary Stollman, PLATO 1980)" --out "%~dp0." --exe Quest.exe --lesson "%~dp0..\src_original\ciswork18.words"
