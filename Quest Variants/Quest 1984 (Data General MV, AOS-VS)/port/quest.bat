@echo off
rem Quest (1984) for the Data General MV -- the multiplayer game it was.
rem Each player's terminal gets the CASTLE title QUEST.CLI typed first; any
rem key while it draws skips it.
rem
rem   quest               start the world and play.  If the world is already
rem                       running on this computer, join it instead: every
rem                       window you start quest in is another player.
rem   quest --no-title    leave the CASTLE title out (QUEST/S did the same)
rem   quest --lan         the same, and players on other computers may join
rem   quest --join HOST   play in the world running on computer HOST
rem   quest --server      run the world with nobody playing at this window
rem   quest --god         play at full strength and never die (for testing);
rem                       with --server, everyone who joins does
rem   quest --help        every option
rem
rem ESC leaves the game and saves the character.  The window that started
rem the world keeps it running until everyone has left; closing that window
rem (or Ctrl-C) saves everyone and ends the game.  Delete the "save" folder
rem to start the world afresh.
cd /d "%~dp0"
if not exist save mkdir save
aosvs32.exe --port 4084 --title data\CASTLE -d data -s save %* data\QUEST.PR
