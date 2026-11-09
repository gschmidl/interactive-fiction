@echo off
rem Quest for the Data General MV -- the multiplayer game it was.
rem
rem   quest               start the world and play.  If the world is already
rem                       running on this computer, join it instead: every
rem                       window you start quest in is another player.
rem   quest --lan         the same, and players on other computers may join
rem   quest --join HOST   play in the world running on computer HOST
rem                       (HOST:PORT unless it is on the first port, 8000)
rem   quest --server      run the world with nobody playing at this window
rem   quest --port N      use port N instead of looking for one
rem   quest --god         play at full strength and never die (for testing);
rem                       with --server, everyone who joins does
rem   quest --help        every option
rem
rem The port is the first free one from 8000 to 9000, skipping ports other
rem programs hold and ports Windows has reserved: the port finder from eXo's
rem launch_if.bat, taught about reserved ports.  A world already running
rem here in that range is joined instead.
rem
rem ESC at the command prompt leaves the game and saves the character.
rem The window that started the world keeps it running until everyone has
rem left; closing that window (or Ctrl-C) saves everyone and ends the game.
rem Delete the "save" folder to reset the world.
setlocal
cd /d "%~dp0"
if not exist save mkdir save
set "first=8000"
set "last=9000"
set "port="
set "chosen="
for %%A in (%*) do (
    if /i "%%~A"=="--port" set "chosen=1"
    if /i "%%~A"=="--join" set "chosen=1"
)
rem --port N comes after ours and wins; --join HOST defaults to the first port.
if defined chosen set "port=%first%"
if defined chosen goto run
set "nettmp=%TEMP%\quest_net_%RANDOM%.txt"
set "restmp=%TEMP%\quest_res_%RANDOM%.txt"
rem Get-NetTCPConnection, not netstat: netstat leaves out the game's listening
rem socket (Get-NetTCPConnection gives it state 0).  Without PowerShell the
rem list is empty, and the game itself still joins a game on the first port.
powershell -NoProfile -NonInteractive -Command "Get-NetTCPConnection -ErrorAction Stop | ForEach-Object { '{0} {1}' -f $_.LocalPort, $_.OwningProcess }" > "%nettmp%" 2>nul
netsh interface ipv4 show excludedportrange protocol=tcp | findstr /r /c:"^ *[0-9]" > "%restmp%"
rem used_PORT: someone holds PORT.  game_PORT: one of our games holds it.
for /f "tokens=2 delims=," %%p in ('tasklist /fi "imagename eq aosvs32.exe" /fo csv /nh 2^>nul') do set "pid_%%~p=1"
for /f "usebackq tokens=1,2" %%a in ("%nettmp%") do (
    set "used_%%a=1"
    if defined pid_%%b set "game_%%a=1"
)
call :FindGame
if not defined port call :FindFreePort
del "%nettmp%" "%restmp%" 2>nul
if defined port goto run
echo quest: no free port from %first% to %last%; try  quest --port N
pause
exit /b 1

:run
aosvs32.exe --port %port% -d data -s save %* data\QUEST.PR
exit /b

:FindGame
rem A game already running here.  It took the first free port, so look in order.
for /l %%P in (%first%,1,%last%) do if defined game_%%P (
    set "port=%%P"
    exit /b 0
)
exit /b 0

:FindFreePort
rem launch_if.bat's :FindFreePort, which asks only netstat; Windows-reserved
rem ports never show up there, so they are skipped here too.
for /l %%P in (%first%,1,%last%) do if not defined used_%%P (
    call :Reserved %%P || (
        set "port=%%P"
        exit /b 0
    )
)
exit /b 0

:Reserved
rem Succeeds if port %1 is inside one of Windows' excluded port ranges.
for /f "usebackq tokens=1,2" %%a in ("%restmp%") do (
    if %1 geq %%a if %1 leq %%b exit /b 0
)
exit /b 1
