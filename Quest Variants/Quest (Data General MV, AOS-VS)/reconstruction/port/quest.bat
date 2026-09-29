@echo off
rem Quest for the Data General MV -- the multiplayer game it was.
rem
rem   quest               start the world and play.  If the world is already
rem                       running on this computer, join it instead: every
rem                       window you start quest in is another player.
rem   quest --lan         the same, and players on other computers may join
rem   quest --join HOST   play in the world running on computer HOST
rem   quest --server      run the world with nobody playing at this window
rem   quest --god         play at full strength and never die (for testing);
rem                       with --server, everyone who joins does
rem   quest --help        every option
rem
rem ESC at the command prompt leaves the game and saves the character.
rem The window that started the world keeps it running until everyone has
rem left; closing that window (or Ctrl-C) saves everyone and ends the game.
rem Delete the "save" folder to reset the world.
cd /d "%~dp0"
if not exist save mkdir save
aosvs32.exe --port 4040 -d data -s save %* data\QUEST.PR
