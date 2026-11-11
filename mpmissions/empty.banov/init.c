#include "$CurrentDir:mpmissions\empty.banov\Restart_System.c"

void main()
{
	Hive ce = CreateHive();
	if ( ce )
		ce.InitOffline();

	int year, month, day, hour, minute;
	//! ТЕСТ 2026-09-04: солнце Banov инвертировано (CfgWorlds latitude=+48, а все
	//! официальные карты DayZ используют ОТРИЦАТЕЛЬНУЮ: Chernarus -56, Enoch -54;
	//! минус = север). SetStarsObserverPosition(-48,18) возвращает астрономического
	//! наблюдателя в северное полушарие — лето снова длинный день.
	//! ЕСЛИ ТЕСТ НЕ ПРОЙДЁТ (солнце не изменится) — откатить обе строки:
	//! вернуть reset_month = 2 и удалить SetStarsObserverPosition.
	g_Game.GetWorld().SetStarsObserverPosition(-48.0, 18.0);
	int reset_month = 7, reset_day = 13;
	g_Game.GetWorld().GetDate(year, month, day, hour, minute);

	// Reset to target month/day if we've drifted outside the window
	if (month != reset_month || day != reset_day)
	{
		g_Game.GetWorld().SetDate(year, reset_month, reset_day, hour, minute);
	}
	
	bool removedTerjeBackdoor;
	g_Game.GameScript.CallFunction(GetPluginManager(), "UnregisterPlugin", removedTerjeBackdoor, "PluginTerjeCmdDebug");
	if (removedTerjeBackdoor)
	{
		Print("Removed Terje's backdoor plugin");
	}
}

// ===== ВРЕМЕННО (анализ лута): CSV спавнабельности всех предметов =====
// После рестарта забрать CSV из mpmissions\empty.banov\storage\log, затем УДАЛИТЬ эти строки.
class FireFlySpawnAnalyzeHelper
{
	void RunDelayed()
	{
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this, "Start", 60, false);
	}
	void Start()
	{
		bool ok = GetCEApi().SpawnAnalyze("*");
		Print(string.Format("[FireFly] SpawnAnalyze = %1", ok));
	}
}

#ifdef EXPANSIONMODQUESTS
//! FireFly W1: per-player caps + K1 stock bridge fallback.
//! NOTE: Expansion's AddItemsToNearbyMarketZone is a no-op for DELIVERY objectives
//! (ExpansionQuestObjectiveCollectionEventBase::AddItemsToMarketZone returns unless COLLECT),
//! so K1 queues its +3 food here; the native flag stays set for future Expansion fix.
class FireFly_W1CapsData
{
	int DayIndex = -1;
	int WeekIndex = -1;
	int AcceptedDay = 0;
	int AcceptedWeek = 0;
	int CashDay = 0;
	int CashWeek = 0;
	int RepDay = 0;
	int RepWeek = 0;
}

class FireFly_W1ServerCapsData
{
	int DayIndex = -1;
	int K1Completions = 0;
	int K1Food = 0;
}

class FireFly_W1Caps
{
	static const int MAX_ACCEPTED_DAY = 2;
	static const int MAX_ACCEPTED_WEEK = 6;
	static const int MAX_CASH_DAY = 10000;
	static const int MAX_CASH_WEEK = 50000;
	static const int MAX_REP_DAY = 100;
	static const int MAX_REP_WEEK = 300;
	static const int MAX_K1_COMPLETIONS_DAY = 3;
	static const int MAX_K1_FOOD_DAY = 9;
	static const int K1_QUEST_ID = 3500;
	static const int K1_FOOD_AMOUNT = 3;
	static const string K1_FOOD_CLASS = "CrabCan";
	static const string DATA_DIR = "$profile:FireFly_W1";

	static bool IsW1Quest(int id)
	{
		return id == 3500 || id == 3501 || id == 3502;
	}

	static int GetDayIndex()
	{
		int ts = CF_Date.Now(true).GetTimestamp();
		return (ts - 32400) / 86400;
	}

	static int GetWeekIndex()
	{
		return (GetDayIndex() + 3) / 7;
	}

	static string GetPath(string uid)
	{
		return DATA_DIR + "/caps_" + uid + ".json";
	}

	static void EnsureDir()
	{
		if (!FileExist(DATA_DIR))
			MakeDirectory(DATA_DIR);
	}

	static FireFly_W1CapsData Load(string uid)
	{
		EnsureDir();
		FireFly_W1CapsData data = new FireFly_W1CapsData();
		string path = GetPath(uid);
		if (FileExist(path))
			JsonFileLoader<FireFly_W1CapsData>.JsonLoadFile(path, data);
		Normalize(data);
		return data;
	}

	static void Save(string uid, FireFly_W1CapsData data)
	{
		EnsureDir();
		JsonFileLoader<FireFly_W1CapsData>.JsonSaveFile(GetPath(uid), data);
	}

	static void Normalize(FireFly_W1CapsData data)
	{
		int day = GetDayIndex();
		int week = GetWeekIndex();
		if (data.DayIndex != day)
		{
			data.DayIndex = day;
			data.AcceptedDay = 0;
			data.CashDay = 0;
			data.RepDay = 0;
		}
		if (data.WeekIndex != week)
		{
			data.WeekIndex = week;
			data.AcceptedWeek = 0;
			data.CashWeek = 0;
			data.RepWeek = 0;
		}
	}

	static int GetCashReward(ExpansionQuestConfig cfg)
	{
		int total = 0;
		auto rewards = cfg.GetRewards();
		if (!rewards)
			return 0;
		foreach (ExpansionQuestRewardConfig reward: rewards)
		{
			if (reward && reward.GetClassName() == "ExpansionBanknoteHryvnia")
				total += reward.GetAmount();
		}
		return total;
	}

	static bool CanStart(ExpansionQuestConfig cfg, PlayerIdentity identity)
	{
		if (!identity || !cfg)
			return true;
		int id = cfg.GetID();
		if (!IsW1Quest(id))
			return true;
		if (id == K1_QUEST_ID)
		{
			FireFly_W1ServerCapsData server = LoadServer();
			if (server.K1Completions >= MAX_K1_COMPLETIONS_DAY || server.K1Food + K1_FOOD_AMOUNT > MAX_K1_FOOD_DAY)
			{
				Notify(identity, "Конвой ушёл", "Дневной лимит военного конвоя на сервере исчерпан. Следующий конвой — после сброса 09:00 UTC.");
				return false;
			}
		}
		string uid = identity.GetId();
		FireFly_W1CapsData data = Load(uid);
		if (data.AcceptedDay >= MAX_ACCEPTED_DAY || data.AcceptedWeek >= MAX_ACCEPTED_WEEK)
		{
			Notify(identity, "Лимит заказов", "На сегодня/неделю лимит принятых заказов исчерпан. Возвращайтесь после сброса 09:00 UTC.");
			return false;
		}
		data.AcceptedDay++;
		data.AcceptedWeek++;
		Save(uid, data);
		return true;
	}

	static bool CanComplete(ExpansionQuest quest, PlayerIdentity identity)
	{
		if (!quest || !identity)
			return true;
		ExpansionQuestConfig cfg = quest.GetQuestConfig();
		if (!cfg || !IsW1Quest(cfg.GetID()))
			return true;
		string uid = identity.GetId();
		FireFly_W1CapsData data = Load(uid);
		int cash = GetCashReward(cfg);
		int rep = cfg.GetReputationReward();
		if (cash > 0 && (data.CashDay + cash > MAX_CASH_DAY || data.CashWeek + cash > MAX_CASH_WEEK))
		{
			Notify(identity, "Лимит награды", "Дневной/недельный лимит денежной награды исчерпан. Возвращайтесь после сброса.");
			return false;
		}
		if (rep > 0 && (data.RepDay + rep > MAX_REP_DAY || data.RepWeek + rep > MAX_REP_WEEK))
		{
			Notify(identity, "Лимит репутации", "Дневной/недельный лимит репутации исчерпан. Возвращайтесь после сброса.");
			return false;
		}
		if (cfg.GetID() == K1_QUEST_ID)
		{
			FireFly_W1ServerCapsData server = LoadServer();
			if (server.K1Completions >= MAX_K1_COMPLETIONS_DAY || server.K1Food + K1_FOOD_AMOUNT > MAX_K1_FOOD_DAY)
			{
				Notify(identity, "Конвой ушёл", "Сегодняшний конвой уже закрыт. Мешок останется у вас; следующий конвой — после сброса 09:00 UTC.");
				return false;
			}
		}
		return true;
	}

	static void RecordCompletion(ExpansionQuest quest)
	{
		if (!quest)
			return;
		ExpansionQuestConfig cfg = quest.GetQuestConfig();
		if (!cfg || !IsW1Quest(cfg.GetID()))
			return;
		string uid = quest.GetPlayerUID();
		if (uid == "")
			return;
		FireFly_W1CapsData data = Load(uid);
		int cash = GetCashReward(cfg);
		int rep = cfg.GetReputationReward();
		data.CashDay += cash;
		data.CashWeek += cash;
		data.RepDay += rep;
		data.RepWeek += rep;
		Save(uid, data);
		if (cfg.GetID() == K1_QUEST_ID)
		{
			FireFly_W1ServerCapsData server = LoadServer();
			server.K1Completions++;
			server.K1Food += K1_FOOD_AMOUNT;
			SaveServer(server);
			QueueK1Stock(quest);
		}
	}

	static string GetServerPath()
	{
		return DATA_DIR + "/server_caps.json";
	}

	static FireFly_W1ServerCapsData LoadServer()
	{
		EnsureDir();
		FireFly_W1ServerCapsData data = new FireFly_W1ServerCapsData();
		string path = GetServerPath();
		if (FileExist(path))
			JsonFileLoader<FireFly_W1ServerCapsData>.JsonLoadFile(path, data);
		int day = GetDayIndex();
		if (data.DayIndex != day)
		{
			data.DayIndex = day;
			data.K1Completions = 0;
			data.K1Food = 0;
		}
		return data;
	}

	static void SaveServer(FireFly_W1ServerCapsData data)
	{
		EnsureDir();
		JsonFileLoader<FireFly_W1ServerCapsData>.JsonSaveFile(GetServerPath(), data);
	}

	static void QueueK1Stock(ExpansionQuest quest)
	{
	#ifdef EXPANSIONMODMARKET
		PlayerBase player = quest.GetPlayer();
		if (!player)
			return;
		auto marketSettings = GetExpansionSettings().GetMarket();
		if (!marketSettings)
			return;
		ExpansionMarketTraderZone zone = marketSettings.GetTraderZoneByPosition(player.GetPosition());
		if (!zone)
			return;
		ExpansionQuestPersistentServerData serverData = ExpansionQuestModule.GetModuleInstance().GetServerData();
		if (!serverData)
			return;
		serverData.AddQuestMarketItem(zone.Position, K1_FOOD_CLASS, K1_FOOD_AMOUNT);
		serverData.Save();
		Print("[FireFly W1] K1 stock queued: " + K1_FOOD_CLASS + " x" + K1_FOOD_AMOUNT + " at " + zone.Position.ToString());
	#endif
	}

	static void Notify(PlayerIdentity identity, string title, string text)
	{
		if (!identity)
			return;
		ExpansionNotification(new StringLocaliser(title), new StringLocaliser(text), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create(identity);
	}
}
#endif

class CustomMission: MissionServer
{
	override void OnMissionStart()
	{
		super.OnMissionStart();
		// ВРЕМЕННО: анализ спавна лута (удалить после получения CSV)
		FireFlySpawnAnalyzeHelper helper = new FireFlySpawnAnalyzeHelper();
		helper.RunDelayed();
	}

	override PlayerBase CreateCharacter(PlayerIdentity identity, vector pos, ParamsReadContext ctx, string characterName)
	{
		Entity playerEnt;
		playerEnt = g_Game.CreatePlayer( identity, characterName, pos, 0, "NONE" );
		Class.CastTo( m_player, playerEnt );

		g_Game.SelectPlayer( identity, m_player );

		return m_player;
	}

	// Add resistance for cold for fresh players
	override void StartingEquipSetup(PlayerBase player, bool clothesChosen)
	{
		player.SetTemporaryResistanceToAgent(eAgents.INFLUENZA, 900);
	}

#ifdef EXPANSIONMODQUESTS
	override bool Expansion_CanStartQuest(ExpansionQuestConfig questConfig, PlayerIdentity identity)
	{
		if (!g_Game.IsServer())
			return true;
		if (!super.Expansion_CanStartQuest(questConfig, identity))
			return false;
		return FireFly_W1Caps.CanStart(questConfig, identity);
	}

	override bool Expansion_CanCompleteQuest(ExpansionQuest quest, PlayerIdentity identity)
	{
		if (!g_Game.IsServer())
			return true;
		if (!super.Expansion_CanCompleteQuest(quest, identity))
			return false;
		return FireFly_W1Caps.CanComplete(quest, identity);
	}

	// === EXPERIMENT QUESTS (2041/2042/2043) ===

	override void Expansion_OnQuestStart(ExpansionQuest quest)
	{
		int qid = quest.GetQuestConfig().GetID();

		switch (qid)
		{
			case 2041:
				if (!ExpansionQuestModule.GetModuleInstance().TempQuestHolderExists(4001))
					SpawnLabTech(4001, "5 0 5", "0 0 0"); // TODO: zone position
				break;
			case 2042:
				if (!ExpansionQuestModule.GetModuleInstance().TempQuestHolderExists(4002))
					SpawnLabTech(4002, "5 0 5", "0 0 0"); // TODO: zone position
				break;
			case 2043:
				if (!ExpansionQuestModule.GetModuleInstance().TempQuestHolderExists(4003))
					SpawnLabTech(4003, "5 0 5", "0 0 0"); // TODO: zone position
				break;
			case 107:
				//! Сюжет «Пропавший»: умирающий Тарас у реки (отдаёт журнал и листовку)
				if (!ExpansionQuestModule.GetModuleInstance().TempQuestHolderExists(4010))
					SpawnTaras();
				break;
			case 108:
				//! Сюжет «Голос в эфире»: хор фанатиков у вышки (модуль QuestScenes)
				FireFly_QuestAmbient.Spawn("108_tower", "FireFly_CultChant_SoundSet", "5 0 5"); // TODO: позиция = точка Travel 10802 (радиовышка)
				break;
		}
	}

	override void Expansion_OnQuestContinue(ExpansionQuest quest)
	{
		int qid = quest.GetQuestConfig().GetID();

		switch (qid)
		{
			case 107:
				//! После рестарта сервера темп-NPC не переживают выгрузку — восстанавливаем Тараса,
				//! если игрок продолжает квест 107
				if (!ExpansionQuestModule.GetModuleInstance().TempQuestHolderExists(4010))
					SpawnTaras();
				break;
			case 108:
				FireFly_QuestAmbient.Spawn("108_tower", "FireFly_CultChant_SoundSet", "5 0 5"); // TODO: позиция = точка Travel 10802 (радиовышка)
				break;
		}
	}

	override void Expansion_OnQuestCancel(ExpansionQuest quest)
	{
		int qid = quest.GetQuestConfig().GetID();

		switch (qid)
		{
			case 2041:
				if (!ExpansionQuestModule.GetModuleInstance().IsOtherQuestInstanceActive(2041))
					ExpansionQuestModule.GetModuleInstance().DeleteQuestHolder(4001, ExpansionQuestNPCType.AI);
				break;
			case 2042:
				if (!ExpansionQuestModule.GetModuleInstance().IsOtherQuestInstanceActive(2042))
					ExpansionQuestModule.GetModuleInstance().DeleteQuestHolder(4002, ExpansionQuestNPCType.AI);
				break;
			case 2043:
				if (!ExpansionQuestModule.GetModuleInstance().IsOtherQuestInstanceActive(2043))
					ExpansionQuestModule.GetModuleInstance().DeleteQuestHolder(4003, ExpansionQuestNPCType.AI);
				break;
			case 107:
				if (!ExpansionQuestModule.GetModuleInstance().IsOtherQuestInstanceActive(107))
					ExpansionQuestModule.GetModuleInstance().DeleteQuestHolder(4010, ExpansionQuestNPCType.AI);
				break;
			case 108:
				FireFly_QuestAmbient.Remove("108_tower", 108);
				break;
		}
	}

	//! FireFly: when all quest objectives are done, clean up the temp lab tech NPCs.
	//! NOTE: "Expansion_OnObjectiveCompleted(quest, objectiveID)" DOES NOT EXIST in Expansion.
	//! init.c hooks are per-quest only; per-objective events are not exposed to the mission.
	//! TODO: scream/explosion effects when the player enters the experiment zone
	//! (objectives 20412 / 20422 / 20432) must be moved into a FireFly script mod:
	//! modded class ExpansionQuestObjectiveTravelEvent with override SetReachedLocation().
	override void Expansion_OnQuestObjectivesComplete(ExpansionQuest quest)
	{
		int qid = quest.GetQuestConfig().GetID();

		switch (qid)
		{
			case 2041:
				if (!ExpansionQuestModule.GetModuleInstance().IsOtherQuestInstanceActive(2041))
					ExpansionQuestModule.GetModuleInstance().DeleteQuestHolder(4001, ExpansionQuestNPCType.AI);
				break;
			case 2042:
				if (!ExpansionQuestModule.GetModuleInstance().IsOtherQuestInstanceActive(2042))
					ExpansionQuestModule.GetModuleInstance().DeleteQuestHolder(4002, ExpansionQuestNPCType.AI);
				break;
			case 2043:
				if (!ExpansionQuestModule.GetModuleInstance().IsOtherQuestInstanceActive(2043))
					ExpansionQuestModule.GetModuleInstance().DeleteQuestHolder(4003, ExpansionQuestNPCType.AI);
				break;
			case 107:
				//! все цели выполнены — Тарас «умер»/его забрала охрана, убираем сцену
				if (!ExpansionQuestModule.GetModuleInstance().IsOtherQuestInstanceActive(107))
					ExpansionQuestModule.GetModuleInstance().DeleteQuestHolder(4010, ExpansionQuestNPCType.AI);
				break;
			case 108:
				FireFly_QuestAmbient.Remove("108_tower", 108);
				break;
		}
	}

	override void Expansion_OnQuestCompletion(ExpansionQuest quest)
	{
		int qid = quest.GetQuestConfig().GetID();
		FireFly_W1Caps.RecordCompletion(quest);

		switch (qid)
		{
			case 2041: ExpansionQuestModule.GetModuleInstance().DeleteQuestHolder(4001, ExpansionQuestNPCType.AI); break;
			case 2042: ExpansionQuestModule.GetModuleInstance().DeleteQuestHolder(4002, ExpansionQuestNPCType.AI); break;
			case 2043: ExpansionQuestModule.GetModuleInstance().DeleteQuestHolder(4003, ExpansionQuestNPCType.AI); break;
			case 107: ExpansionQuestModule.GetModuleInstance().DeleteQuestHolder(4010, ExpansionQuestNPCType.AI); break;
			case 108: FireFly_QuestAmbient.Remove("108_tower", 108); break;
		}
	}

	protected void SpawnLabTech(int holderID, vector pos, vector ori)
	{
		ExpansionTempQuestHolder holder = new ExpansionTempQuestHolder(holderID, "ExpansionQuestNPCAIMaria", "Лаборант", "...");
		if (!holder)
			return;
		holder.SetNPCEmoteID(EmoteConstants.ID_EMOTE_WATCHING);
		holder.SetLoadoutName("NPC_Laborant");
		ExpansionTempQuestHolderPosition holderPos = new ExpansionTempQuestHolderPosition(pos, ori);
		ExpansionQuestModule.GetModuleInstance().SpawnQuestHolder(holder, holderPos);
	}

	protected void SpawnTaras()
	{
		//! Умирающий разведчик Тарас (квест 107): лежит у реки, в диалоге отдаёт
		//! журнал и листовку. Держать позицию синхронизированной с obj_10703_river.json!
		ExpansionTempQuestHolder holder = new ExpansionTempQuestHolder(4010, "ExpansionQuestNPCAIBoris", "Тарас", "У воды лежит окровавленный мужчина. Он едва дышит.");
		if (!holder)
			return;
		holder.SetNPCEmoteID(EmoteConstants.ID_EMOTE_LYINGDOWN);
		holder.SetLoadoutName("Story_Taras");
		//! Позиция Тараса = точка Travel 10703 (obj_10703_river.json)
		ExpansionTempQuestHolderPosition holderPos = new ExpansionTempQuestHolderPosition("5027.94 188.302 1207.4", "0 0 0");
		ExpansionQuestModule.GetModuleInstance().SpawnQuestHolder(holder, holderPos);
	}

	protected void SpawnExplosion(vector pos)
	{
		g_Game.CreateObject("FireFly_ExperimentExplosion", pos);
	}
#endif
};

Mission CreateCustomMission(string path)
{
	return new CustomMission();
}	
	
