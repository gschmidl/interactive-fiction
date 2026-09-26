@echo off
rem Jonathan Reed's DUNGEON (Prime PL/I subset G, June 1983).
rem The game asks for a "wierd positive number" - that is the seed; the same
rem number gives the same cave as it did on the Prime.
"%~dp0dungeon.exe" %*
