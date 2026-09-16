@echo off
rem Quest for the Data General MV -- runs the server and one player.
rem ESC at the command prompt leaves the game and saves the character.
rem Delete the "save" folder to reset the world.
cd /d "%~dp0"
if not exist save mkdir save
aosvs32.exe -d data -s save data\QUEST.PR %*
