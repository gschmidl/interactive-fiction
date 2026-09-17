@echo off
rem Rebuilds Moria.exe from the original lesson files and the game's dataset.
rem The shared PLATO engine tree is not published; point PLATO_WORK at it.
set "WORK=%PLATO_WORK%"
if "%WORK%"=="" set "WORK=%~dp0..\..\_PLATO_work"
set "SRC=%~dp0..\src_original"
python "%WORK%\tools\mkgame.py" --title "Moria (Kevet Duncombe and Jim Battin, PLATO/CYBIS)" --out "%~dp0." --exe Moria.exe ^
 --lesson "%SRC%\0moria.words" --lesson "%SRC%\0moriag.words" --lesson "%SRC%\0moriad.words" ^
 --lesson "%SRC%\0moriah.words" --lesson "%SRC%\0moriac.words" ^
 --dataset "0moriads=%SRC%\0moriads.dataset" ^
 --status "w step  W door/run  a d turn  x about  c cast  i info  C camp  SHIFT-BACK (Shift+F9) save and leave"
