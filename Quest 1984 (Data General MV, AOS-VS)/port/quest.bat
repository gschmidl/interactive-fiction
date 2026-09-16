@echo off
rem Quest (1984) for the Data General MV -- the server and one player, after
rem the CASTLE title QUEST.CLI typed first ("quest /s" skips it, as QUEST/S
rem did).  ESC leaves the game and saves the character.  Delete the "save"
rem folder to start the world afresh.
cd /d "%~dp0"
if not exist save mkdir save
if /i "%~1"=="/s" (
  aosvs32.exe -d data -s save data\QUEST.PR
) else (
  aosvs32.exe -c data\CASTLE -d data -s save data\QUEST.PR
)
