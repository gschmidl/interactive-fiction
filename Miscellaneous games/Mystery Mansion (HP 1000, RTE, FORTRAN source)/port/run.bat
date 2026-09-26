@echo off
rem Mystery Mansion (Bill Wolpert, HP 1000 RTE, 23 July 81).  SUSPEND and
rem RESTORE keep games in saves\CRn\ (n is the cartridge number asked for).
rem --site plays it as on Wolpert's own machine: playing hours (-u passes
rem the security code), a name, a player log and comments.
"%~dp0mmm.exe" %*
