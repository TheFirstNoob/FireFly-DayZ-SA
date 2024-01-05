/*
	Автор: 123new aka Sania(ZoS) 
	Создано для проекта: Zone of Survival (ZoS)
	Версия: 1.04 (21.10.2020) - TFN: Работает на 1.23
	Взято: https://s-platoon.ru/files/file/16-restart-system-with-autokick-sistema-restartov-s-avtokikom-igrokov/
	P.S. С новой политикой автора скрипт стал бесплатным и свободно распространяется, но больше не дорабатывается.
	Спрашивайте разрешение у автора на изменение кода (Перевод можете сделать сами если требуется).
	
	О Скрипте:
	Серверный скрипт Авторестарта с выводом сообщений в чат сервера о: Рестарте, Времени работы сервера, Открытии-Закрытии сервера, Авто-Кик игроков, Времени его блокировки и Выключение сервера!
	ВНИМАНИЕ TFN: Скрипт посылает shutdown на сервер и ваш DayZ_Server64.exe будет закрыт. Автозапуска НЕ БУДЕТ! Делайте это через планировщик или bat файлом через тот же loop.

	Инструкция: Добавить первой строчкой в init.c
	#include "$CurrentDir:mpmissions\dayzOffline.chernarusplus\Restart_System.c"
*/
#define RestartServer_System
class RestartServer_System
{
	// ------------------------------------------------------------
	// For enable script restart system
	
	string Name_Block_Log_Script = "[#RestartSystem] "; // метка в логах
	bool enable_work_script = true; // включение-отключение работы всего скрипта. При false все опции, что даны ниже, отключаются!
	// Рестарты сервера:
	bool enable_shutdown_server_procedures = true; // включение-отключение функции рестартов (при false отключится именно сам функционал рестартов)
	bool Enabled_Restarts_In_fixed_time = true; // переключение режима рестартов сервера (true - по фиксированному времени, false - по времени от старта сервера) (параметр 'enable_shutdown_server_procedures' не влияет на это зачение) (время используемое для кика игроков и блокировки сервера при 'true' используется фиксированное, при 'false' - динамическое)
	
	/*
			Для полноценной работы скрипта обязательно необходимо указать одно из 2 блоков ниже!
			От них зависит работоспособность функционала блокировки сервера и кика игроков, даже если "enable_shutdown_server_procedures = false;"
	*/
	// Блок 1:
	// Фиксированное время для рестартов (работает при Enabled_Restarts_In_fixed_time = true)
	ref array<string> Restarts_Fixed_Time = {"05:59:00","11:59:00","17:59:00","23:59:00"};  
	// Блок 2:
	// Динамическое время для рестартов сервера, берется от момента запуска сервера в указанном формате (работает при Enabled_Restarts_In_fixed_time = false)
	string time_format = "hours"; // Формат времени для динамических рестартов для значение ниже, может иметь только значения: 'seconds', 'minutes', 'hours'
	float time_wait_autorestart = 3.0; // Время для динамического рестарта, которое сервер будет отсчитывать от момента запуска сервера. Указывается в формате значения 'time_format' (например 3.2) (не рекоммендуется ставить более 4 часов)
	
	/*
			Опциональный функционал скрипта
	*/
	//Функционал, рабоающий только при включенной опции  enable_shutdown_server_procedures = true;
	int Time_Wait_AfterChatInformation_shutdown_server = 30; // в секундах, Указывается за столько секунд перед фактическим рестартом будет автоинформирование в чат сервера.
	string text_ChatInformation_shutdown_server = "ВНИМАНИЕ: Сервер будет выключен через '%time%' секунд!"; // сообщение автоинформирования для параметра выше
	bool enable_shutdown_server_chat_RestartInformation_functions = true;	// включение-отключение функционала уведомлений в чат перед скорым рестартом по указанному ниже времени
	ref array<int> RestartsInformation_chat_minutes = {15,10,5,4,3,2,1}; // время в минутах, за сколько будет отправлено каждое уведомление в чат о рестарте
	string text_RestartsInformation_chat = "ВНИМАНИЕ: Рестарт сервера через '%time%' минут(ы)!"; // само уведомление в чат
	
	// Функционал, отвечающий за фоновые уведомления в чат о времени до рестарта и времени работы сервера (uptime)
	bool enable_chat_info_restart_and_uptime = false; //включение-отключение информирования в чат через равные промежутки времени о времени работы сервера и времени, оставшегося до рестарта сервера  (true - вкл., false - откл.)
	string text_chat_info_restart_and_uptime = "[Info] Uptime '%uptime%' minutes, restart after '%timer%' minutes!"; // само сообщение, %timer% и %uptime% заменятся сами на время!
	//string text_chat_info_restart_and_uptime = "[Инфо] Uptime '%uptime%' минут(-ы), рестарт через '%uptime%' минут(-ы) !";
	string time_format_chat_info_restart_and_uptime = "minutes"; // Формат времени, какое будет выводиться в сообщении. Может иметь только значения: 'seconds', 'minutes', 'hours' 
	bool match_time_chat_info_restart_and_uptime = true; // включить-отключить округление времени в сообщении до целого числа (true - вкл., false - откл.)
	float time_chat_information_autorestart = 0.05; // Время частоты повтора отправки информации в чат.  Указывается в формате значения 'time_format_chat_info_restart_and_uptime' (например 0.20) (указывается в процентном соотношении к единице времени в 'time_format_chat_info_restart_and_uptime', т.е. 0.10 при 'hours' = 10% от 1 часа = 6 минут)

	// Функционал, отвечающий за блокировки входа на сервер переод рестартом
	bool enable_lock_server_in_restart = true; //включение-отключение блокировки входа на сервер перед рестартом  (true - вкл., false - откл.)
	string text_lock_server_in_restart = "ВНИМАНИЕ: Выполнена блокировка входа на сервер перед рестартом!"; // само сообщение в чат, отправляемое играющим, когда вход заблокирован.
	float time_undo_lock_server = 3.0; // в минутах - время в минутах, за сколько будет выполнена блокировка входа на сервер (перед фактическим рестартом сервера)
	
	// Функционал, отвечающий за кик всех игроков перед рестартом сервера
	bool enable_kick_all_from_server_in_restart = true; //включение-отключение кика всех игроков перед рестартом сервера  (true - вкл., false - откл.)
	int Time_kick_all_informationWait = 120; // в секундах - за столько секунд перед фактическим киком с сервера будет автоинформирование в чат
	string text_kick_all_from_server_in_restart = "ВНИМАНИЕ: Отключение сервера на рестарт, все игроки будут исключены с сервера через '%time%' секунд!"; // само сообщение, %time% заменится само на нужное время!
	float time_undo_kick_all_from_server_after_lock = 1.0; // в минутах - за столько минут будет запущен автокик всех игроков с сервера (перед фактическим рестартом сервера)
	
	
	// ------------------------------------------------------------
	// Don't Edit next settings!!!
	bool ApplyCustomActualTime = false; // enable the custom time for calculating script restart timers
	ref array <int> CustomActualTime = {15,05,35}; //hh,mm,ss
	bool EnableDebugLogs = false; // Enable the debug logs for restart timer
	float all_time_to_restart = 0.0;
	float tick_time_timer = 0.0;
	bool Server_closed = false;	
	// ------------------------------------------------------------
	void RestartServer_System()
	{
		Server_closed = false;
	}
	
	void Write_Log(string message) 
	{
		if(message)
		{
			WritePrintLog(message, Name_Block_Log_Script);
		}
	}
	void WriteDebug_Log(string message) 
	{
		if((EnableDebugLogs) && (message))
		{
			WritePrintLog(message, Name_Block_Log_Script + "[#DebugLog] ");
		}
	}
	void WritePrintLog(string text, string Name_Block_Log_Script)
	{
		if (text != "")
		{			
			#ifdef ZoS_scripts_enabled
				GetZoS_GlobalFunctions().WritePrintLog(text);
			#else
				private string day_log = GetRealDay();
				private string time_log = GetRealTime();	
				private string date_time_log = "[" + day_log + " - " + time_log + "] ";		
				PrintFormat(date_time_log + Name_Block_Log_Script + ": " + text);
			#endif
		}
	}
	string GetRealDay()
	{
		int year, month, day;	
		GetYearMonthDay(year, month, day);
		string date = day.ToStringLen(2) + "." + month.ToStringLen(2) + "." + year.ToStringLen(2);
		return date;
	}
	string GetRealTime()
	{
		int hour, minute, second;
		GetHourMinuteSecond(hour, minute, second);
		string time = hour.ToStringLen(2) + "." + minute.ToStringLen(2) + "." + second.ToStringLen(2);
		return time;
	}	
	// Functions For Use in scripts:		
	void fnc_SendMessage_GlobalChat(string message)
	{
		if (message != "")
		{
			private array<Man> players = new array<Man>; 
			GetGame().GetPlayers( players ); 
						
			for ( int i = 0; i < players.Count(); i++ ) 
			{ 
				if(players.Get(i))
				{
					fnc_SendMessage_PersonalInChat_player(message, players.Get(i));
				}
			} 
			players.Clear();
		}		
	}
	void fnc_SendMessage_PersonalInChat_player(string message, Man player)
	{
		if( message ) 
		{
			if( player ) 
			{
				Param1<string> m_GlobalMessage = new Param1<string>(message); 
				GetGame().RPCSingleParam(player, ERPCs.RPC_USER_ACTION_MESSAGE, m_GlobalMessage, true, player.GetIdentity()); 
			}
		}
	}	
	
	void Close_server() 
	{	
		private string text_chat = text_lock_server_in_restart;
		private string text_log = "[ChatSendingToAll] " + text_chat;
		fnc_SendMessage_GlobalChat(text_chat);
		Write_Log(text_log);
		text_log = "Server Command Runned: Server will be closed!";
		Server_closed = true;	
		Write_Log(text_log);
	}	
	void Open_server() 
	{	
		private string text_chat = "Server will be opened!";
		private string text_log = "[ChatSendingToAll] " + text_chat;
		fnc_SendMessage_GlobalChat(text_chat);
		Write_Log(text_log);
		text_log = "Server Command Runned: Server will be opened!";
		Server_closed = false;
		Write_Log(text_log);
	}
	
	void Kick_all_from_server_Run() 
	{		
		private string text_log = "Server Command Runned: Executing kick all players!";
		
		array<string> players_disconnected = new array<string>; 		
		array<Man> players = new array<Man>; 
		GetGame().GetPlayers( players ); 		
		if (players.Count() > 0)
		{
			players_disconnected.Insert("Will be disconnected list players:");
			for ( int i = 0; i < players.Count(); i++ ) 
			{ 
				PlayerBase player; 
				Class.CastTo(player, players.Get(i)); 
				if( player ) 
				{
					private PlayerIdentity identity = player.GetIdentity();
					if (identity)
					{
						private string Name_P = identity.GetName();
						private string UID_P = identity.GetPlainId();
						private string Game_UID_P = identity.GetId();
						private string Game_ID_P = identity.GetPlayerId().ToString();					
						private string TEMP_MESSage_log = "Name: " + Name_P + " UID: " + UID_P + " GAME_UID: " + Game_UID_P + "";
						players_disconnected.Insert(TEMP_MESSage_log);
						RunPlayerDisconnect(player);
					}
				}
			}
		}		
		Write_Log(text_log);
		if (players_disconnected.Count() > 0)
		{
			foreach( string players_disconnected_selected : players_disconnected )
			{
				Write_Log(players_disconnected_selected);
			}
		}
	}
	void Kick_all_from_server() 
	{
		private string sending_text_kick_all_from_server_in_restart = text_kick_all_from_server_in_restart;
		if(sending_text_kick_all_from_server_in_restart.Contains("%time%"))
		{
			sending_text_kick_all_from_server_in_restart.Replace("%time%", Time_kick_all_informationWait.ToString());
		}
		private string text_log = "[ChatSendingToAll] " + sending_text_kick_all_from_server_in_restart;
		fnc_SendMessage_GlobalChat(sending_text_kick_all_from_server_in_restart);
		Write_Log(text_log);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(this.Kick_all_from_server_Run, (1000 * Time_kick_all_informationWait), false);
	}
	
	void Shutdown_server() 
	{
		private string sending_text_ChatInformation_shutdown_server = text_ChatInformation_shutdown_server;
		if(sending_text_ChatInformation_shutdown_server.Contains("%time%"))
		{
			sending_text_ChatInformation_shutdown_server.Replace("%time%", Time_Wait_AfterChatInformation_shutdown_server.ToString());
		}
		fnc_SendMessage_GlobalChat(sending_text_ChatInformation_shutdown_server);
		private string text_log = "[ChatSendingToAll] " + sending_text_ChatInformation_shutdown_server;
		Write_Log(text_log);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(this.Shutdown_server_Run, (1000 * Time_Wait_AfterChatInformation_shutdown_server), false);
	}	
	void Shutdown_server_Run() 
	{
		private string text_log = "Server Command Runned: Executing Shutdown server!!!";
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(GetDayZGame().RequestExit, 1000, 0, false); 
	}
	
	void Run_Timer_AutoRestart_server() 
	{
		if(enable_work_script)
		{
			private string text_log = "Work script is Enabled! Script Timers will be Started!";
			Write_Log(text_log);		
		//	Timer myTimer1 = new Timer ( CALL_CATEGORY_SYSTEM );
		//	myTimer1.Run(time_wait_autorestart, Get_RestartServer_System(), "Shutdown_server", NULL, false); // calls "Refresh" on "this" every 10 seconds, until Pause or Stop is called
			int time_format_coef = 1000; //'seconds', 'minutes', 'hours'
			if (time_format == "minutes")
			{
				time_format_coef = time_format_coef * 60;
			}
			if (time_format == "hours")
			{
				time_format_coef = (time_format_coef * 60) * 60;
			}			
			if(!Enabled_Restarts_In_fixed_time)
			{			
				all_time_to_restart = GetTimeToDinamicRestart(time_wait_autorestart, time_format_coef);
				//all_time_to_restart = (time_wait_autorestart * time_format_coef);
			} else
			{				
				all_time_to_restart = GetTimeToFixedRestart();		
			}
			GetTimeRestartInText(all_time_to_restart);
			float all_time_to_kick_all = all_time_to_restart - ((60 * 1000) * time_undo_kick_all_from_server_after_lock);
			float all_time_to_lock_server = all_time_to_restart - ((60 * 1000) * time_undo_lock_server);
			if(enable_shutdown_server_procedures)
			{
				GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(this.Shutdown_server, (all_time_to_restart - (1000 * Time_Wait_AfterChatInformation_shutdown_server)), false);
			}
			if(enable_kick_all_from_server_in_restart)
			{
				GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(this.Kick_all_from_server, (all_time_to_kick_all - (1000 * Time_kick_all_informationWait)), false);				
			}
			if(enable_lock_server_in_restart)
			{
				GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(this.Close_server, all_time_to_lock_server, false);
			}
			if(enable_chat_info_restart_and_uptime)
			{
				GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(this.Run_InfoChat_UptimeAndRestartTime_server, (time_chat_information_autorestart * time_format_coef), true);
			}
			if((enable_shutdown_server_chat_RestartInformation_functions) && (enable_shutdown_server_procedures))
			{
				if(RestartsInformation_chat_minutes.Count() > 0)
				{
					foreach(private int Readed_minute:RestartsInformation_chat_minutes)
					{
						float time_sending_autorestart_info = (all_time_to_restart - (1000 * Time_Wait_AfterChatInformation_shutdown_server)) - (Readed_minute * 1000 * 60);
						GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(this.Run_InfoChat_AutoRestart_server, time_sending_autorestart_info, false, false, 0);
					}
				}
			}
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(this.Run_TickTimer, 1000, true);
			//GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(this.TimeChecking, 1000, true);
		} else
		{
			private string text_log_disabled = "Work script is disabled!";
			Write_Log(text_log_disabled);	
		}
	}	
	void Run_TickTimer() 
	{
		tick_time_timer++;
	}
	void GetActualTime(out int hour = 0,out int minute = 0, out int second = 0)
	{
		if((ApplyCustomActualTime) && (CustomActualTime.Count() == 3))
		{
			hour = CustomActualTime.Get(0); minute = CustomActualTime.Get(1); second = CustomActualTime.Get(2);
			Write_Log("[ActualTimeCorrect] Setuped the work time is:  " + hour.ToString() + ":" + minute.ToString()  + ":" + second.ToString());
		} else
		{
			GetHourMinuteSecond(hour, minute, second);
		}
	}
	void GetTimeRestartInText(float time_undo_restart)
	{	
		private int this_hour, this_minute, this_second;
		GetActualTime(this_hour, this_minute, this_second);
		
		private int time_format_coef_second = 1000; //'seconds'
		private int time_format_coef_minute = time_format_coef_second * 60; //'minute'
		private int time_format_coef_hour = time_format_coef_minute * 60; //'hour'
		
		private int HoursUndoRestart = Math.Floor( time_undo_restart / time_format_coef_hour);
		private int MinutesAndSecondsUndoRestart = (time_undo_restart - (HoursUndoRestart * time_format_coef_hour));
		private int MinutesUndoRestart = Math.Floor( MinutesAndSecondsUndoRestart / time_format_coef_minute);
		private int CalcSecondsUndoRestart = (MinutesAndSecondsUndoRestart - (MinutesUndoRestart * time_format_coef_minute));
		private int SecondsUndoRestart = Math.Floor( CalcSecondsUndoRestart / time_format_coef_second);
		if(SecondsUndoRestart >= 60)
		{
			while(SecondsUndoRestart >= 60)
			{
				MinutesUndoRestart = MinutesUndoRestart + 1;
				SecondsUndoRestart = SecondsUndoRestart - 60;
			}
		}
		if(MinutesUndoRestart >= 60)
		{
			while(MinutesUndoRestart >= 60)
			{
				HoursUndoRestart = HoursUndoRestart + 1;
				MinutesUndoRestart = MinutesUndoRestart - 60;
			}
		}		
		private int HoursNextRestart = this_hour + HoursUndoRestart;
		private int MinutesNextRestart = this_minute + MinutesUndoRestart;
		private int SecondsNextRestart = this_second + SecondsUndoRestart;
		if(SecondsNextRestart >= 60)
		{
			while(SecondsNextRestart >= 60)
			{
				MinutesNextRestart = MinutesNextRestart + 1;
				SecondsNextRestart = SecondsNextRestart - 60;
			}
		}
		if(MinutesNextRestart >= 60)
		{
			while(MinutesNextRestart >=  60)
			{
				HoursNextRestart = HoursNextRestart + 1;
				MinutesNextRestart = MinutesNextRestart - 60;
			}
		}		
		private string TxTHoursUndoRestart = "00"; private string TxTMinutesUndoRestart = "00"; private string TxTSecondsUndoRestart = "00"; 
		private string TxTHoursNextRestart = "00"; private string TxTMinutesNextRestart = "00"; private string TxTSecondsNextRestart = "00"; 
		private string Time_Undo_Restart = ""; private string Time_Next_Restart = "";
		if(HoursUndoRestart < 10) { TxTHoursUndoRestart = "0" + HoursUndoRestart.ToString(); } else { TxTHoursUndoRestart = HoursUndoRestart.ToString(); }
		if(MinutesUndoRestart < 10) { TxTMinutesUndoRestart = "0" + MinutesUndoRestart.ToString(); } else { TxTMinutesUndoRestart = MinutesUndoRestart.ToString(); }
		if(SecondsUndoRestart < 10) { TxTSecondsUndoRestart = "0" + SecondsUndoRestart.ToString(); } else { TxTSecondsUndoRestart = SecondsUndoRestart.ToString(); }
		if(HoursNextRestart < 10) { TxTHoursNextRestart = "0" + HoursNextRestart.ToString(); } else { TxTHoursNextRestart = HoursNextRestart.ToString(); }
		if(MinutesNextRestart < 10) { TxTMinutesNextRestart = "0" + MinutesNextRestart.ToString(); } else { TxTMinutesNextRestart = MinutesNextRestart.ToString(); }
		if(SecondsNextRestart < 10) { TxTSecondsNextRestart = "0" + SecondsNextRestart.ToString(); } else { TxTSecondsNextRestart = SecondsNextRestart.ToString(); }
		Time_Undo_Restart = TxTHoursUndoRestart + ":" + TxTMinutesUndoRestart + ":" + TxTSecondsUndoRestart;
		Time_Next_Restart = TxTHoursNextRestart + ":" + TxTMinutesNextRestart + ":" + TxTSecondsNextRestart;
		Write_Log("[TimeUndoRestart] Time Undo Restart: " + Time_Undo_Restart + "!");
		Write_Log("[TimeNextRestart] Next Restart at the time: " + Time_Next_Restart + "!");
	}
	float GetTimeToDinamicRestart(float time_wait_autorestart = 3.0, int time_format_coef = 1000)
	{
		Write_Log("[RestartSettings] Parameter 'Enabled_Restarts_In_fixed_time' = '" + Enabled_Restarts_In_fixed_time.ToString() + "'!");
		Write_Log("[TimeToDinamicRestart] Will be used a dinamik restart time from the start server with coeficient: '" + time_wait_autorestart.ToString() + "'!");		
		private float return_value = -1;
		return_value = (time_wait_autorestart * time_format_coef);
		WriteDebug_Log(" return_value " + return_value.ToString());
		return return_value;
	}	
	float GetTimeToFixedRestart()
	{
		private float return_value = -1;
		
		Write_Log("[RestartSettings] Parameter 'Enabled_Restarts_In_fixed_time' = '" + Enabled_Restarts_In_fixed_time.ToString() + "'!");
		Write_Log("[TimeToFixedRestart] Will be used a fixed restart time from the settings script!");
		
		private int hour, minute, second;
		//GetYearMonthDay(year, month, day);
		GetActualTime(hour, minute, second);
		
		private array<int> ActualTime = new array<int>; ActualTime.Resize(3);
		ActualTime.Set(0,hour);	ActualTime.Set(1,minute); ActualTime.Set(2,second);

		private array<int> RestartHours = new array<int>;
		private array<int> RestartMinutes = new array<int>;
		private array<int> RestartSeconds = new array<int>;
		foreach(private string Readed_time:Restarts_Fixed_Time)
		{
			private array <string> Time_Readed = new array <string>;
			if (Readed_time.Contains(":"))
			{
				Readed_time.Split(":", Time_Readed);
				if(Time_Readed.Count() == 3)
				{
					RestartHours.Insert(Time_Readed.Get(0).ToInt());
					RestartMinutes.Insert(Time_Readed.Get(1).ToInt());
					RestartSeconds.Insert(Time_Readed.Get(2).ToInt());
					Write_Log("[TimeToFixedRestart] Readed from settings script the fixed restart time is: '" + Readed_time + "'!");
				}
			}
		}
		if ((RestartMinutes.Count() == RestartSeconds.Count()) && (RestartHours.Count() == RestartMinutes.Count()) && (RestartMinutes.Count() > 0))
		{
			private map <int,int> Detect_time = new map <int,int>;
			
			WriteDebug_Log(" CALCULATING:  ");
			foreach(private int hour_restart:RestartHours)
			{
				Write_Log(" Detected fixed restart: " + hour_restart.ToString() + ":" + RestartMinutes.Get(RestartHours.Find(hour_restart)).ToString() + ":" + RestartSeconds.Get(RestartHours.Find(hour_restart)).ToString() + " ! Calсulate(Hour Restart - This Hour Time): " + hour_restart.ToString() + "-" + hour.ToString() + "=" + (hour_restart - hour).ToString());
				private int Result = hour_restart - hour;
				if(Result >= 24) { Result = 24 - Result;}
				if(Result < 0) { Result = 24 + Result;}
				Write_Log(" Result insert:" + hour_restart.ToString() + " - " + Result.ToString());
				Detect_time.Insert(hour_restart, Result);
			}
			// SAVE FORMAT MASIVE
			private array<int> RestartTimeDetect = new array<int>; RestartTimeDetect.Resize(6);
			// DEFAULT TIME
			private int RestartHourDetected = 0;
			private int HoursToRestart = 0;
			private int RestartMinuteDetected = 0;
			private int MinutesToRestart = 0;
			private int RestartSecundeDetected = 0;
			private int SecundesToRestart = 0;
			private int LastCount = 0;
			
			private int time_coef_second = 1000; //'seconds', 'minutes', 'hours'
			private int	time_coef_minute = time_coef_second * 60;
			private int time_coef_hour = time_coef_minute * 60;
			
			//HOURS DETECT			
			HoursToRestart = GetUndoRestart_Hour(Detect_time, LastCount, RestartHourDetected);
			WriteDebug_Log(" [HoursToRestart] RestartHourDetected: " + RestartHourDetected.ToString() + " HoursToRestart: " + HoursToRestart.ToString());
			//MINUTES DETECT
			RestartTimeDetect.Set(0,RestartHourDetected);
			RestartTimeDetect.Set(1,HoursToRestart);
			RestartTimeDetect.Set(2,RestartMinuteDetected);
			RestartTimeDetect.Set(3,0);
			RestartTimeDetect.Set(4,0);
			RestartTimeDetect.Set(5,0);
			WriteDebug_Log(" [HoursToRestart] RestartHourDetected: " + RestartTimeDetect.Get(0).ToString() + " HoursToRestart: " + RestartTimeDetect.Get(1).ToString());
			MinutesToRestart = GetUndoRestart_Minutes(RestartHours, RestartMinutes, RestartSeconds, ActualTime, Detect_time, LastCount, RestartTimeDetect);
			GetRestartTimeFromMassive(RestartTimeDetect, RestartHourDetected, HoursToRestart, RestartMinuteDetected, MinutesToRestart,RestartSecundeDetected, SecundesToRestart);
			
			//SECUNDES DETECT
			RestartTimeDetect.Set(4,RestartSecundeDetected);
			SecundesToRestart = GetUndoRestart_Secundes (RestartHours, RestartMinutes, RestartSeconds, ActualTime, Detect_time, LastCount, RestartTimeDetect);
			GetRestartTimeFromMassive(RestartTimeDetect, RestartHourDetected, HoursToRestart, RestartMinuteDetected, MinutesToRestart,RestartSecundeDetected, SecundesToRestart);
			
			Write_Log("[TimeToFixedRestart][CALCULATED] Optimal Restart Detected: " + RestartHourDetected.ToString() + ":" + RestartMinuteDetected.ToString() + ":" + RestartSecundeDetected.ToString() + " !  Undo restart is: " + HoursToRestart.ToString() + ":" + MinutesToRestart.ToString() + ":" + SecundesToRestart.ToString());
			
			time_coef_second = 1000; //'seconds', 'minutes', 'hours'
			time_coef_minute = time_coef_second * 60;
			time_coef_hour = time_coef_minute * 60;
			//if(MinutesToRestart > 0) { HoursToRestart = HoursToRestart - 1; }
			//if(SecundesToRestart > 0) { MinutesToRestart = MinutesToRestart - 1; }
			return_value = (SecundesToRestart * time_coef_second) + (MinutesToRestart * time_coef_minute) + (HoursToRestart  * time_coef_hour);
		}
		WriteDebug_Log(" return_value " + return_value.ToString());
		return return_value;
	}
	int GetMinRestartValue (map <int,int> TimeMassive)
	{
		private array<int> TimeValue = TimeMassive.GetValueArray();
		TimeValue.Sort(false); 
		return TimeMassive.GetKeyByValue(TimeValue.Get(0));
	}
	int GetUndoMinRestartValue (map <int,int> TimeMassive, int LastCount)
	{
		private array<int> TimeValue = TimeMassive.GetValueArray();
		TimeValue.Sort(false); 
		return TimeMassive.GetKeyByValue(TimeValue.Get(0 + LastCount));
	}
	void GetRestartTimeFromMassive(array<int> GettedRestartTimeDetect, out int RestartHourDetected, out int HoursToRestart, out int RestartMinuteDetected, out int MinutesToRestart, out int RestartSecundeDetected, out int SecundesToRestart)
	{
		RestartHourDetected = GettedRestartTimeDetect.Get(0);
		HoursToRestart = GettedRestartTimeDetect.Get(1);
		RestartMinuteDetected = GettedRestartTimeDetect.Get(2);
		MinutesToRestart = GettedRestartTimeDetect.Get(3);
		RestartSecundeDetected = GettedRestartTimeDetect.Get(4);
		SecundesToRestart = GettedRestartTimeDetect.Get(5);
	}
	
	int GetUndoRestart_Hour (map <int,int> TimeMassive, out int LastCount, out int RestartHourDetected)
	{
		RestartHourDetected = GetUndoMinRestartValue(TimeMassive,LastCount); 
		private int HoursToRestart = TimeMassive.Get(RestartHourDetected);
		Write_Log("[GetUndoRestart_Hour] CALCULATED RestartHour: " + RestartHourDetected.ToString() + " Hours Undo Restart: " + HoursToRestart.ToString());
		return HoursToRestart;
	}
	int GetUndoRestart_Minutes (array<int> RestartHours, array<int> RestartMinutes, array<int> RestartSeconds, array<int> GetActualTime, map <int,int> TimeMassive, out int LastCount, out array<int> ReturnGetRestartTimeDetect)
	{
		private int RestartHourDetected = ReturnGetRestartTimeDetect.Get(0);
		private int HoursToRestart = ReturnGetRestartTimeDetect.Get(1);
		private int RestartMinuteDetected = ReturnGetRestartTimeDetect.Get(2);
		private int MinutesToRestart = ReturnGetRestartTimeDetect.Get(3);
		private int RestartSecundeDetected = ReturnGetRestartTimeDetect.Get(4);
		private int SecundesToRestart = ReturnGetRestartTimeDetect.Get(5);
		private int hour = GetActualTime.Get(0);
		private int minute = GetActualTime.Get(1);
		private int	second =  GetActualTime.Get(2);
		
		if(LastCount > 0)
		{
			HoursToRestart = GetUndoRestart_Hour(TimeMassive, LastCount, RestartHourDetected);
			ReturnGetRestartTimeDetect.Set(0,RestartHourDetected);
			ReturnGetRestartTimeDetect.Set(1,HoursToRestart);
			ReturnGetRestartTimeDetect.Set(2,0); RestartMinuteDetected = 0;
			ReturnGetRestartTimeDetect.Set(3,0); MinutesToRestart = 0;
			ReturnGetRestartTimeDetect.Set(4,0); RestartSecundeDetected = 0;
			ReturnGetRestartTimeDetect.Set(5,0); SecundesToRestart = 0;
			WriteDebug_Log("[GetUndoRestart_Minutes] CALCULATING: updated from LastCount " + LastCount.ToString() + ", Restart time: " + RestartHourDetected.ToString() + ":" + RestartMinuteDetected.ToString() + ":" + RestartSecundeDetected.ToString() + " Undo Restart: " + HoursToRestart.ToString() + ":" + MinutesToRestart.ToString() + ":" + SecundesToRestart.ToString());
		}
		
		RestartMinuteDetected = RestartMinutes.Get(RestartHours.Find(RestartHourDetected)); 
		MinutesToRestart = (RestartMinuteDetected - minute);  
		Write_Log("[GetUndoRestart_Minutes] CALCULATING: RestartMinute: " + RestartMinuteDetected.ToString() + " Minutes Undo Restart: " + MinutesToRestart.ToString());
		WriteDebug_Log("[GetUndoRestart_Minutes] Checking 0: Restart time: " + RestartHourDetected.ToString() + ":" + RestartMinuteDetected.ToString() + ":" + RestartSecundeDetected.ToString() + " Undo Restart: " + HoursToRestart.ToString() + ":" + MinutesToRestart.ToString() + ":" + SecundesToRestart.ToString());
		
		if(MinutesToRestart >= 60) 
		{ 
			Write_Log("[GetUndoRestart_Minutes] CALCULATING DETECT: Minutes Undo Restart >= 60 (" + MinutesToRestart.ToString() + "), fixing!" );	
			MinutesToRestart = 60 - MinutesToRestart; 
			WriteDebug_Log("[GetUndoRestart_Minutes] Checking 1: Restart time: " + RestartHourDetected.ToString() + ":" + RestartMinuteDetected.ToString() + ":" + RestartSecundeDetected.ToString() + " Undo Restart: " + HoursToRestart.ToString() + ":" + MinutesToRestart.ToString() + ":" + SecundesToRestart.ToString());
		}
		if(MinutesToRestart < 0)
		{ 
			Write_Log("[GetUndoRestart_Minutes] CALCULATING DETECT: Minutes Undo Restart < 0 (" + MinutesToRestart.ToString() + "), fixing!" );	
			MinutesToRestart = 60 + MinutesToRestart; 
			HoursToRestart = HoursToRestart - 1; 	
			WriteDebug_Log("[GetUndoRestart_Minutes] Checking 1: Restart time: " + RestartHourDetected.ToString() + ":" + RestartMinuteDetected.ToString() + ":" + RestartSecundeDetected.ToString() + " Undo Restart: " + HoursToRestart.ToString() + ":" + MinutesToRestart.ToString() + ":" + SecundesToRestart.ToString());
			if(HoursToRestart < 0) 
			{ 
				private bool control_while = false;
			//	while(control_while == false)
				{
					Write_Log("[GetUndoRestart_Minutes] CALCULATING DETECT: Hours Undo Restart < 0 (" + HoursToRestart.ToString() + "), generate the new values!" );			
					//private int HoursToRestart = GetUndoRestart_Hour(TimeMassive, LastCount + 1, RestartHourDetected) - 1;
					LastCount = LastCount + 1;
					HoursToRestart = GetUndoRestart_Hour(TimeMassive, LastCount, RestartHourDetected);
					ReturnGetRestartTimeDetect.Set(0,RestartHourDetected);
					ReturnGetRestartTimeDetect.Set(1,HoursToRestart);
					ReturnGetRestartTimeDetect.Set(2,RestartMinuteDetected);
					ReturnGetRestartTimeDetect.Set(3,MinutesToRestart);
					ReturnGetRestartTimeDetect.Set(4,RestartSecundeDetected);
					ReturnGetRestartTimeDetect.Set(5,SecundesToRestart);
					MinutesToRestart = GetUndoRestart_Minutes(RestartHours, RestartMinutes, RestartSeconds, GetActualTime, TimeMassive, LastCount, ReturnGetRestartTimeDetect);	
					GetRestartTimeFromMassive(ReturnGetRestartTimeDetect, RestartHourDetected, HoursToRestart, RestartMinuteDetected, MinutesToRestart,RestartSecundeDetected, SecundesToRestart);					
					WriteDebug_Log("[GetUndoRestart_Minutes] LastCount: LastCount=" + LastCount.ToString() + "" );
					WriteDebug_Log("[GetUndoRestart_Minutes] Checking 0 - 2: Restart time: " + RestartHourDetected.ToString() + ":" + RestartMinuteDetected.ToString() + ":" + RestartSecundeDetected.ToString() + " Undo Restart: " + HoursToRestart.ToString() + ":" + MinutesToRestart.ToString() + ":" + SecundesToRestart.ToString());
					if(HoursToRestart >= 0)
					{
						Write_Log("[GetUndoRestart_Minutes] CALCULATING: Generated RestartHour: " + RestartHourDetected.ToString() + " Hours Undo Restart: " + HoursToRestart.ToString());
						Write_Log("[GetUndoRestart_Minutes] CALCULATING: Generated RestartMinute: " + RestartMinuteDetected.ToString() + " Minutes Undo Restart: " + 	MinutesToRestart.ToString());
						GetRestartTimeFromMassive(ReturnGetRestartTimeDetect, RestartHourDetected, HoursToRestart, RestartMinuteDetected, MinutesToRestart,RestartSecundeDetected, SecundesToRestart);
						WriteDebug_Log("[GetUndoRestart_Minutes] Checking 2: Restart time: " + RestartHourDetected.ToString() + ":" + RestartMinuteDetected.ToString() + ":" + RestartSecundeDetected.ToString() + " Undo Restart: " + HoursToRestart.ToString() + ":" + MinutesToRestart.ToString() + ":" + SecundesToRestart.ToString());
						control_while = true;	
					}
				}
			}
		}
		Write_Log("[GetUndoRestart_Minutes] CALCULATED: RestartMinute: " + RestartMinuteDetected.ToString() + " Minutes Undo Restart: " + MinutesToRestart.ToString());	
		Write_Log("[GetUndoRestart_Minutes] CALCULATED: Restart time: " + RestartHourDetected.ToString() + ":" + RestartMinuteDetected.ToString() + ":" + RestartSecundeDetected.ToString() + " Undo Restart: " + HoursToRestart.ToString() + ":" + MinutesToRestart.ToString() + ":" + SecundesToRestart.ToString());		
		ReturnGetRestartTimeDetect.Set(0,RestartHourDetected);
		ReturnGetRestartTimeDetect.Set(1,HoursToRestart);
		ReturnGetRestartTimeDetect.Set(2,RestartMinuteDetected);
		ReturnGetRestartTimeDetect.Set(3,MinutesToRestart);
		ReturnGetRestartTimeDetect.Set(4,RestartSecundeDetected);
		ReturnGetRestartTimeDetect.Set(5,SecundesToRestart);
		return MinutesToRestart;
	}
	int GetUndoRestart_Secundes (array<int> RestartHours, array<int> RestartMinutes, array<int> RestartSeconds, array<int> GetActualTime, map <int,int> TimeMassive, out int LastCount, out array<int> ReturnGetRestartTimeDetect)
	{
		private int RestartHourDetected = ReturnGetRestartTimeDetect.Get(0);
		private int HoursToRestart = ReturnGetRestartTimeDetect.Get(1);
		private int RestartMinuteDetected = ReturnGetRestartTimeDetect.Get(2);
		private int MinutesToRestart = ReturnGetRestartTimeDetect.Get(3);
		private int RestartSecundeDetected = ReturnGetRestartTimeDetect.Get(4);
		private int SecundesToRestart = ReturnGetRestartTimeDetect.Get(5);
		private int hour = GetActualTime.Get(0);
		private int minute = GetActualTime.Get(1);
		private int	second =  GetActualTime.Get(2);
		
		if(LastCount > 0)
		{
			HoursToRestart = GetUndoRestart_Hour(TimeMassive, LastCount, RestartHourDetected);
			ReturnGetRestartTimeDetect.Set(0,RestartHourDetected);
			ReturnGetRestartTimeDetect.Set(1,HoursToRestart);	
			ReturnGetRestartTimeDetect.Set(4,0); RestartSecundeDetected = 0;
			ReturnGetRestartTimeDetect.Set(5,0); SecundesToRestart = 0;
			MinutesToRestart = GetUndoRestart_Minutes(RestartHours, RestartMinutes, RestartSeconds, GetActualTime, TimeMassive, LastCount, ReturnGetRestartTimeDetect);
			GetRestartTimeFromMassive(ReturnGetRestartTimeDetect, RestartHourDetected, HoursToRestart, RestartMinuteDetected, MinutesToRestart,RestartSecundeDetected, SecundesToRestart);
			WriteDebug_Log("[GetUndoRestart_Secundes] CALCULATING: updated from LastCount " + LastCount.ToString() + ", Restart time: " + RestartHourDetected.ToString() + ":" + RestartMinuteDetected.ToString() + ":" + RestartSecundeDetected.ToString() + " Undo Restart: " + HoursToRestart.ToString() + ":" + MinutesToRestart.ToString() + ":" + SecundesToRestart.ToString());
		}
		
		RestartSecundeDetected = RestartSeconds.Get(RestartHours.Find(RestartHourDetected));
		SecundesToRestart = (RestartSecundeDetected - second);
		WriteDebug_Log("[GetUndoRestart_Secundes] Checking 0: Restart time: " + RestartHourDetected.ToString() + ":" + RestartMinuteDetected.ToString() + ":" + RestartSecundeDetected.ToString() + " Undo Restart: " + HoursToRestart.ToString() + ":" + MinutesToRestart.ToString() + ":" + SecundesToRestart.ToString());
		Write_Log("[GetUndoRestart_Secundes] CALCULATING: RestartSecunde: " + RestartSecundeDetected.ToString() + " Secundes Undo Restart: " + SecundesToRestart.ToString());
		//private int SecundesToRestart = ((Math.AbsInt(60 - RestartSecundeDetected) - second)); Write_Log(" SecundesToRestart " + SecundesToRestart.ToString());
		if(SecundesToRestart >= 60) 
		{ 
			Write_Log("[GetUndoRestart_Secundes] CALCULATING DETECT: Secundes Undo Restart >= 60 (" + SecundesToRestart.ToString() + "), fixing!" );	
			SecundesToRestart = 60 - SecundesToRestart;
			WriteDebug_Log("[GetUndoRestart_Secundes] Checking 1: Restart time: " + RestartHourDetected.ToString() + ":" + RestartMinuteDetected.ToString() + ":" + RestartSecundeDetected.ToString() + " Undo Restart: " + HoursToRestart.ToString() + ":" + MinutesToRestart.ToString() + ":" + SecundesToRestart.ToString());
		} 
		if(SecundesToRestart < 0) 
		{ 
			Write_Log("[GetUndoRestart_Secundes] CALCULATING DETECT: Secundes Undo Restart < 0 (" + SecundesToRestart.ToString() + "), fixing!" );			
			SecundesToRestart = 60 + SecundesToRestart;
			MinutesToRestart = MinutesToRestart - 1;
			WriteDebug_Log("[GetUndoRestart_Secundes] Checking 1: Restart time: " + RestartHourDetected.ToString() + ":" + RestartMinuteDetected.ToString() + ":" + RestartSecundeDetected.ToString() + " Undo Restart: " + HoursToRestart.ToString() + ":" + MinutesToRestart.ToString() + ":" + SecundesToRestart.ToString());
			if(MinutesToRestart < 0) 
			{ 
				private bool control_while_minutes = false;
				Write_Log("[GetUndoRestart_Secundes] CALCULATING DETECT: Minutes Undo Restart < 0 (" + MinutesToRestart.ToString() + "), generate the new values!" );
				if(HoursToRestart >= 1)
				{
					Write_Log("[GetUndoRestart_Secundes] CALCULATING DETECT: Hours Undo Restart >= 1 (" + HoursToRestart.ToString() + "), generate the new values for Minutes Undo Restart!" );
					HoursToRestart = HoursToRestart - 1;
					MinutesToRestart = 60 + MinutesToRestart;
					Write_Log("[GetUndoRestart_Secundes] CALCULATING Correct: Restart time: " + RestartHourDetected.ToString() + ":" + RestartMinuteDetected.ToString() + ":" + RestartSecundeDetected.ToString() + " Undo Restart: " + HoursToRestart.ToString() + ":" + MinutesToRestart.ToString() + ":" + SecundesToRestart.ToString());
					ReturnGetRestartTimeDetect.Set(0,RestartHourDetected);
					ReturnGetRestartTimeDetect.Set(1,HoursToRestart);
					ReturnGetRestartTimeDetect.Set(2,RestartMinuteDetected);
					ReturnGetRestartTimeDetect.Set(3,MinutesToRestart);
					ReturnGetRestartTimeDetect.Set(4,RestartSecundeDetected);
					ReturnGetRestartTimeDetect.Set(5,SecundesToRestart);
				} else
				{
					Write_Log("[GetUndoRestart_Secundes] CALCULATING DETECT: Hours Undo Restart < 1 (" + HoursToRestart.ToString() + "), generate the new Restart Time!" );
				//	while(control_while_minutes == false)
					{
						ReturnGetRestartTimeDetect.Set(0,RestartHourDetected);
						ReturnGetRestartTimeDetect.Set(1,HoursToRestart);
						ReturnGetRestartTimeDetect.Set(2,RestartMinuteDetected);
						ReturnGetRestartTimeDetect.Set(3,MinutesToRestart);
						ReturnGetRestartTimeDetect.Set(4,RestartSecundeDetected);
						ReturnGetRestartTimeDetect.Set(5,SecundesToRestart);
						LastCount = LastCount + 1;
						WriteDebug_Log("[GetUndoRestart_Secundes] 0 LastCount: LastCount=" + LastCount.ToString() + "" );
						MinutesToRestart = GetUndoRestart_Minutes(RestartHours, RestartMinutes, RestartSeconds, GetActualTime, TimeMassive, LastCount, ReturnGetRestartTimeDetect) - 1;
						GetRestartTimeFromMassive(ReturnGetRestartTimeDetect, RestartHourDetected, HoursToRestart, RestartMinuteDetected, MinutesToRestart,RestartSecundeDetected, SecundesToRestart);
						SecundesToRestart = GetUndoRestart_Secundes (RestartHours, RestartMinutes, RestartSeconds, GetActualTime,TimeMassive, LastCount, ReturnGetRestartTimeDetect);
						GetRestartTimeFromMassive(ReturnGetRestartTimeDetect, RestartHourDetected, HoursToRestart, RestartMinuteDetected, MinutesToRestart,RestartSecundeDetected, SecundesToRestart);
						WriteDebug_Log("[GetUndoRestart_Secundes] 1 LastCount: LastCount=" + LastCount.ToString() + "" );
						WriteDebug_Log("[GetUndoRestart_Secundes] Checking 0 - 2: Restart time: " + RestartHourDetected.ToString() + ":" + RestartMinuteDetected.ToString() + ":" + RestartSecundeDetected.ToString() + " Undo Restart: " + HoursToRestart.ToString() + ":" + MinutesToRestart.ToString() + ":" + SecundesToRestart.ToString());
						if(MinutesToRestart >= 0) 
						{ 
							Write_Log("[GetUndoRestart_Secundes] CALCULATING: Generated RestartSecunde: " + RestartSecundeDetected.ToString() + " Secundes Undo Restart: " + SecundesToRestart.ToString());
							GetRestartTimeFromMassive(ReturnGetRestartTimeDetect, RestartHourDetected, HoursToRestart, RestartMinuteDetected, MinutesToRestart,RestartSecundeDetected, SecundesToRestart);
							WriteDebug_Log("[GetUndoRestart_Secundes] Checking 2: Restart time: " + RestartHourDetected.ToString() + ":" + RestartMinuteDetected.ToString() + ":" + RestartSecundeDetected.ToString() + " Undo Restart: " + HoursToRestart.ToString() + ":" + MinutesToRestart.ToString() + ":" + SecundesToRestart.ToString());
							control_while_minutes = true;
						}		
					}
				}			
			}
		} 
		Write_Log("[GetUndoRestart_Secundes] CALCULATED: RestartSecunde: " + RestartSecundeDetected.ToString() + " Secundes Undo Restart: " + SecundesToRestart.ToString());
		Write_Log("[GetUndoRestart_Secundes] CALCULATED: Restart time: " + RestartHourDetected.ToString() + ":" + RestartMinuteDetected.ToString() + ":" + RestartSecundeDetected.ToString() + " Undo Restart: " + HoursToRestart.ToString() + ":" + MinutesToRestart.ToString() + ":" + SecundesToRestart.ToString());		
		ReturnGetRestartTimeDetect.Set(0,RestartHourDetected);
		ReturnGetRestartTimeDetect.Set(1,HoursToRestart);
		ReturnGetRestartTimeDetect.Set(2,RestartMinuteDetected);
		ReturnGetRestartTimeDetect.Set(3,MinutesToRestart);
		ReturnGetRestartTimeDetect.Set(4,RestartSecundeDetected);
		ReturnGetRestartTimeDetect.Set(5,SecundesToRestart);
		return SecundesToRestart;
	}
	void Run_InfoChat_AutoRestart_server(bool fixed_restarts, int Time_fixed_restarts = 0) 
	{
		private string sending_text_RestartsInformation_chat = text_RestartsInformation_chat;
		if(!fixed_restarts)
		{
			private float uptime_server_text;
			private float restart_time_server_text;		
			uptime_server_text = (tick_time_timer / 60);
			restart_time_server_text = (((all_time_to_restart / 1000) / 60) - uptime_server_text);
			if(sending_text_RestartsInformation_chat.Contains("%time%"))
			{
				sending_text_RestartsInformation_chat.Replace("%time%", Math.Round(restart_time_server_text).ToString());
			}
		} else
		{
			if(sending_text_RestartsInformation_chat.Contains("%time%"))
			{
				sending_text_RestartsInformation_chat.Replace("%time%", Math.Round(Time_fixed_restarts).ToString());
			}
		}
		private string text_log = "[ChatSending]: " + sending_text_RestartsInformation_chat;
		fnc_SendMessage_GlobalChat(sending_text_RestartsInformation_chat);
		Write_Log(text_log);
	}	
	void Run_InfoChat_UptimeAndRestartTime_server() 
	{
		private float uptime_server_text = 0.0;
		private float restart_time_server_text = 0.0;
		if(time_format_chat_info_restart_and_uptime == "hours")
		{
			uptime_server_text = (tick_time_timer / 60) / 60;
			restart_time_server_text = ((((all_time_to_restart / 1000) / 60) / 60) - uptime_server_text);			
		}
		if(time_format_chat_info_restart_and_uptime == "minutes")
		{
			uptime_server_text = (tick_time_timer / 60);
			restart_time_server_text = (((all_time_to_restart / 1000) / 60) - uptime_server_text);
		}
		if(time_format_chat_info_restart_and_uptime == "seconds")
		{
			uptime_server_text = tick_time_timer;
			restart_time_server_text = ((all_time_to_restart / 1000) - uptime_server_text); 
		}
		private string text_chat_uptime = "";
		private string text_chat_restart_time = "";
		if(match_time_chat_info_restart_and_uptime)
		{
			text_chat_uptime = Math.Round(uptime_server_text).ToString();
			text_chat_restart_time = Math.Round(restart_time_server_text).ToString();
		} else
		{
			text_chat_uptime = uptime_server_text.ToString();
			text_chat_restart_time = restart_time_server_text.ToString();
		}
		private string sending_text_chat_info_restart_and_uptime = text_chat_info_restart_and_uptime;
		if(sending_text_chat_info_restart_and_uptime.Contains("%uptime%"))
		{
			sending_text_chat_info_restart_and_uptime.Replace("%uptime%", text_chat_uptime);
		}
		if(sending_text_chat_info_restart_and_uptime.Contains("%timer%"))
		{
			sending_text_chat_info_restart_and_uptime.Replace("%timer%", text_chat_restart_time);
		}
		private string text_log = "[ChatSending]: " + sending_text_chat_info_restart_and_uptime;
		fnc_SendMessage_GlobalChat(sending_text_chat_info_restart_and_uptime);
		Write_Log(text_log);
	}
	
	void RunPlayerDisconnect(PlayerBase player)
	{	
		if( player ) 
		{		
			GetGame().GetMission().OnEvent(ClientDisconnectedEventTypeID, new ClientDisconnectedEventParams(player.GetIdentity(), player, 0, false));
		}
	}
	
	void CheckLockServerInConnectPlayer(PlayerBase player) 
	{	
		if (Server_closed)
		{
		//	Print("Server is locked! Connect DISABLED! Player will be kicked!");
		//	Print("PARAMETER: " + (Get_RestartServer_System().Server_closed).ToString());
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(RunPlayerDisconnect, 1 * 1000, false,player); //MOTD Information Server
			return;
		} else
		{
		//	Print("Server is UNlocked! Connect Enabled!");
		//	Print("PARAMETER: " + (Get_RestartServer_System().Server_closed).ToString());
		}	
	}
}


static ref RestartServer_System g_RestartServer_System;
static ref RestartServer_System Get_RestartServer_System()
{
    if ( !g_RestartServer_System )
    {
         g_RestartServer_System = new ref RestartServer_System();
    }
    
    return g_RestartServer_System;
}

modded class CustomMission
//modded class MissionServer
{
	override void OnInit()
	{
		super.OnInit();
		Get_RestartServer_System().Run_Timer_AutoRestart_server();
	}
	override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
	{		
		super.InvokeOnConnect(player, identity);
		Get_RestartServer_System().CheckLockServerInConnectPlayer(player);	
	}
}