@echo off
rem Rebuilds Hobbits.exe from the original lesson file.
set WORK=D:\tools\IFBackup\_PLATO_work
python "%WORK%\tools\mkgame.py" --title "Hobbits (Michael Jackson, PLATO 1980)" --out "%~dp0." --exe Hobbits.exe --lesson "%~dp0..\src_original\ciswork29.words"
