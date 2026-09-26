@echo off
rem ADVENT UNDER IDOS: the 350-point Colossal Cave Adventure of a Danish
rem Philips P7000 (Four-Phase System IV) site, run on the site's own disc pack.
rem -u makes HOURS show the cave open all day and SUSPEND ask for no wait
rem (the game never enforced either); drop it to see the site's settings.
rem The working copy of the pack, with a suspended game, is kept in the
rem saves folder.  SUSPEND, and the next start goes on with that game.
if not exist "%~dp0saves" mkdir "%~dp0saves"
"%~dp0advent.exe" -u "--pack=%~dp0saves\advent.pack" %*
