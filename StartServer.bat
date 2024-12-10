@echo off
:start
::Server name
set serverName=DayZ Test Server
::Server files location
set serverLocation="C:\Program Files (x86)\Steam\steamapps\common\DayZServer"
::Server Port
set serverPort=2303
::Server config
set serverConfig=serverDZ.cfg
::Sets title for terminal (DONT edit)
set modlist=@CF;@Dabs-Framework;@Community-Online-Tools;@BanovFrost;@DayZ-Expansion-Licensed;@DayZ-Expansion-Bundle;@DayZ-Expansion-Animations;@InediaInfectedAI;@InediaStamina;@RaG-Immersive-Vehicles;@Tactical-Flava;@RUSForma-vehicles;@MuchFramework;@MuchStuffPack;@A-6-Secure-Containers;@CannabisPlus;@DayZ-Mining-System;@FireFly-Main;
set servermodlist=@DayZ-Dynamic-AI-Addon;@Server-PVE-SYSTEM;
title %serverName% batch
::DayZServer location (DONT edit)
cd "%serverLocation%"
echo (%time%) %serverName% started.
::Launch parameters (edit end: -config=|-port=|-profiles=|-doLogs|-adminLog|-netLog|-freezeCheck|-filePatching|-BEpath=|-cpuCount=)
start "DayZ Server" /min "DayZServer_x64.exe" -high -config=%serverConfig% -port=%serverPort% -profiles=profiles -mod=%modlist% -servermod=%servermodlist% -doLogs -cpuCount=6 -limitFPS=200