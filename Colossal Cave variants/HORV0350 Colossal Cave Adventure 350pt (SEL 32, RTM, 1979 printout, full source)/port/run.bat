@echo off
rem Adventure 350 for the SEL 32 / RTM (Horvath and Norwood, 1978), from the
rem printout of 1979-03-21.  newgame2.sav and adv.line must exist: build.sh
rem makes them by letting the game set itself up out of adv.data.
rem -u: no prime time and no wait before a restored game.
"%~dp0advent.exe" -u %*
