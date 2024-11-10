#include "$CurrentDir:mpmissions\empty.banov\Restart_System.c"

void main()
{
	Weather weather = g_Game.GetWeather();
 
    weather.MissionWeather(false);
 
    weather.GetRain().SetLimits( 0.0 , 0.0 );
    weather.GetOvercast().SetLimits( 0.0 , 0.0 );
    weather.GetFog().SetLimits( 0.0 , 0.01 );
 
    weather.GetOvercast().SetForecastChangeLimits( 0.0, 0.1 );
    weather.GetRain().SetForecastChangeLimits( 0.0, 0.06 );
    weather.GetFog().SetForecastChangeLimits( 0.0, 0.3 );
 
    weather.GetOvercast().SetForecastTimeLimits( 1800 , 1800 );
    weather.GetRain().SetForecastTimeLimits( 1 , 100 );
    weather.GetFog().SetForecastTimeLimits( 1800 , 1800 );
 
    weather.GetOvercast().Set( Math.RandomFloatInclusive(0.3, 0.4), 0, 0);
    weather.GetRain().Set( Math.RandomFloatInclusive(0.0, 0.1), 0, 0);
    weather.GetFog().Set( Math.RandomFloatInclusive(0.0, 0.1), 0, 0);
 
    weather.SetWindMaximumSpeed(25);
    weather.SetWindFunctionParams(0.1, 1.0, 50);
	
	//INIT ECONOMY--------------------------------------
	Hive ce = CreateHive();
	if ( ce )
		ce.InitOffline();

	//DATE RESET AFTER ECONOMY INIT-------------------------
	int year, month, day, hour, minute;
	int reset_month = 10, reset_day = 16;
	GetGame().GetWorld().GetDate(year, month, day, hour, minute);

	if ((month == reset_month) && (day < reset_day))
	{
		GetGame().GetWorld().SetDate(year, reset_month, reset_day, hour, minute);
	}
	else
	{
		if ((month == reset_month + 1) && (day > reset_day))
		{
			GetGame().GetWorld().SetDate(year, reset_month, reset_day, hour, minute);
		}
		else
		{
			if ((month < reset_month) || (month > reset_month + 1))
			{
				GetGame().GetWorld().SetDate(year, reset_month, reset_day, hour, minute);
			}
		}
	}
}

class CustomMission: MissionServer
{
	override PlayerBase CreateCharacter(PlayerIdentity identity, vector pos, ParamsReadContext ctx, string characterName)
	{
		Entity playerEnt;
		playerEnt = GetGame().CreatePlayer( identity, characterName, pos, 0, "NONE" );
		Class.CastTo( m_player, playerEnt );

		GetGame().SelectPlayer( identity, m_player );

		return m_player;
	}
};

Mission CreateCustomMission(string path)
{
	return new CustomMission();
}