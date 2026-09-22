#include <cstdio>
#include <igameevents.h>
#include <iserver.h>
#include <IPlayerHelpers.h>
#include "extension.h"

#define ARMS_SZ_LEN 192
#define ARMS_FORCEUPDATE_TIMERDURATION 0.02
#define MaxClients 64

#define CALL_FWD(A, B) \
    m_pOnArmsUpdated->PushCell(A); \
    m_pOnArmsUpdated->PushCell(B); \
    m_pOnArmsUpdated->Execute(NULL)

IGameEventManager2 *gameevents = NULL;
IForward *m_pOnArmsUpdated = NULL;

int iSavedActiveWeapon[MaxClients + 1];
bool bPlayerDisableArmsUpdate[MaxClients + 1];

char szPlayerArmsModels_Default[ARMS_SZ_LEN] = "";
char szPlayerArmsModels[MaxClients + 1][ARMS_SZ_LEN];

int iArmsModelOffset = -1;
int iActiveWeaponOffset = -1;
int iGloveDefinitionOffset = -1;
int iGloveItemIdHighOffset = -1;
int iGlovePaintKitOffset = -1;
int iGloveSeedOffset = -1;
int iGloveWearOffset = -1;
int iGloveInitializedOffset = -1;

ArmsFix g_ArmsFix;
SMEXT_LINK(&g_ArmsFix);

SH_DECL_HOOK2(
    IVEngineServer,
    PrecacheModel,
    SH_NOATTRIB,
    0,
    int,
    const char *,
    bool
);

SH_DECL_HOOK3_void(
    IServerGameDLL,
    ServerActivate,
    SH_NOATTRIB,
    0,
    edict_t *,
    int,
    int
);

void PrecacheDefaultArms(bool preload = true)
{
    if (szPlayerArmsModels_Default[0] == '\0')
    {
        return;
    }

    engine->PrecacheModel(
        szPlayerArmsModels_Default,
        preload
    );
}

bool ArmsFix::SDK_OnMetamodLoad(
    ISmmAPI *ismm,
    char *error,
    size_t maxlen,
    bool late
)
{
    GET_V_IFACE_CURRENT(
        GetEngineFactory,
        engine,
        IVEngineServer,
        INTERFACEVERSION_VENGINESERVER
    );

    GET_V_IFACE_CURRENT(
        GetEngineFactory,
        gameevents,
        IGameEventManager2,
        INTERFACEVERSION_GAMEEVENTSMANAGER2
    );

    SH_ADD_HOOK(
        IVEngineServer,
        PrecacheModel,
        engine,
        SH_MEMBER(this, &ArmsFix::PrecacheModel),
        false
    );

    SH_ADD_HOOK(
        IServerGameDLL,
        ServerActivate,
        gamedll,
        SH_MEMBER(this, &ArmsFix::OnServerActivate),
        true
    );

    gameevents->AddListener(this, "player_spawn", true);
    gameevents->AddListener(this, "player_disconnect", true);
    gameevents->AddListener(this, "player_activate", true);

    return true;
}

void ArmsFix::FireGameEvent(IGameEvent *pEvent)
{
    if (!pEvent)
    {
        return;
    }

    const char *name = pEvent->GetName();

    int iClient = playerhelpers->GetClientOfUserId(
        pEvent->GetInt("userid")
    );

    if (iClient <= 0 || iClient > MaxClients)
    {
        return;
    }

    if (name[7] == 's')
    {
        if (bPlayerDisableArmsUpdate[iClient])
        {
            return;
        }

        CBaseEntity *pPlayer =
            gamehelpers->ReferenceToEntity(iClient);

        if (!pPlayer)
        {
            return;
        }

        CALL_FWD(iClient, 0);

        char *dest =
            (char *)((uint8_t *)pPlayer + iArmsModelOffset);

        if (szPlayerArmsModels[iClient][0])
        {
            ke::SafeStrcpy(
                dest,
                ARMS_SZ_LEN,
                szPlayerArmsModels[iClient]
            );
        }
        else
        {
            ke::SafeStrcpy(dest, ARMS_SZ_LEN, "");
        }

        return;
    }

    szPlayerArmsModels[iClient][0] = '\0';
    bPlayerDisableArmsUpdate[iClient] = false;
    iSavedActiveWeapon[iClient] = -1;
}

bool ArmsFix::SDK_OnLoad(
    char *error,
    size_t maxlength,
    bool late
)
{
    sm_sendprop_info_t info;

    if (!gamehelpers->FindSendPropInfo(
        "CCSPlayer",
        "m_szArmsModel",
        &info
    ))
    {
        Q_snprintf(
            error,
            maxlength,
            "Couldn't find CCSPlayer::m_szArmsModel offset!"
        );

        return false;
    }

    iArmsModelOffset = info.actual_offset;

    if (!gamehelpers->FindSendPropInfo(
        "CCSPlayer",
        "m_hActiveWeapon",
        &info
    ))
    {
        Q_snprintf(
            error,
            maxlength,
            "Couldn't find CCSPlayer::m_hActiveWeapon offset!"
        );

        return false;
    }

    iActiveWeaponOffset = info.actual_offset;

    const char *wearableClasses[] =
    {
        "CEconWearable",
        "CEconEntity",
        "CBaseAttributableItem"
    };

    for (int i = 0; i < 3; ++i)
    {
        if (iGloveDefinitionOffset < 0 && gamehelpers->FindSendPropInfo(wearableClasses[i], "m_iItemDefinitionIndex", &info))
        {
            iGloveDefinitionOffset = info.actual_offset;
        }
        if (iGloveItemIdHighOffset < 0 && gamehelpers->FindSendPropInfo(wearableClasses[i], "m_iItemIDHigh", &info))
        {
            iGloveItemIdHighOffset = info.actual_offset;
        }
        if (iGlovePaintKitOffset < 0 && gamehelpers->FindSendPropInfo(wearableClasses[i], "m_nFallbackPaintKit", &info))
        {
            iGlovePaintKitOffset = info.actual_offset;
        }
        if (iGloveSeedOffset < 0 && gamehelpers->FindSendPropInfo(wearableClasses[i], "m_nFallbackSeed", &info))
        {
            iGloveSeedOffset = info.actual_offset;
        }
        if (iGloveWearOffset < 0 && gamehelpers->FindSendPropInfo(wearableClasses[i], "m_flFallbackWear", &info))
        {
            iGloveWearOffset = info.actual_offset;
        }
        if (iGloveInitializedOffset < 0 && gamehelpers->FindSendPropInfo(wearableClasses[i], "m_bInitialized", &info))
        {
            iGloveInitializedOffset = info.actual_offset;
        }
    }

    m_pOnArmsUpdated = forwards->CreateForward(
        "AF_OnArmsUpdate",
        ET_Ignore,
        2,
        NULL,
        Param_Cell,
        Param_Cell
    );

    PrecacheDefaultArms(true);

    return true;
}

int ArmsFix::PrecacheModel(
    const char *model,
    bool precache
)
{
    if (!model)
    {
        return META_RESULT_ORIG_RET(int);
    }

    if (
        V_strncmp(model, "models/weapons/v_models/arms/glove_har", 38) == 0
        || V_strncmp(model, "models/weapons/v_models/arms/glove_f", 36) == 0
        || V_strncmp(model, "models/weapons/v_models/arms/ph", 31) == 0
    )
    {
        RETURN_META_VALUE(MRES_SUPERCEDE, 0);
    }

    RETURN_META_VALUE(MRES_IGNORED, 0);
}

void ArmsFix::OnServerActivate(
    edict_t *pEdictList,
    int edictCount,
    int clientMax
)
{
    PrecacheDefaultArms(true);
}

void ArmsFix::SDK_OnUnload()
{
    SH_REMOVE_HOOK(
        IVEngineServer,
        PrecacheModel,
        engine,
        SH_MEMBER(this, &ArmsFix::PrecacheModel),
        false
    );

    SH_REMOVE_HOOK(
        IServerGameDLL,
        ServerActivate,
        gamedll,
        SH_MEMBER(this, &ArmsFix::OnServerActivate),
        true
    );

    if (m_pOnArmsUpdated)
    {
        forwards->ReleaseForward(m_pOnArmsUpdated);
        m_pOnArmsUpdated = NULL;
    }

    gameevents->RemoveListener(this);
}

cell_t sm_AF_Version(
    IPluginContext *pContext,
    const cell_t *params
)
{
    return SMEXT_CONF_CUSTOM_VERCODE;
}

cell_t sm_AF_SetDefaultArmsModel(
    IPluginContext *pContext,
    const cell_t *params
)
{
    char *szMdlPath;

    pContext->LocalToString(
        params[1],
        &szMdlPath
    );

    if (!szMdlPath || !szMdlPath[0])
    {
        szPlayerArmsModels_Default[0] = '\0';
        return 1;
    }

    ke::SafeStrcpy(
        szPlayerArmsModels_Default,
        ARMS_SZ_LEN,
        szMdlPath
    );

    engine->PrecacheModel(
        szPlayerArmsModels_Default,
        true
    );

    return 1;
}

cell_t sm_AF_ResetDefaultArmsModel(
    IPluginContext *pContext,
    const cell_t *params
)
{
    szPlayerArmsModels_Default[0] = '\0';
    return 1;
}

cell_t sm_AF_HasClientCustomArms(
    IPluginContext *pContext,
    const cell_t *params
)
{
    int iClient = params[1];

    if (iClient < 1 || iClient > MaxClients)
    {
        return pContext->ThrowNativeError(
            "Wrong client index (%d)",
            iClient
        );
    }

    return szPlayerArmsModels[iClient][0] != '\0';
}

cell_t sm_AF_SetClientArmsModel(
    IPluginContext *pContext,
    const cell_t *params
)
{
    int iClient = params[1];

    if (iClient < 1 || iClient > MaxClients)
    {
        return pContext->ThrowNativeError(
            "Wrong client index (%d)",
            iClient
        );
    }

    char *szMdlPath;

    pContext->LocalToString(
        params[2],
        &szMdlPath
    );

    if (!szMdlPath || !szMdlPath[0])
    {
        return pContext->ThrowNativeError(
            "Arms model path cannot be empty"
        );
    }

    engine->PrecacheModel(
        szMdlPath,
        true
    );

    CALL_FWD(iClient, 4);

    ke::SafeStrcpy(
        szPlayerArmsModels[iClient],
        ARMS_SZ_LEN,
        szMdlPath
    );

    return 1;
}

cell_t sm_AF_RemoveClientArmsModel(
    IPluginContext *pContext,
    const cell_t *params
)
{
    int iClient = params[1];

    if (iClient < 1 || iClient > MaxClients)
    {
        return pContext->ThrowNativeError(
            "Wrong client index (%d)",
            iClient
        );
    }

    szPlayerArmsModels[iClient][0] = '\0';

    return 1;
}

cell_t sm_AF_GetClientArmsModel(
    IPluginContext *pContext,
    const cell_t *params
)
{
    int iClient = params[1];

    if (iClient < 1 || iClient > MaxClients)
    {
        return pContext->ThrowNativeError(
            "Wrong client index (%d)",
            iClient
        );
    }

    const char *model =
        szPlayerArmsModels[iClient][0]
            ? szPlayerArmsModels[iClient]
            : szPlayerArmsModels_Default;

    size_t len;

    pContext->StringToLocalUTF8(
        params[2],
        params[3],
        model,
        &len
    );

    return len;
}

cell_t sm_AF_ApplyClientGloveSkin(
    IPluginContext *pContext,
    const cell_t *params
)
{
    int iClient = params[1];
    int iGlove = params[2];

    if (iClient < 1 || iClient > MaxClients)
    {
        return pContext->ThrowNativeError("Wrong client index (%d)", iClient);
    }

    CBaseEntity *pGlove = gamehelpers->ReferenceToEntity(iGlove);
    if (!pGlove)
    {
        return pContext->ThrowNativeError("Glove entity (%d) is invalid", iGlove);
    }

    if (iGloveDefinitionOffset < 0 || iGlovePaintKitOffset < 0 || iGloveWearOffset < 0)
    {
        return false;
    }

    uint8_t *base = (uint8_t *)pGlove;
    if (iGloveDefinitionOffset >= 0)
    {
        *(int *)(base + iGloveDefinitionOffset) = params[3];
    }
    if (iGloveItemIdHighOffset >= 0)
    {
        *(int *)(base + iGloveItemIdHighOffset) = 0;
    }
    if (iGlovePaintKitOffset >= 0)
    {
        *(int *)(base + iGlovePaintKitOffset) = params[4];
    }
    if (iGloveSeedOffset >= 0)
    {
        *(int *)(base + iGloveSeedOffset) = params[5];
    }
    if (iGloveWearOffset >= 0)
    {
        union
        {
            cell_t cell;
            float value;
        } wear;

        wear.cell = params[6];
        *(float *)(base + iGloveWearOffset) = wear.value;
    }
    if (iGloveInitializedOffset >= 0)
    {
        *(bool *)(base + iGloveInitializedOffset) = true;
    }

    return true;
}

cell_t sm_AF_GetDefaultArmsModel(
    IPluginContext *pContext,
    const cell_t *params
)
{
    size_t len;

    pContext->StringToLocalUTF8(
        params[1],
        params[2],
        szPlayerArmsModels_Default,
        &len
    );

    return len;
}

static void FrameAction_SetClientActiveWeapon_End(
    void *pData
)
{
    int iClient = (uintptr_t)pData;

    if (iClient < 1 || iClient > MaxClients)
    {
        return;
    }

    if (iSavedActiveWeapon[iClient] < 64)
    {
        return;
    }

    CBaseEntity *pPlayer =
        gamehelpers->ReferenceToEntity(iClient);

    if (!pPlayer)
    {
        return;
    }

    CBaseHandle &hndl =
        *(CBaseHandle *)(
            (uint8_t *)pPlayer + iActiveWeaponOffset
        );

    CBaseEntity *pOther =
        gamehelpers->ReferenceToEntity(
            iSavedActiveWeapon[iClient]
        );

    if (pOther)
    {
        hndl.Set((IHandleEntity *)pOther);
    }
}

static void FrameAction_SetClientActiveWeapon_Middle(
    void *pData
)
{
    smutils->AddFrameAction(
        FrameAction_SetClientActiveWeapon_End,
        pData
    );
}

static void FrameAction_SetClientActiveWeapon(
    void *pData
)
{
    smutils->AddFrameAction(
        FrameAction_SetClientActiveWeapon_Middle,
        pData
    );
}

cell_t sm_AF_RequestArmsUpdate(
    IPluginContext *pContext,
    const cell_t *params
)
{
    int iClient = params[1];

    if (iClient < 1 || iClient > MaxClients)
    {
        return pContext->ThrowNativeError(
            "Wrong client index (%d)",
            iClient
        );
    }

    CBaseEntity *pPlayer =
        gamehelpers->ReferenceToEntity(iClient);

    if (!pPlayer)
    {
        return pContext->ThrowNativeError(
            "Client (%d) is not connected or missing",
            iClient
        );
    }

    if (bPlayerDisableArmsUpdate[iClient])
    {
        CALL_FWD(iClient, 2);

        ke::SafeStrcpy(
            (char *)((uint8_t *)pPlayer + iArmsModelOffset),
            ARMS_SZ_LEN,
            ""
        );

        return false;
    }

    if (params[2])
    {
        CBaseHandle &hndl =
            *(CBaseHandle *)(
                (uint8_t *)pPlayer + iActiveWeaponOffset
            );

        CBaseEntity *pHandleEntity =
            gamehelpers->ReferenceToEntity(
                hndl.GetEntryIndex()
            );

        if (
            pHandleEntity &&
            hndl == reinterpret_cast<IHandleEntity *>(
                pHandleEntity
            )->GetRefEHandle()
        )
        {
            iSavedActiveWeapon[iClient] =
                gamehelpers->EntityToBCompatRef(
                    pHandleEntity
                );

            hndl.Set(NULL);

            smutils->AddFrameAction(
                FrameAction_SetClientActiveWeapon,
                (uintptr_t *)iClient
            );
        }
    }

    CALL_FWD(iClient, 1);

    ke::SafeStrcpy(
        (char *)((uint8_t *)pPlayer + iArmsModelOffset),
        ARMS_SZ_LEN,
        szPlayerArmsModels[iClient][0]
            ? szPlayerArmsModels[iClient]
            : szPlayerArmsModels_Default
    );

    return true;
}

cell_t sm_AF_DisableClientArmsUpdate(
    IPluginContext *pContext,
    const cell_t *params
)
{
    int iClient = params[1];

    if (iClient < 1 || iClient > MaxClients)
    {
        return pContext->ThrowNativeError(
            "Wrong client index (%d)",
            iClient
        );
    }

    CBaseEntity *pPlayer =
        gamehelpers->ReferenceToEntity(iClient);

    if (!pPlayer)
    {
        return pContext->ThrowNativeError(
            "Client (%d) is not connected or missing",
            iClient
        );
    }

    bPlayerDisableArmsUpdate[iClient] = params[2];

    if (params[2] && params[3])
    {
        CALL_FWD(iClient, 3);

        ke::SafeStrcpy(
            (char *)((uint8_t *)pPlayer + iArmsModelOffset),
            ARMS_SZ_LEN,
            ""
        );
    }

    return 1;
}

cell_t sm_AF_IsClientArmsNotUpdating(
    IPluginContext *pContext,
    const cell_t *params
)
{
    int iClient = params[1];

    if (iClient < 1 || iClient > MaxClients)
    {
        return pContext->ThrowNativeError(
            "Wrong client index (%d)",
            iClient
        );
    }

    return bPlayerDisableArmsUpdate[iClient];
}

cell_t sm_AF_ForceArmsUpdate(
    IPluginContext *pContext,
    const cell_t *params
)
{
    int iClient = params[1];

    if (iClient < 1 || iClient > MaxClients)
    {
        return pContext->ThrowNativeError(
            "Wrong client index (%d)",
            iClient
        );
    }

    if (
        bPlayerDisableArmsUpdate[iClient] &&
        !params[2]
    )
    {
        return false;
    }

    CBaseEntity *pPlayer =
        gamehelpers->ReferenceToEntity(iClient);

    if (!pPlayer)
    {
        return pContext->ThrowNativeError(
            "Client (%d) is not connected or missing",
            iClient
        );
    }

    CBaseHandle &hndl =
        *(CBaseHandle *)(
            (uint8_t *)pPlayer + iActiveWeaponOffset
        );

    CBaseEntity *pHandleEntity =
        gamehelpers->ReferenceToEntity(
            hndl.GetEntryIndex()
        );

    if (
        pHandleEntity &&
        hndl == reinterpret_cast<IHandleEntity *>(
            pHandleEntity
        )->GetRefEHandle()
    )
    {
        iSavedActiveWeapon[iClient] =
            gamehelpers->EntityToBCompatRef(
                pHandleEntity
            );

        hndl.Set(NULL);

        smutils->AddFrameAction(
            FrameAction_SetClientActiveWeapon,
            (uintptr_t *)iClient
        );

        return true;
    }

    return false;
}

const sp_nativeinfo_t NativesList[] =
{
    {"AF_Version", sm_AF_Version},
    {"AF_SetDefaultArmsModel", sm_AF_SetDefaultArmsModel},
    {"AF_ResetDefaultArmsModel", sm_AF_ResetDefaultArmsModel},
    {"AF_SetClientArmsModel", sm_AF_SetClientArmsModel},
    {"AF_HasClientCustomArms", sm_AF_HasClientCustomArms},
    {"AF_RemoveClientArmsModel", sm_AF_RemoveClientArmsModel},
    {"AF_GetClientArmsModel", sm_AF_GetClientArmsModel},
    {"AF_GetDefaultArmsModel", sm_AF_GetDefaultArmsModel},
    {"AF_RequestArmsUpdate", sm_AF_RequestArmsUpdate},
    {"AF_DisableClientArmsUpdate", sm_AF_DisableClientArmsUpdate},
    {"AF_IsClientArmsNotUpdating", sm_AF_IsClientArmsNotUpdating},
    {"AF_ForceArmsUpdate", sm_AF_ForceArmsUpdate},
    {"AF_RefreshClientViewModel", sm_AF_ForceArmsUpdate},
    {"AF_ApplyClientGloveSkin", sm_AF_ApplyClientGloveSkin},
    {NULL, NULL},
};

void ArmsFix::SDK_OnAllLoaded()
{
    sharesys->AddNatives(
        myself,
        NativesList
    );
}