@echo off
setlocal
title Zwebti Mix
if not exist "%~dp0launch.ps1" goto incomplete
echo.
echo  ZWEBTI MIX
echo  Starting the Windows game launcher...
echo.
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -STA -File "%~dp0launch.ps1"
set "PLAY_EXIT=%ERRORLEVEL%"
if not "%PLAY_EXIT%"=="0" (
    echo.
    echo  Launch help: open START_HERE.html in this folder.
    pause
)
exit /b %PLAY_EXIT%

:incomplete
echo.
echo  Extract the ZIP before starting Zwebti Mix.
echo.
echo  Right-click the downloaded ZIP, choose Extract All, then Extract.
echo  Open the extracted folder and double-click PLAY.cmd there.
echo.
pause
exit /b 1
