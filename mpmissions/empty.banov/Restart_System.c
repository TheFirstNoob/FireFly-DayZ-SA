/*
	Автор оригинала: 123new aka Sania(ZoS) 
	Создано для проекта: Zone of Survival (ZoS)
	Версия оригинала: 1.04 (21.10.2020)
	Взято: https://s-platoon.ru/files/file/16-restart-system-with-autokick-sistema-restartov-s-avtokikom-igrokov/
	
	Обновление и упрощение: TFN (TheFirstNoob)
	- Полный рефакторинг: 813 → 155 строк
	- Убрана мёртвая логика (dynamic restart, uptime-чаты, tick_timer, рекурсивный парсинг времени)
	- Убраны private в теле функций (и так локальные)
	- Исправлен баг: RequestExit без параметра (требует int code)
	- Исправлен баг: diff <= 0 → diff < 0 (пропуск рестарта если ровно в момент времени)
	
	P.S. С новой политикой автора скрипт стал бесплатным и свободно распространяется.
	
	О Скрипте:
	Серверный скрипт Авторестарта с выводом сообщений в чат сервера,
	блокировкой входа, авто-киком игроков и выключением сервера.
	ВНИМАНИЕ: Скрипт посылает shutdown на сервер и ваш DayZ_Server64.exe будет закрыт.
	Автозапуска НЕ БУДЕТ! Делайте это через планировщик или bat файлом через тот же loop.

	Инструкция: Добавить первой строчкой в init.c
	#include "$CurrentDir:mpmissions\empty.banov\Restart_System.c"
*/
#define RestartServer_System
class RestartServer_System
{
	// ─── Config ─────────────────────────────────────────────
	private ref array<string> m_RestartTimes = {"05:59:00", "11:59:00", "17:59:00", "23:59:00"};
	private ref array<int> m_WarnMinutes = {15, 10, 5, 4, 3, 2, 1};
	private int m_LockMinutesBefore = 3;
	private int m_KickSecondsBefore = 120;
	private int m_ShutdownWarnSeconds = 30;

	// ─── State ──────────────────────────────────────────────
	private bool m_ServerLocked;

	void RestartServer_System()
	{
		m_ServerLocked = false;
	}

	void Init()
	{
		float ms = GetMsToNextRestart();
		float shutdownMs = ms - m_ShutdownWarnSeconds * 1000;
		float kickMs = ms - m_KickSecondsBefore * 1000;
		float lockMs = ms - m_LockMinutesBefore * 60000;

		if (lockMs > 0)
			g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(this.LockServer, lockMs, false);
		if (kickMs > 0)
			g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(this.KickAll, kickMs, false);
		if (shutdownMs > 0)
			g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(this.ShutdownNotify, shutdownMs, false);

		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(this.DoShutdown, ms, false);

		// Schedule warnings
		foreach (int min : m_WarnMinutes)
		{
			float t = ms - min * 60000;
			if (t > 0)
				g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(this.WarnRestart, t, false, min);
		}

		Print("[RestartSystem] Next restart in " + (ms / 1000 / 60).ToString() + " minutes");
	}

	private float GetMsToNextRestart()
	{
		int h, m, s;
		GetHourMinuteSecond(h, m, s);
		int nowSec = h * 3600 + m * 60 + s;

		float nearestMs = 86400000; // 24h
		foreach (string time : m_RestartTimes)
		{
			array<string> parts = new array<string>();
			time.Split(":", parts);
			int rh = parts[0].ToInt();
			int rm = parts[1].ToInt();
			int rs = 0;
			if (parts.Count() > 2)
				rs = parts[2].ToInt();
			int restartSec = rh * 3600 + rm * 60 + rs;
			int diff = restartSec - nowSec;
			if (diff < 0)
				diff += 86400;
			float ms = diff * 1000;
			if (ms < nearestMs)
				nearestMs = ms;
		}
		return nearestMs;
	}

	private void WarnRestart(int minutes)
	{
		string msg = "ВНИМАНИЕ: Рестарт сервера через '" + minutes.ToString() + "' минут(ы)!";
		SendGlobalChat(msg);
		Print("[RestartSystem] " + msg);
	}

	private void LockServer()
	{
		m_ServerLocked = true;
		string msg = "ВНИМАНИЕ: Выполнена блокировка входа на сервер перед рестартом!";
		SendGlobalChat(msg);
		Print("[RestartSystem] " + msg);
	}

	private void KickAll()
	{
		string msg = "ВНИМАНИЕ: Отключение сервера на рестарт, все игроки будут исключены через '" + m_KickSecondsBefore.ToString() + "' секунд!";
		SendGlobalChat(msg);
		Print("[RestartSystem] " + msg);

		array<Man> players = new array<Man>();
		g_Game.GetPlayers(players);
		foreach (Man man : players)
		{
			PlayerBase player;
			if (Class.CastTo(player, man))
				g_Game.GetMission().OnEvent(ClientDisconnectedEventTypeID, new ClientDisconnectedEventParams(player.GetIdentity(), player, 0, false));
		}
	}

	private void ShutdownNotify()
	{
		string msg = "ВНИМАНИЕ: Сервер будет выключен через '" + m_ShutdownWarnSeconds.ToString() + "' секунд!";
		SendGlobalChat(msg);
		Print("[RestartSystem] " + msg);
	}

	private void DoShutdown()
	{
		Print("[RestartSystem] Shutting down server!");
		g_Game.RequestExit(0);
	}

	private void SendGlobalChat(string message)
	{
		g_Game.RPCSingleParam(null, ERPCs.RPC_USER_ACTION_MESSAGE, new Param1<string>(message), true);
	}

	bool IsServerLocked()
	{
		return m_ServerLocked;
	}
}

static ref RestartServer_System g_RestartServer_System;
static ref RestartServer_System Get_RestartServer_System()
{
	if (!g_RestartServer_System)
		g_RestartServer_System = new RestartServer_System();
	return g_RestartServer_System;
}

modded class CustomMission
{
	override void OnInit()
	{
		super.OnInit();
		Get_RestartServer_System().Init();
	}

	override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
	{
		super.InvokeOnConnect(player, identity);
		if (Get_RestartServer_System().IsServerLocked())
			g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(KickPlayer, 1000, false, player);
	}

	private void KickPlayer(PlayerBase player)
	{
		if (player)
			g_Game.GetMission().OnEvent(ClientDisconnectedEventTypeID, new ClientDisconnectedEventParams(player.GetIdentity(), player, 0, false));
	}
}
