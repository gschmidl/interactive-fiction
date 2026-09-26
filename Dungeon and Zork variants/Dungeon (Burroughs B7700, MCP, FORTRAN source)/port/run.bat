@echo off
rem Dungeon V2.0 as a Burroughs B7700 ran it.  The data base (ZORK_DBTXT,
rem ZORK_PTXT, ZORK_PINDX) must be beside dungeon.exe - build.sh makes it;
rem SAVE and RESTORE keep the game in saves\ZORK_SAVDATA.
"%~dp0dungeon.exe" %*
