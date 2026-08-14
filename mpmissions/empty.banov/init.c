#include "$CurrentDir:mpmissions\empty.banov\Restart_System.c"

void main()
{
	Hive ce = CreateHive();
	if ( ce )
		ce.InitOffline();

	int year, month, day, hour, minute;
	int reset_month = 2, reset_day = 13;
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
	