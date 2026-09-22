@echo off
rem Qork V3.0A - S. O. Lidie's CDC NOS Dungeon ("created 84/05/30").
rem qork.dat and qork.ini (build.sh makes them) must be here.  SAVE and
rem RESTORE use saves\QORK.SAV.  Every game rolls the Cyber's dice; pass
rem --seed N for others.  At the end of input (Ctrl-Z) the game says "I
rem cannot hear you!" and reads on.
"%~dp0qork.exe" %*
