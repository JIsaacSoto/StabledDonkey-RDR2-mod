#include "main.h"
#include "natives.h"

#include "Config.h"
#include "Log.h"
#include "mount/MountAdapter.h"
#include "native/CarePrompts.h"
#include "native/ModelInfoPatch.h"
#include "native/ScriptPatch.h"
#include "native/SuperDonk.h"

namespace StabledDonkey
{
    namespace
    {
        constexpr const char *VERSION = "0.6.0";
        constexpr DWORD TICK_MS = 100;

        HMODULE g_module = nullptr;
        bool g_logStarted = false;

        // Last supported animal the player rode. Kept after dismounting so
        // the care prompts know which animal is "the" donkey.
        struct MountTracker
        {
            Ped mount = 0;
            int bondingPoints = -1;
            int bondingLevel = -1;
            bool riding = false;

            void Update()
            {
                const Ped player = PLAYER::PLAYER_PED_ID();
                const bool onMount = PED::IS_PED_ON_MOUNT(player);
                const Ped current = onMount ? PED::GET_MOUNT(player) : 0;

                if (onMount)
                {
                    const MountAdapter adapter(current);
                    if (!adapter.IsSupported())
                    {
                        mount = 0;
                        riding = false;
                        return;
                    }
                    if (current != mount)
                    {
                        mount = current;
                        bondingPoints = adapter.GetBondingPoints();
                        bondingLevel = adapter.GetBondingLevel();
                        char state[64];
                        Log("Mount: riding model=0x%08X bonding=%d level=%d %s", adapter.GetModel(), bondingPoints,
                            bondingLevel, NativeHorse::Describe(adapter.GetModel(), state, sizeof(state)));
                    }
                    riding = true;
                }
                else if (riding)
                {
                    riding = false;
                    Log("Mount: dismounted bonding=%d level=%d", bondingPoints, bondingLevel);
                }

                if (!mount || !ENTITY::DOES_ENTITY_EXIST(mount))
                {
                    mount = 0;
                    return;
                }
                const MountAdapter adapter(mount);
                const int points = adapter.GetBondingPoints();
                const int level = adapter.GetBondingLevel();
                if (points != bondingPoints || level != bondingLevel)
                {
                    bondingPoints = points;
                    bondingLevel = level;
                    Log("Mount: bonding=%d level=%d", points, level);
                }
            }
        };

        MountTracker g_mountTracker;

        void Run()
        {
            if (!g_logStarted)
            {
                g_logStarted = true;
                StartLogSession();
            }
            // ScriptHook restarts this function when a save is loaded.
            g_config.Load(g_module);
            Log("StabledDonkey %s started", VERSION);
            g_config.LogValues();
            g_mountTracker = MountTracker{};
            ScriptPatch::Reset();

            while (true)
            {
                NativeHorse::Update();
                ScriptPatch::Update();
                g_mountTracker.Update();
                CarePrompts::Update(g_mountTracker.mount);
                SuperDonk::Update(g_mountTracker.mount);
                WAIT(TICK_MS);
            }
        }
    }

    void SetModule(HMODULE module)
    {
        g_module = module;
    }
}

void ScriptMain()
{
    StabledDonkey::Run();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        StabledDonkey::SetModule(hModule);
        scriptRegister(hModule, ScriptMain);
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        scriptUnregister(ScriptMain);
    }
    return TRUE;
}
