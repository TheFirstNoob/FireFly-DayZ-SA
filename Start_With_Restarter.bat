@echo off

chcp 65001
set process=DayZServer_x64.exe
echo Мониторинг процесса %process%

:loop
tasklist | find "%process%" > nul

if not %errorlevel% == 0 (
echo %process% не запущен, запускаем...
call StartServer.bat
)

timeout /t 60 > nul
goto :loop