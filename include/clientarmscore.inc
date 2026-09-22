#if defined _clientarmscore_included
  #endinput
#endif
#define _clientarmscore_included


enum AF_UpdateReason {
    AF_PlayerSpawned            = 0,
    AF_PluginRequested          = 1,
    AF_BlockedPluginRequested   = 2,
    AF_OnDisableAndUpdate       = 3,
    AF_ChangedByPlugin          = 4,
};

/*********************************************************
 * Called when arms updated
 *
 * @param client        The client index of the player
 * @param res           Update reason
 * @no return
 *********************************************************/
forward void AF_OnArmsUpdate(int client, AF_UpdateReason res);


/*********************************************************
 * Get ArmsFix+ version
 *
 * @return              Version number
 *********************************************************/
native int AF_Version();


/*********************************************************
 * Change default arms models
 *
 * @param mdl_path      Full path to arms mdl (relative to ...\csgo\)
 * @no return
 *********************************************************/
native void AF_SetDefaultArmsModel(char[] mdl_path);


/*********************************************************
 * Resets default arms models
 *
 * @no return
 *********************************************************/
native void AF_ResetDefaultArmsModel();


/*********************************************************
 * Change client arms models
 *
 * @param client        Client index (not userid)
 * @param mdl_path      String path to the model (maxlen=192 including zero terminator)
 * @no return
 *********************************************************/
native void AF_SetClientArmsModel(int client, char[] mdl_path);


/*********************************************************
 * Is client using custom arms model
 *
 * @param client        Client index (not userid)
 * @return              true is client using custom arms model
 *********************************************************/
native bool AF_HasClientCustomArms(int client);


/*********************************************************
 * Simply remove custom arms model (like on player model change)
 *
 * @param client        Client index (not userid)
 * @no return
 *********************************************************/
native void AF_RemoveClientArmsModel(int client);


/*********************************************************
 * Get player arms model
 *
 * @param client        Client index (not userid)
 * @param dest          Destination string
 * @param maxlen        Destination string length
 * @return              Number of bytes written
 *********************************************************/
native int AF_GetClientArmsModel(int client, char[] dest, int maxlen);


/*********************************************************
 * Get default arms model
 *
 * @param dest          Destination string
 * @param maxlen        Destination string length
 * @return              Number of bytes written
 *********************************************************/
native int AF_GetDefaultArmsModel(char[] dest, int maxlen);


/*********************************************************
 * Simply SET arms model (like on player model change)
 *
 * @param client        Client index (not userid)
 * @param force         Update arms RIGHT NOW (wont be updated if arms disabled)
 * @return              true is changed to custom, false otherwise
 *********************************************************/
native bool AF_RequestArmsUpdate(int client, bool force = false);


/*********************************************************
 * Disable arms updating (usable for gloves plugin with gloves's own arms)
 *
 * @param client        Client index (not userid)
 * @param disable       true to disable updating, true to enable updating
 * @param remove_arms   true to remove arms on disable
 * @no return
 *********************************************************/
native void AF_DisableClientArmsUpdate(int client, bool disable = true, bool remove_arms = true);


/*********************************************************
 * Are they disabled?
 *
 * @param client        Client index (not userid)
 * @return              true if blocked, false otherwise
 *********************************************************/
native bool AF_IsClientArmsNotUpdating(int client);


/*********************************************************
 * EXP: Force set client arms stored in m_szArmsModel netprop (or by AF_SetClientArmsModel)
 *
 * @param client        Client index (not userid)
 * @param ignore_blocked Doesn`t matter if arms blocked. Force Update Power!
 * @return              true if updated, false if NOT updated (can be achieved by disabling updates)
 *********************************************************/
native bool AF_ForceArmsUpdate(int client, bool ignore_blocked = false);

native bool AF_RefreshClientViewModel(int client, bool ignore_blocked = false);

native bool AF_ApplyClientGloveSkin(int client, int glove, int defIndex, int paintKit, int seed = 0, float wear = 0.00000001);


public Extension __ext_clientarmscore = 
{
    name = "ClientArmsCore",
    file = "ClientArmsCore.ext",
#if defined AUTOLOAD_EXTENSIONS
    autoload = 1,
#else
    autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
    required = 1,
#else
    required = 0,
#endif
};

#if !defined REQUIRE_EXTENSIONS
public __ext_clientarmscore_SetNTVOptional()
{
    MarkNativeAsOptional("AF_Version");
    MarkNativeAsOptional("AF_SetDefaultArmsModel");
    MarkNativeAsOptional("AF_ResetDefaultArmsModel");
    MarkNativeAsOptional("AF_HasClientCustomArms");
    MarkNativeAsOptional("AF_SetClientArmsModel");
    MarkNativeAsOptional("AF_RemoveClientArmsModel");
    MarkNativeAsOptional("AF_GetClientArmsModel");
    MarkNativeAsOptional("AF_GetDefaultArmsModel");
    MarkNativeAsOptional("AF_RequestArmsUpdate");
    MarkNativeAsOptional("AF_DisableClientArmsUpdate");
    MarkNativeAsOptional("AF_IsClientArmsNotUpdating");
    MarkNativeAsOptional("AF_ForceArmsUpdate");
    MarkNativeAsOptional("AF_RefreshClientViewModel");
    MarkNativeAsOptional("AF_ApplyClientGloveSkin");
}
#endif