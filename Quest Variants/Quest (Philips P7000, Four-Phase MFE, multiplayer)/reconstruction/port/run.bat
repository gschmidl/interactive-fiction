@echo off
rem QUEST UNDER MFE: QUEST version 1, the multi-player cave game of a
rem Danish Philips P7000 (Four-Phase IV/90) site, on the site's own pack.
rem
rem   play                one player
rem   play --players=3    a game for three: run play in two more windows on
rem                       this computer and they join it.  With --lan players
rem                       on other computers join with: play --join=HOST:PORT
rem                       (the first window shows it; PORT may be left out
rem                       when it is 8000)
rem   play --easy         with the 'EASY' library (the site had the hard one)
rem   play --port=N       use port N instead of looking for one
rem
rem The port is the first free one from 8000 to 9000, skipping ports other
rem programs hold and ports Windows has reserved: the port finder from eXo's
rem launch_if.bat, taught about reserved ports.  A game already waiting for
rem players here in that range is joined instead.
rem
rem -u gives QUEST the site's password when it says the cave is closed (it
rem does not run on weekdays from 9 to 12 and from 13 to 17).  QHELP.txt is
rem QUEST's own manual; quest --help lists the keys.
setlocal
set "first=8000"
set "last=9000"
set "port="
set "chosen="
for %%A in (%*) do (
    if /i "%%~A"=="--port" set "chosen=1"
    if /i "%%~A"=="--join" set "chosen=1"
)
rem --port=N comes after ours and wins; --join=HOST defaults to the first port.
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
for /f "tokens=2 delims=," %%p in ('tasklist /fi "imagename eq quest.exe" /fo csv /nh 2^>nul') do set "pid_%%~p=1"
for /f "usebackq tokens=1,2" %%a in ("%nettmp%") do (
    set "used_%%a=1"
    if defined pid_%%b set "game_%%a=1"
)
call :FindGame
if not defined port call :FindFreePort
del "%nettmp%" "%restmp%" 2>nul
if defined port goto run
echo quest: no free port from %first% to %last%; try  run --port=N
pause
exit /b 1

:run
"%~dp0quest.exe" -u --port=%port% %*
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
