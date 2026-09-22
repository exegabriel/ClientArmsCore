#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <events>
#include <textparse>
#include <cstrike>
#include <sdktools>
#include <eItems>
#include <PTaH>
#include <clientarmscore>

#define BOTSKINS_CONFIG "configs/botskins.txt"
#define BOTSKINS_GAME_PATH "addons/sourcemod/configs/botskins.txt"
#define MAX_BOT_KNIFE_OPTIONS 128
#define MAX_BOT_GLOVE_OPTIONS 128

int g_iKnifeDef[MAX_BOT_KNIFE_OPTIONS];
int g_iKnifePaint[MAX_BOT_KNIFE_OPTIONS];
int g_iKnifeCount;
int g_iGloveDef[MAX_BOT_GLOVE_OPTIONS];
int g_iGlovePaint[MAX_BOT_GLOVE_OPTIONS];
int g_iGloveCount;
int g_iKnifeChoice[MAXPLAYERS + 1] = {-1, ...};
int g_iGloveChoice[MAXPLAYERS + 1] = {-1, ...};
int g_iGloveEntity[MAXPLAYERS + 1] = {-1, ...};
bool g_bGloveApplyQueued[MAXPLAYERS + 1];
bool g_bKnivesEnabled;
bool g_bGlovesEnabled;
char g_szConfigContents[65536];
char g_szParseSections[8][64];
int g_iParseDepth;
int g_iParseDefIndex = -1;

public Plugin myinfo =
{
	name = "[CS:GO] BOT Skins",
	author = "Unknown",
	description = "Applies customized skins to bots.",
	version = "0.0.2"
};

public void OnPluginStart()
{
	if (!LoadBotSkinsConfig())
	{
		SetFailState("Unable to load %s", BOTSKINS_CONFIG);
	}

	HookEventEx("player_spawn", Event_PlayerSpawn, EventHookMode_Post);
}

public void OnClientDisconnect(int client)
{
	RemoveBotGlove(client);
	g_bGloveApplyQueued[client] = false;
	g_iKnifeChoice[client] = -1;
	g_iGloveChoice[client] = -1;
}

public void Event_PlayerSpawn(Event event, const char[] name, bool dontBroadcast)
{
	int client = GetClientOfUserId(event.GetInt("userid"));
	if (client <= 0 || !IsClientInGame(client) || !IsFakeClient(client))
	{
		return;
	}

	g_bGloveApplyQueued[client] = false;
	CreateTimer(0.25, Timer_ApplyBotSkins, GetClientUserId(client), TIMER_FLAG_NO_MAPCHANGE);
}

public Action Timer_ApplyBotSkins(Handle timer, int userid)
{
	int client = GetClientOfUserId(userid);
	if (client <= 0 || !IsClientInGame(client) || !IsFakeClient(client) || !IsPlayerAlive(client))
	{
		return Plugin_Stop;
	}

	ApplyBotSkins(client);
	return Plugin_Stop;
}

void ApplyBotSkins(int client)
{
	if (g_bKnivesEnabled && g_iKnifeCount > 0)
	{
		if (g_iKnifeChoice[client] == -1)
		{
			g_iKnifeChoice[client] = GetRandomInt(0, g_iKnifeCount - 1);
		}

		ApplyBotKnife(client, g_iKnifeChoice[client]);
	}

	if (g_bGlovesEnabled && g_iGloveCount > 0)
	{
		if (g_iGloveChoice[client] == -1)
		{
			g_iGloveChoice[client] = GetRandomInt(0, g_iGloveCount - 1);
		}

		ApplyBotGlove(client, g_iGloveChoice[client]);
	}
}

void ApplyBotKnife(int client, int choice)
{
	if (choice < 0 || choice >= g_iKnifeCount)
	{
		return;
	}

	char classname[64];
	int defIndex = g_iKnifeDef[choice];
	if (!GetKnifeClassName(defIndex, classname, sizeof(classname)))
	{
		PrintToServer("[Bot Skins] bot %d knife def_index %d has no classname", client, defIndex);
		return;
	}

	int oldKnife = GetPlayerWeaponSlot(client, CS_SLOT_KNIFE);
	if (oldKnife > MaxClients && IsValidEntity(oldKnife))
	{
		RemovePlayerItem(client, oldKnife);
		AcceptEntityInput(oldKnife, "Kill");
	}

	int weapon = GivePlayerItem(client, classname);
	if (weapon == -1 || !IsValidEntity(weapon))
	{
		PrintToServer("[Bot Skins] bot %d failed to give knife %s def_index %d", client, classname, defIndex);
		return;
	}

	PrintToServer("[Bot Skins] bot %d knife applied: %s def_index %d paint %d", client, classname, defIndex, g_iKnifePaint[choice]);

	SetWeaponEconProperty(weapon, "m_iItemDefinitionIndex", defIndex);
	SetWeaponEconProperty(weapon, "m_iItemIDHigh", -1);
	SetWeaponEconProperty(weapon, "m_iItemIDLow", -1);
	SetWeaponEconProperty(weapon, "m_nFallbackPaintKit", g_iKnifePaint[choice]);
	SetWeaponEconProperty(weapon, "m_nFallbackSeed", 0);
	SetWeaponEconProperty(weapon, "m_nFallbackStatTrak", -1);
	SetWeaponEconProperty(weapon, "m_iEntityQuality", 3);
	SetWeaponEconPropertyFloat(weapon, "m_flFallbackWear", 0.01);
	EquipPlayerWeapon(client, weapon);
}

void ApplyBotGlove(int client, int choice)
{
	if (choice < 0 || choice >= g_iGloveCount)
	{
		return;
	}

	if (g_bGloveApplyQueued[client])
	{
		return;
	}

	g_bGloveApplyQueued[client] = true;
	CreateTimer(0.12, Timer_ApplyBotGloveDelayed, GetClientUserId(client), TIMER_FLAG_NO_MAPCHANGE);
}

public Action Timer_ApplyBotGloveDelayed(Handle timer, int userid)
{
	int client = GetClientOfUserId(userid);
	g_bGloveApplyQueued[client] = false;

	if (client <= 0 || !IsClientInGame(client) || !IsFakeClient(client) || !IsPlayerAlive(client))
	{
		return Plugin_Stop;
	}

	int choice = g_iGloveChoice[client];
	if (choice < 0 || choice >= g_iGloveCount)
	{
		return Plugin_Stop;
	}

	if (GetFeatureStatus(FeatureType_Native, "AF_RemoveClientArmsModel") == FeatureStatus_Available)
	{
		AF_RemoveClientArmsModel(client);
	}

	RemoveBotGlove(client);
	int glove = CreateEntityByName("wearable_item");
	if (glove == -1 || !IsValidEntity(glove))
	{
		PrintToServer("[Bot Skins] bot %d failed to create glove wearable", client);
		return Plugin_Stop;
	}

	SetWeaponEconProperty(glove, "m_iItemDefinitionIndex", g_iGloveDef[choice]);
	SetWeaponEconProperty(glove, "m_iItemIDLow", -1);
	SetWeaponEconProperty(glove, "m_nFallbackPaintKit", g_iGlovePaint[choice]);
	SetWeaponEconProperty(glove, "m_nFallbackSeed", GetRandomInt(1, 1000));
	SetWeaponEconPropertyFloat(glove, "m_flFallbackWear", 0.00000001);
	SetWeaponEconProperty(glove, "m_bInitialized", 1);

	DispatchSpawn(glove);

	if (!IsValidEntity(glove))
	{
		return Plugin_Stop;
	}

	SetWeaponEconPropertyEnt(glove, "m_hOwnerEntity", client);

	if (!HasEntProp(client, Prop_Send, "m_hMyWearables"))
	{
		AcceptEntityInput(glove, "Kill");
		return Plugin_Stop;
	}

	SetEntPropEnt(client, Prop_Send, "m_hMyWearables", glove, 0);
	SetBotGloveBodygroup(client);
	g_iGloveEntity[client] = glove;
	ApplyArmsFixGloveModel(client, glove, g_iGloveDef[choice], g_iGlovePaint[choice]);

	float wear = 0.0;
	if (HasEntProp(glove, Prop_Send, "m_flFallbackWear"))
	{
		wear = GetEntPropFloat(glove, Prop_Send, "m_flFallbackWear");
	}
	else if (HasEntProp(glove, Prop_Data, "m_flFallbackWear"))
	{
		wear = GetEntPropFloat(glove, Prop_Data, "m_flFallbackWear");
	}

	PrintToServer("[Bot Skins] bot %d glove: ent=%d def=%d paint=%d wear=%.3f", client, glove, g_iGloveDef[choice], g_iGlovePaint[choice], wear);
	return Plugin_Stop;
}

void ApplyArmsFixGloveModel(int client, int glove, int defIndex, int paintKit)
{
	if (IsValidEntity(glove))
	{
		if (GetFeatureStatus(FeatureType_Native, "AF_ApplyClientGloveSkin") == FeatureStatus_Available)
		{
			AF_ApplyClientGloveSkin(client, glove, defIndex, paintKit, 0, 0.00000001);
		}
	}

	if (GetFeatureStatus(FeatureType_Native, "AF_SetClientArmsModel") != FeatureStatus_Available
		|| (GetFeatureStatus(FeatureType_Native, "AF_RequestArmsUpdate") != FeatureStatus_Available
			&& GetFeatureStatus(FeatureType_Native, "AF_RefreshClientViewModel") != FeatureStatus_Available))
	{
		PrintToServer("[Bot Skins] ClientArmsCore natives unavailable for bot %d", client);
		return;
	}

	char model[PLATFORM_MAX_PATH];
	if (!eItems_GetGlovesViewModelByDefIndex(defIndex, model, sizeof(model)) || model[0] == '\0')
	{
		PrintToServer("[Bot Skins] glove def_index %d has no eItems view model", defIndex);
		return;
	}

	SetBotGloveBodygroup(client);

	if (GetFeatureStatus(FeatureType_Native, "AF_DisableClientArmsUpdate") == FeatureStatus_Available)
	{
		AF_DisableClientArmsUpdate(client, false, false);
	}

	ForceClientArmsRefresh(client, model);
	CreateTimer(0.15, Timer_RefreshBotArms, GetClientUserId(client), TIMER_FLAG_NO_MAPCHANGE);
	CreateTimer(0.35, Timer_RefreshBotArms, GetClientUserId(client), TIMER_FLAG_NO_MAPCHANGE);
}

void ForceClientArmsRefresh(int client, char[] model)
{
	if (GetFeatureStatus(FeatureType_Native, "AF_SetClientArmsModel") == FeatureStatus_Available)
	{
		AF_SetClientArmsModel(client, model);
	}

	if (GetFeatureStatus(FeatureType_Native, "AF_RefreshClientViewModel") == FeatureStatus_Available)
	{
		AF_RefreshClientViewModel(client, true);
	}
	else if (GetFeatureStatus(FeatureType_Native, "AF_RequestArmsUpdate") == FeatureStatus_Available)
	{
		AF_RequestArmsUpdate(client, true);
	}

	if (GetFeatureStatus(FeatureType_Native, "AF_RefreshClientViewModel") != FeatureStatus_Available
		&& GetFeatureStatus(FeatureType_Native, "AF_ForceArmsUpdate") == FeatureStatus_Available)
	{
		AF_ForceArmsUpdate(client, true);
	}
}

public Action Timer_RefreshBotArms(Handle timer, int userid)
{
	int client = GetClientOfUserId(userid);
	if (client <= 0 || !IsClientInGame(client) || !IsPlayerAlive(client))
	{
		return Plugin_Stop;
	}

	if (g_iGloveChoice[client] < 0 || g_iGloveChoice[client] >= g_iGloveCount)
	{
		return Plugin_Stop;
	}

	int choice = g_iGloveChoice[client];
	int glove = g_iGloveEntity[client];
	if (glove > MaxClients && IsValidEntity(glove)
		&& GetFeatureStatus(FeatureType_Native, "AF_ApplyClientGloveSkin") == FeatureStatus_Available)
	{
		AF_ApplyClientGloveSkin(client, glove, g_iGloveDef[choice], g_iGlovePaint[choice], 0, 0.00000001);
	}

	char model[PLATFORM_MAX_PATH];
	if (!eItems_GetGlovesViewModelByDefIndex(g_iGloveDef[choice], model, sizeof(model)) || model[0] == '\0')
	{
		return Plugin_Stop;
	}

	SetBotGloveBodygroup(client);

	ForceClientArmsRefresh(client, model);
	return Plugin_Stop;
}

void SetBotGloveBodygroup(int client)
{
	if (!HasEntProp(client, Prop_Send, "m_nBody"))
	{
		return;
	}

	SetEntProp(client, Prop_Send, "m_nBody", GetClientTeam(client) == CS_TEAM_CT ? 0 : 1);
}

bool GetKnifeClassName(int defIndex, char[] classname, int maxlength)
{
	switch (defIndex)
	{
		case 503: strcopy(classname, maxlength, "weapon_knife_css");
		case 505: strcopy(classname, maxlength, "weapon_knife_flip");
		case 506: strcopy(classname, maxlength, "weapon_knife_gut");
		case 507: strcopy(classname, maxlength, "weapon_knife_karambit");
		case 509: strcopy(classname, maxlength, "weapon_knife_tactical");
		case 512: strcopy(classname, maxlength, "weapon_knife_falchion");
		case 514: strcopy(classname, maxlength, "weapon_knife_survival_bowie");
		case 515: strcopy(classname, maxlength, "weapon_knife_butterfly");
		case 516: strcopy(classname, maxlength, "weapon_knife_push");
		case 517: strcopy(classname, maxlength, "weapon_knife_cord");
		case 518: strcopy(classname, maxlength, "weapon_knife_canis");
		case 519: strcopy(classname, maxlength, "weapon_knife_ursus");
		case 520: strcopy(classname, maxlength, "weapon_knife_gypsy_jackknife");
		case 521: strcopy(classname, maxlength, "weapon_knife_outdoor");
		case 522: strcopy(classname, maxlength, "weapon_knife_stiletto");
		case 523: strcopy(classname, maxlength, "weapon_knife_widowmaker");
		case 525: strcopy(classname, maxlength, "weapon_knife_skeleton");
		default: return false;
	}

	return true;
}

void RemoveBotGlove(int client)
{
	int trackedGlove = g_iGloveEntity[client];
	g_iGloveEntity[client] = -1;

	int equippedGlove = -1;

	if (HasEntProp(client, Prop_Send, "m_hMyWearables"))
	{
		equippedGlove = GetEntPropEnt(client, Prop_Send, "m_hMyWearables");
		SetEntPropEnt(client, Prop_Send, "m_hMyWearables", -1, 0);
	}

	if (trackedGlove > MaxClients && IsValidEntity(trackedGlove))
	{
		AcceptEntityInput(trackedGlove, "Kill");
	}

	if (equippedGlove > MaxClients
		&& equippedGlove != trackedGlove
		&& IsValidEntity(equippedGlove))
	{
		AcceptEntityInput(equippedGlove, "Kill");
	}

	int entity = -1;
	while ((entity = FindEntityByClassname(entity, "wearable_item")) != -1)
	{
		if (entity == trackedGlove || entity == equippedGlove || !IsValidEntity(entity))
		{
			continue;
		}

		int owner = -1;
		if (HasEntProp(entity, Prop_Send, "m_hOwnerEntity"))
		{
			owner = GetEntPropEnt(entity, Prop_Send, "m_hOwnerEntity");
		}
		else if (HasEntProp(entity, Prop_Data, "m_hOwnerEntity"))
		{
			owner = GetEntPropEnt(entity, Prop_Data, "m_hOwnerEntity");
		}

		if (owner == client)
		{
			AcceptEntityInput(entity, "Kill");
		}
	}
}

void SetWeaponEconProperty(int entity, const char[] property, int value)
{
	if (HasEntProp(entity, Prop_Send, property))
	{
		SetEntProp(entity, Prop_Send, property, value);
	}
	else if (HasEntProp(entity, Prop_Data, property))
	{
		SetEntProp(entity, Prop_Data, property, value);
	}
}

void SetWeaponEconPropertyFloat(int entity, const char[] property, float value)
{
	if (HasEntProp(entity, Prop_Send, property))
	{
		SetEntPropFloat(entity, Prop_Send, property, value);
	}
	else if (HasEntProp(entity, Prop_Data, property))
	{
		SetEntPropFloat(entity, Prop_Data, property, value);
	}
}

void SetWeaponEconPropertyEnt(int entity, const char[] property, int value)
{
	if (HasEntProp(entity, Prop_Send, property))
	{
		SetEntPropEnt(entity, Prop_Send, property, value);
	}
	else if (HasEntProp(entity, Prop_Data, property))
	{
		SetEntPropEnt(entity, Prop_Data, property, value);
	}
}

bool LoadBotSkinsConfig()
{
	char configPath[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, configPath, sizeof(configPath), BOTSKINS_CONFIG);
	File file = OpenFile(configPath, "r");

	if (file == null)
	{
		LogError("Unable to open %s (resolved path: %s)", BOTSKINS_CONFIG, configPath);
		return false;
	}

	int length = file.ReadString(g_szConfigContents, sizeof(g_szConfigContents), -1);
	delete file;

	if (length <= 0)
	{
		LogError("Unable to parse %s (resolved path: %s)", BOTSKINS_CONFIG, configPath);
		return false;
	}

	SMCParser parser = new SMCParser();
	parser.OnEnterSection = SMC_OnEnterSection;
	parser.OnLeaveSection = SMC_OnLeaveSection;
	parser.OnKeyValue = SMC_OnKeyValue;
	g_iParseDepth = 0;
	g_iParseDefIndex = -1;

	int line;
	int column;
	SMCError result = parser.ParseString(g_szConfigContents, line, column);
	if (result != SMCError_Okay)
	{
		char error[128];
		parser.GetErrorString(result, error, sizeof(error));
		LogError("Unable to parse %s at line %d, column %d: %s", BOTSKINS_CONFIG, line, column, error);
		delete parser;
		return false;
	}

	delete parser;
	PrintToServer("[Bot Skins] loaded: knives=%d gloves=%d knives_enabled=%d gloves_enabled=%d", g_iKnifeCount, g_iGloveCount, g_bKnivesEnabled, g_bGlovesEnabled);
	return g_iKnifeCount > 0 || g_iGloveCount > 0;
}

public SMCResult SMC_OnEnterSection(SMCParser parser, const char[] name, bool optQuotes)
{
	if (g_iParseDepth < sizeof(g_szParseSections))
	{
		strcopy(g_szParseSections[g_iParseDepth], sizeof(g_szParseSections[]), name);
		g_iParseDepth++;
	}

	if (g_iParseDepth == 3)
	{
		g_iParseDefIndex = -1;
	}

	return SMCParse_Continue;
}

public SMCResult SMC_OnLeaveSection(SMCParser parser)
{
	if (g_iParseDepth > 0)
	{
		g_iParseDepth--;
	}

	return SMCParse_Continue;
}

public SMCResult SMC_OnKeyValue(SMCParser parser, const char[] key, const char[] value, bool keyQuotes, bool valueQuotes)
{
	if (g_iParseDepth == 2 && StrEqual(g_szParseSections[1], "settings"))
	{
		if (StrEqual(key, "knives_enabled"))
		{
			g_bKnivesEnabled = StringToInt(value) != 0;
		}
		else if (StrEqual(key, "gloves_enabled"))
		{
			g_bGlovesEnabled = StringToInt(value) != 0;
		}
	}
	else if (g_iParseDepth == 3
		&& (StrEqual(g_szParseSections[1], "gloves") || StrEqual(g_szParseSections[1], "knives"))
		&& StrEqual(key, "def_index"))
	{
		g_iParseDefIndex = StringToInt(value);
	}
	else if (g_iParseDepth == 4 && StrEqual(g_szParseSections[3], "paints") && g_iParseDefIndex >= 0)
	{
		int paint = StringToInt(value);
		if (StrEqual(g_szParseSections[1], "knives") && g_iKnifeCount < MAX_BOT_KNIFE_OPTIONS)
		{
			g_iKnifeDef[g_iKnifeCount] = g_iParseDefIndex;
			g_iKnifePaint[g_iKnifeCount++] = paint;
		}
		else if (StrEqual(g_szParseSections[1], "gloves") && g_iGloveCount < MAX_BOT_GLOVE_OPTIONS)
		{
			g_iGloveDef[g_iGloveCount] = g_iParseDefIndex;
			g_iGlovePaint[g_iGloveCount++] = paint;
		}
	}

	return SMCParse_Continue;
}