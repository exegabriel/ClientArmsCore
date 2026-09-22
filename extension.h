#ifndef _A2SFIREWALL_EXTENSION_H_
#define _A2SFIREWALL_EXTENSION_H_

#include "smsdk_ext.h"

class ArmsFix : public SDKExtension, public IGameEventListener2
{
public:

    // SDKExtension
    virtual bool SDK_OnLoad(char *error, size_t maxlength, bool late);
    virtual void SDK_OnUnload();
    virtual void SDK_OnAllLoaded();

    // Hooks
    int PrecacheModel(const char *model, bool precache);
    bool OnEventFire(IGameEvent *event, bool bDontBroadcast);
    //bool OnEventFire_Post(IGameEvent *event, bool bDontBroadcast);
    void OnServerActivate(edict_t *pEdictList, int edictCount, int clientMax);

    // Metamod
    virtual bool SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late);

    // Events
    virtual void FireGameEvent(IGameEvent *event);
    virtual int GetEventDebugID(void) {return 42; }
};
#endif