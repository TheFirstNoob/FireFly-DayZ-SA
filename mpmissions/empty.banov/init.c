#include "$CurrentDir:mpmissions\empty.banov\Restart_System.c"

void main()
{
	Hive ce = CreateHive();
	if ( ce )
		ce.InitOffline();

	int year, month, day, hour, minute;
	int reset_month = 2, reset_day = 1;
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

class CustomMission: MissionServer
{
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
		}
	}

	//! FireFly: when player reaches safe distance after injection, trigger the experiment result.
	override void Expansion_OnObjectiveCompleted(ExpansionQuest quest, int objectiveID)
	{
		int qid = quest.GetQuestConfig().GetID();

		switch (qid)
		{
			// 2041 — Control: nothing happens
			case 2041:
				ExpansionQuestModule.GetModuleInstance().DeleteQuestHolder(4001, ExpansionQuestNPCType.AI);
				break;

			// 2042 — Neutralizer: scream → explosion at the experiment zone
			case 2042:
				if (objectiveID == 20412) // Player reached the zone
				{
					SEffectManager.PlaySound("FireFly_ExperimentScream_SoundSet", "5 0 5"); // TODO: zone position
					g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SpawnExplosion, 5000, false, "5 0 5"); // TODO: zone position
					ExpansionQuestModule.GetModuleInstance().DeleteQuestHolder(4002, ExpansionQuestNPCType.AI);
				}
				break;

			// 2043 — Concentrate: monster scream → InfectedCamp spawns
			case 2043:
				if (objectiveID == 20432) // Player reached the zone
				{
					SEffectManager.PlaySound("FireFly_MonsterScream_SoundSet", "5 0 5"); // TODO: zone position
					// InfectedCamp objective (20434) spawns monster via quest system
					ExpansionQuestModule.GetModuleInstance().DeleteQuestHolder(4003, ExpansionQuestNPCType.AI);
				}
				break;
		}
	}

	override void Expansion_OnQuestCompletion(ExpansionQuest quest)
	{
		int qid = quest.GetQuestConfig().GetID();

		switch (qid)
		{
			case 2041: ExpansionQuestModule.GetModuleInstance().DeleteQuestHolder(4001, ExpansionQuestNPCType.AI); break;
			case 2042: ExpansionQuestModule.GetModuleInstance().DeleteQuestHolder(4002, ExpansionQuestNPCType.AI); break;
			case 2043: ExpansionQuestModule.GetModuleInstance().DeleteQuestHolder(4003, ExpansionQuestNPCType.AI); break;
		}
	}

	protected void SpawnLabTech(int holderID, string posStr, string oriStr)
	{
		ExpansionTempQuestHolder holder = new ExpansionTempQuestHolder(holderID, "ExpansionQuestNPCAIMaria", "Лаборант", "...");
		if (!holder)
			return;
		holder.SetNPCEmoteID(EmoteConstants.ID_EMOTE_WATCHING);
		holder.SetLoadoutName("NPC_Laborant");
		ExpansionTempQuestHolderPosition pos = new ExpansionTempQuestHolderPosition(posStr, oriStr);
		ExpansionQuestModule.GetModuleInstance().SpawnQuestHolder(holder, pos);
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
	