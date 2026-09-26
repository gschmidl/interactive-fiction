@echo off
rem  FUNADV, run from its original VAX/VMS image.
rem
rem  Options are passed through, so the terminal timing can be changed:
rem    play -baud 2400    slower line: longer pauses between lines
rem    play -baud 0       no pacing at all, output as fast as it comes
rem    play -flash 0      no screen flashes
rem    play -effects none the game's own flashes only, none of the added ones
rem    play -demo         just play the screen effects, then exit
cd /d "%~dp0"
vaxvms.exe -L lib -D . %* FUNADV.EXE
