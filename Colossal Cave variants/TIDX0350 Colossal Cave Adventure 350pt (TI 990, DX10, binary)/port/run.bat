@echo off
rem Adventure "thru a Cave" (TI 990, DX10 GAMES library).  -u keeps the cave
rem open at all hours and lets a suspended game resume at once; drop it for
rem the original rules.  Save files are made in the saves folder: answer YES
rem to "Will/did you save your game?" and give a name, e.g. cave.sav.
if not exist "%~dp0saves" mkdir "%~dp0saves"
pushd "%~dp0saves"
"%~dp0adventure.exe" -u %*
popd
