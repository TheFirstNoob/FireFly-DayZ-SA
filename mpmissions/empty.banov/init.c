#include "$CurrentDir:mpmissions\empty.banov\Restart_System.c"

void main()
{
	Hive ce = CreateHive();
	if ( ce )
		ce.InitOffline();

	int year, month, day, hour, minute;
	int reset_month = 6, reset_day = 14;
	g_Game.GetWorld().GetDate(year, month, day, hour, minute);

	if ((month == reset_month) && (day < reset_day))
	{
		g_Game.GetWorld().SetDate(year, reset_month, reset_day, hour, minute);
	}
	else
	{
		if ((month == reset_month + 1) && (day > reset_day))
		{
			g_Game.GetWorld().SetDate(year, reset_month, reset_day, hour, minute);
		}
		else
		{
			if ((month < reset_month) || (month > reset_month + 1))
			{
				g_Game.GetWorld().SetDate(year, reset_month, reset_day, hour, minute);
			}
		}
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
};

Mission CreateCustomMission(string path)
{
	return new CustomMission();
}	
	