@echo off
rem Rebuilds Avatar.exe from the original lesson files, namesets and dataset.
set "WORK=D:\tools\IFBackup\_PLATO_work"
set "SRC=%~dp0..\src_original"
python "%WORK%\tools\mkgame.py" --title "Avatar (Bruce Maggs, Andrew Shapira and David Sides, PLATO/CYBIS)" --out "%~dp0." --exe Avatar.exe ^
 --lesson "%SRC%\2avat.words" --lesson "%SRC%\2avatar.words" --lesson "%SRC%\2upstairs.words" ^
 --lesson "%SRC%\2darkmoor.words" --lesson "%SRC%\2avatuse.words" --lesson "%SRC%\2avatcom.words" ^
 --lesson "%SRC%\2avathelp.words" --lesson "%SRC%\2avatmisc.words" --lesson "%SRC%\2avatstat.words" ^
 --dataset "2avatnset=%SRC%\2avatnset.dataset" --dataset "2avatname=%SRC%\2avatname.dataset" ^
 --dataset "2avatgld=%SRC%\2avatgld.dataset" --dataset "2avatst=%SRC%\2avatst.dataset" ^
 --dataset "2avatmap=%SRC%\2avatmap.dataset" ^
 --access 40000 ^
 --status "W step  a d x turn  t stairs  f fight  c cast  i items  I info  SHIFT-BACK (Shift+F9) leave"
