@echo off
rem Dungeon V3.0a (Tom Hutchinson, HP 1000 RTE, 18 Nov 82).  The data base
rem (@DUNGN, @DUNGT, @DUNGI) must be beside dungeon.exe - build.sh makes
rem it; SAVE and RESTORE keep games in saves\ under the name you give.
"%~dp0dungeon.exe" %*
