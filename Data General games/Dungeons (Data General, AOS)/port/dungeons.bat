@echo off
rem Dungeons -- what DG.CLI did on AOS.  "dungeons 3" starts a level 3
rem dungeon at once; "dungeons" shows the levels and asks, and for level 1
rem explains the one-key commands first.  The texts are DG.CLI's own.
setlocal
set "HERE=%~dp0"
set "LEVEL=%~1"
if not "%LEVEL%"=="" goto play

echo This is Rev. 4 of the Dungeon game.
echo There are four difficulty levels available:
echo    1 - Hacker
echo    2 - Experienced
echo    3 - Pro
echo    4 - Suicidal
echo.
echo To supress this display, give the difficulty level as an argument.
echo Example: DG 3  or DUNGEON 3
echo.
set /p "LEVEL=Which level do you wish to attempt? "
echo.
if not "%LEVEL%"=="1" goto play
echo Commands are read as the first letter of the desired command.
echo For example, to enter the command ATTACK, you type an A
echo without a 'NEW LINE' or 'CR'. The program reads each letter as
echo it is typed and finishes typing the rest of the command for you.
echo At any time you may type an H to get HELP.
echo.
set /p "WAIT=Type a 'NEW LINE' to begin playing. "

:play
"%HERE%aosvs16.exe" -d "%HERE%data" "%HERE%data\DG.PR" %LEVEL%
