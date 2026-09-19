@echo off
rem Play with the proof-read wording used by the Linux build.  The game
rem looks in the current directory before its own, so running it from
rem text-revised\ picks up that database.
cd /d "%~dp0text-revised"
"%~dp0adventure.exe"
