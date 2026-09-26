@echo off
rem ABENTEUER - the German Colossal Cave of a Harris VULCAN site (Gary
rem Palter's portable Adventure, HCSD 1977).  NEUSPIEL.DAT (the game as
rem the site had it) and ADV.DATA must be here; SICHR name writes
rem saves\name.SAV, and BRING name, as the first command, goes on with it.
rem The cave keeps the site's hours - weekdays 8 to 18 only wizards - and
rem a saved game must rest 90 minutes: pass -u to play at any time.
"%~dp0abenteuer.exe" %*
