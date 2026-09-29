@echo off
rem ABENTEUER - the German Colossal Cave of a Harris VULCAN site (Gary
rem Palter's portable Adventure, HCSD 1977).  NEUSPIEL.DAT (the game as
rem the site had it) and ADV.DATA must be here; SICHR name writes
rem saves\name.SAV, and BRING name, as the first command, goes on with it.
rem -u, given here, lifts the site's hours (weekdays 8 to 18 only wizards)
rem and the 90-minute rest of a saved game; run abenteuer.exe directly
rem for the original rules.
"%~dp0abenteuer.exe" -u %*
