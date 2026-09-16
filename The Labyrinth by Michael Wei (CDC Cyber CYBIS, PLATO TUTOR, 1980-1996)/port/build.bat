@echo off
rem Rebuilds both Labyrinth programs from the original lesson files.
set WORK=D:\tools\IFBackup\_PLATO_work
set SRC=%~dp0..\src_original
python "%WORK%\tools\mkgame.py" --title "The Labyrinth (Michael Wei, PLATO)" --out "%~dp0." --exe Labyrinth.exe --lesson "%SRC%\labyrinth.words" --lesson "%SRC%\exp.words" --lesson "%SRC%\khazaddum.words" --lesson "%SRC%\operate.words" --lesson "%SRC%\helpless.words"
if errorlevel 1 exit /b 1
python "%WORK%\tools\mkgame.py" --title "The Labyrinth - old version (Michael Wei, PLATO)" --out "%~dp0old" --exe LabyrinthOld.exe --lesson "%SRC%\olabyrinth.words" --lesson "%SRC%\odungeon.words" --lesson "%SRC%\fenhollen.words" --lesson "%SRC%\hyborea.words" --lesson "%SRC%\helpless.words" --lesson "%SRC%\operate.words"
