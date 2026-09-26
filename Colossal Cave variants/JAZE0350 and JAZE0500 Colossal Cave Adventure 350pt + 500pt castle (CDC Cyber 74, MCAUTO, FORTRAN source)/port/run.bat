@echo off
rem Adventure as it ran on the MCAUTO Cyber 74: answer BEG for the 350 point
rem cave or ADV for the 500 point cave with the castle.  databs1.txt,
rem databs2.txt and amaint.dat must be here; build.sh puts them there.
rem -u: no prime time, so a full game at any hour.
"%~dp0advent.exe" -u %*
