@echo off
rem QUEST UNDER MFE: QUEST version 1, the multi-player cave game of a
rem Danish Philips P7000 (Four-Phase IV/90) site, on the site's own pack.
rem
rem   play                one player
rem   play --players=3    a game for three: run play in two more windows on
rem                       this computer and they join it.  With --lan players
rem                       on other computers join with: play --join=HOST
rem   play --easy         with the 'EASY' library (the site had the hard one)
rem
rem -u gives QUEST the site's password when it says the cave is closed (it
rem does not run on weekdays from 9 to 12 and from 13 to 17).  QHELP.txt is
rem QUEST's own manual; quest --help lists the keys.
"%~dp0quest.exe" -u %*
