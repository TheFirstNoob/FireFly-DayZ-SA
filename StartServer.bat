@echo off
:start
set serverName=DayZ Server
set serverLocation="C:\Program Files (x86)\Steam\steamapps\common\DayZServer"
set serverPort=2302
set serverConfig=serverDZ.cfg
set modlist=@CF;@Dabs-Framework;@Community-Online-Tools;@Banov;@DayZ-Expansion-Licensed;@DayZ-Expansion-Bundle;@DayZ-Expansion-Animations;@InediaInfectedAI;@InediaStamina;@RaG-Immersive-Vehicles;@Tactical-Flava;@AmmoStackBullet;@RUSForma-vehicles;@Ambient-Animals-Pack;@Apokot-Fishing-Mod;@MuchStuffPack;@Talking-Bots;@A-6-Secure-Containers;@CannabisPlus;@DayZ-Mining-System;
set servermodlist=@DayZ-Dynamic-AI-Addon;
title %serverName% batch
cd "%serverLocation%"
echo (%time%) %serverName% started.
start "DayZ Server" /min "DayZServer_x64.exe" -high -config=%serverConfig% -port=%serverPort% -profiles=profiles -mod=%modlist% -servermod=%servermodlist% -doLogs -cpuCount=6