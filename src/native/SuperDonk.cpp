#include "SuperDonk.h"

#include "Config.h"
#include "Log.h"
#include "mount/MountAdapter.h"

#include "main.h"
#include "natives.h"

#include <Windows.h>

#include <cstdint>
#include <cstdio>

// SuperDonkMode: raises the base attribute ranks (health, stamina, speed,
// acceleration) of the donkey or mule the player rides, with the same
// ATTRIBUTE natives the game uses for horses. Off by default.
//
// Base ranks, not bonus ranks: player_horse rewrites the speed and
// acceleration bonus ranks of the player's horse from its tack on every
// update (player_horse.c func_662..664), so the tack bonuses stack on top of
// these base ranks as on any horse. The stable shows every stat as base
// rank + 1 out of 10; the defaults (health 6, stamina 4, speed 5,
// acceleration 5) are the game's own default ranks of the Dark Bay Turkoman
// (health, stamina) and the White Arabian (speed, acceleration), read with
// GET_DEFAULT_ATTRIBUTE_RANK.
//
// Tested (1491.50): on horses and mules max health and max stamina follow
// 60 + 20 * rank. The donkey takes the ranks (speed and acceleration apply)
// but its max health (251) and stamina (300, about rank 4 in practice) are
// fixed by its model and do not change with the rank.
namespace StabledDonkey::SuperDonk
{
    namespace
    {
        // alloc8or/rdr3-nativedb-data
        constexpr uint64_t NATIVE_GET_ATTRIBUTE_BASE_RANK = 0x147149F2E909323C;
        constexpr uint64_t NATIVE_SET_ATTRIBUTE_BASE_RANK = 0x5DA12E025D47D4E5;
        constexpr uint64_t NATIVE_GET_MAX_ATTRIBUTE_RANK = 0x704674A0535A471D;
        constexpr uint64_t NATIVE_GET_DEFAULT_ATTRIBUTE_RANK = 0x958DD43D41F89A47;
        constexpr uint64_t NATIVE_GET_PED_MAX_HEALTH = 0x4700A416E8324EF3;
        constexpr uint64_t NATIVE_GET_PED_MAX_STAMINA = 0xCB42AFE2B613EE55;
        constexpr uint64_t NATIVE_GET_HASH_KEY = 0xFD340785ADF8CFB7;

        // ePedAttribute
        constexpr int PA_HEALTH = 0;
        constexpr int PA_STAMINA = 1;
        constexpr int PA_SPEED = 5;
        constexpr int PA_ACCELERATION = 6;

        constexpr ULONGLONG CHECK_MS = 1000;

        struct Stat
        {
            int attribute;
            const char *name;
        };
        constexpr Stat STATS[] = {
            {PA_HEALTH, "health"},
            {PA_STAMINA, "stamina"},
            {PA_SPEED, "speed"},
            {PA_ACCELERATION, "acceleration"},
        };

        ULONGLONG g_nextCheck = 0;
        bool g_referenceLogged = false;

        int Target(int attribute)
        {
            switch (attribute)
            {
            case PA_HEALTH:
                return g_config.superDonkHealth;
            case PA_STAMINA:
                return g_config.superDonkStamina;
            case PA_SPEED:
                return g_config.superDonkSpeed;
            case PA_ACCELERATION:
                return g_config.superDonkAcceleration;
            default:
                return -1;
            }
        }

        // The game's own default ranks of the reference breeds, once per
        // launch, so the ini defaults can be checked on other game builds.
        void LogReferenceRanks()
        {
            if (g_referenceLogged)
                return;
            g_referenceLogged = true;
            for (const char *name : {"A_C_HORSE_ARABIAN_WHITE", "A_C_HORSE_TURKOMAN_DARKBAY"})
            {
                const Hash model = invoke<Hash>(NATIVE_GET_HASH_KEY, name);
                int rank[4]{};
                for (int i = 0; i < 4; ++i)
                    rank[i] = invoke<int>(NATIVE_GET_DEFAULT_ATTRIBUTE_RANK, model, STATS[i].attribute);
                Log("SuperDonk: default ranks %s health=%d stamina=%d speed=%d acceleration=%d", name, rank[0],
                    rank[1], rank[2], rank[3]);
            }
        }

        void Apply(Ped ped)
        {
            const MountAdapter adapter(ped);
            if (!adapter.IsSupported() || ENTITY::IS_ENTITY_DEAD(ped))
                return;

            const int healthBefore = invoke<int>(NATIVE_GET_PED_MAX_HEALTH, ped);
            const float staminaBefore = invoke<float>(NATIVE_GET_PED_MAX_STAMINA, ped);
            bool changed = false;
            char detail[192]{};
            size_t used = 0;
            for (const Stat &stat : STATS)
            {
                int target = Target(stat.attribute);
                if (target < 0)
                    continue; // -1 in the ini: leave this stat alone
                const int max = invoke<int>(NATIVE_GET_MAX_ATTRIBUTE_RANK, ped, stat.attribute);
                if (max > 0 && target > max)
                    target = max;
                const int base = invoke<int>(NATIVE_GET_ATTRIBUTE_BASE_RANK, ped, stat.attribute);
                if (base == target)
                    continue;
                invoke<Void>(NATIVE_SET_ATTRIBUTE_BASE_RANK, ped, stat.attribute, target);
                const int now = invoke<int>(NATIVE_GET_ATTRIBUTE_BASE_RANK, ped, stat.attribute);
                if (used < sizeof(detail))
                    used += std::snprintf(detail + used, sizeof(detail) - used, " %s=%d->%d", stat.name, base, now);
                changed = true;
            }
            if (!changed)
                return;
            Log("SuperDonk: model=0x%08X%s max_health=%d->%d max_stamina=%.1f->%.1f", adapter.GetModel(), detail,
                healthBefore, invoke<int>(NATIVE_GET_PED_MAX_HEALTH, ped), staminaBefore,
                invoke<float>(NATIVE_GET_PED_MAX_STAMINA, ped));
        }
    }

    void Update(Ped trackedMount)
    {
        if (!g_config.superDonkMode)
            return;
        const ULONGLONG now = GetTickCount64();
        if (now < g_nextCheck)
            return;
        g_nextCheck = now + CHECK_MS;

        LogReferenceRanks();
        const Ped player = PLAYER::PLAYER_PED_ID();
        const Ped riding = PED::IS_PED_ON_MOUNT(player) ? PED::GET_MOUNT(player) : 0;
        if (riding)
            Apply(riding);
        if (trackedMount && trackedMount != riding && ENTITY::DOES_ENTITY_EXIST(trackedMount))
            Apply(trackedMount);
    }
}
