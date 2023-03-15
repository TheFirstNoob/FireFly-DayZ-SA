@echo off
:start
::Server name
set serverName=DayZ Test Server
::Server files location
set serverLocation="C:\Program Files (x86)\Steam\steamapps\common\DayZServer"
::Server Port
set serverPort=2302
::Server config
set serverConfig=serverDZ.cfg
::Logical CPU cores to use (Equal or less than available)
set serverCPU=4
::Sets title for terminal (DONT edit)
set modlist=@CF;@Dabs-Framework;@Community-Online-Tools;@Banov;@DayZ-Expansion-Core;@DayZ-Expansion-Licensed;@DayZ-Expansion-Bundle;@DayZ-Expansion-AI;
title %serverName% batch
::DayZServer location (DONT edit)
cd "%serverLocation%"
echo (%time%) %serverName% started.
::Launch parameters (edit end: -config=|-port=|-profiles=|-doLogs|-adminLog|-netLog|-freezeCheck|-filePatching|-BEpath=|-cpuCount=)
start "DayZ Server" /min "DayZServer_x64.exe" -config=%serverConfig% -port=%serverPort% -profiles=profiles -cpuCount=%serverCPU% -dologs -adminlog -netlog -freezecheck -mod=%modlist%
::Time in seconds before kill server process (14400 = 4 hours)
timeout 50000
taskkill /im DayZServer_x64.exe /F
::Time in seconds to wait before..
timeout 10
::Go back to the top and repeat the whole cycle again
goto start