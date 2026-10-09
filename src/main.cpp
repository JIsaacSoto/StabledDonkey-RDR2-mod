#include "main.h"
#include "natives.h"

#include "mount/MountAdapter.h"

#include <cstdarg>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <atomic>

namespace StabledDonkey
{
    static Ped g_observedMount = 0;
    static int g_lastBondingPoints = -1;
    static int g_lastBondingLevel = -1;
    static bool g_playerWasMounted = false;
    static std::atomic<Ped> g_bondingOverridePed{0};
    static std::atomic<bool> g_bondingOverrideActive{false};
    static std::atomic<uint64_t> g_modelAliasHits{0};
    static std::atomic<uint64_t> g_activeModelQueries{0};
    static std::atomic<uint64_t> g_supportedModelsSeen{0};
    static uint64_t g_loggedModelAliasHits = 0;
    static uint64_t g_loggedActiveModelQueries = 0;
    static void *g_originalGetEntityModel = nullptr;

    constexpr uintptr_t GET_ENTITY_MODEL_HANDLER_RVA = 0xF8BCDC;
    constexpr size_t GET_ENTITY_MODEL_PATCH_SIZE = 15;
    constexpr Hash BONDING_ELIGIBLE_MODEL = 0x400B3937;
    constexpr Hash DONKEY_MODEL = 0x69A37A7B;
    constexpr Hash MULE_MODEL = 0xB6A7CE35;
    constexpr Hash PAINTED_MULE_MODEL = 0x23685521;
    constexpr float BONDING_OVERRIDE_RANGE = 4.0f;
    void Log(const char *format, ...);

    void WriteAbsoluteJump(uint8_t *destination, const void *target)
    {
        destination[0] = 0xFF; // jmp qword ptr [rip]
        destination[1] = 0x25;
        destination[2] = 0;
        destination[3] = 0;
        destination[4] = 0;
        destination[5] = 0;
        const uint64_t address = reinterpret_cast<uint64_t>(target);
        std::memcpy(destination + 6, &address, sizeof(address));
    }

    using NativeHandler = void (*)(void *nativeContext);

    bool IsSupportedMountModel(Hash model)
    {
        return model == DONKEY_MODEL || model == MULE_MODEL || model == PAINTED_MULE_MODEL;
    }

    void GetEntityModelHook(void *nativeContext)
    {
        const auto original = reinterpret_cast<NativeHandler>(g_originalGetEntityModel);
        if (!nativeContext || !original)
            return;

        original(nativeContext);
        if (IsMountAdapterModelQueryActive() || !g_bondingOverrideActive.load(std::memory_order_relaxed))
            return;

        auto *contextBytes = static_cast<uint8_t *>(nativeContext);
        const auto arguments = *reinterpret_cast<uint64_t **>(contextBytes + 0x10);
        auto *returnValues = *reinterpret_cast<uint64_t **>(contextBytes);
        if (!arguments || !returnValues)
            return;

        g_activeModelQueries.fetch_add(1, std::memory_order_relaxed);
        const Hash model = static_cast<Hash>(*returnValues);
        if (IsSupportedMountModel(model))
        {
            // The vanilla care reward path may resolve a mount through a
            // different script handle than GET_MOUNT. Gate on the actual model
            // returned by the native instead of assuming both handles match.
            // This changes only the eligibility result; vanilla still owns the
            // action, XP award and toast.
            *reinterpret_cast<Hash *>(returnValues) = BONDING_ELIGIBLE_MODEL;
            g_modelAliasHits.fetch_add(1, std::memory_order_relaxed);
            g_supportedModelsSeen.fetch_add(1, std::memory_order_relaxed);
        }
    }

    bool InstallBondingGateHook()
    {
        auto *imageBase = reinterpret_cast<uint8_t *>(GetModuleHandleW(nullptr));
        if (!imageBase)
        {
            Log("Bonding gate hook failed: game image unavailable");
            return false;
        }

        auto *target = imageBase + GET_ENTITY_MODEL_HANDLER_RVA;
        constexpr uint8_t expectedPrologue[GET_ENTITY_MODEL_PATCH_SIZE] = {
            0x40, 0x53,                         // push rbx
            0x48, 0x83, 0xEC, 0x20,             // sub rsp, 20h
            0x48, 0x8B, 0x41, 0x10,             // mov rax, [rcx+10h]
            0x48, 0x8B, 0xD9,                   // mov rbx, rcx
            0x8B, 0x08                          // mov ecx, [rax]
        };

        MEMORY_BASIC_INFORMATION memoryInfo{};
        if (VirtualQuery(target, &memoryInfo, sizeof(memoryInfo)) == 0 ||
            std::memcmp(target, expectedPrologue, sizeof(expectedPrologue)) != 0)
        {
            Log("Bonding gate hook refused: GET_ENTITY_MODEL handler bytes do not match the inspected build");
            return false;
        }

        auto *trampoline = static_cast<uint8_t *>(VirtualAlloc(
            nullptr, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
        if (!trampoline)
        {
            Log("Bonding gate hook failed: trampoline allocation error=%lu", GetLastError());
            return false;
        }

        std::memcpy(trampoline, target, GET_ENTITY_MODEL_PATCH_SIZE);
        WriteAbsoluteJump(trampoline + GET_ENTITY_MODEL_PATCH_SIZE,
                          target + GET_ENTITY_MODEL_PATCH_SIZE);

        DWORD oldProtection = 0;
        if (!VirtualProtect(target, GET_ENTITY_MODEL_PATCH_SIZE, PAGE_EXECUTE_READWRITE, &oldProtection))
        {
            VirtualFree(trampoline, 0, MEM_RELEASE);
            Log("Bonding gate hook failed: VirtualProtect error=%lu", GetLastError());
            return false;
        }

        g_originalGetEntityModel = trampoline;
        uint8_t replacement[GET_ENTITY_MODEL_PATCH_SIZE]{};
        WriteAbsoluteJump(replacement, reinterpret_cast<const void *>(GetEntityModelHook));
        replacement[sizeof(replacement) - 1] = 0x90;
        std::memcpy(target, replacement, sizeof(replacement));
        FlushInstructionCache(GetCurrentProcess(), target, sizeof(replacement));

        DWORD ignored = 0;
        VirtualProtect(target, GET_ENTITY_MODEL_PATCH_SIZE, oldProtection, &ignored);
        Log("Bonding gate hook installed: handler=%p trampoline=%p model_alias=0x%08X",
            target,
            trampoline,
            BONDING_ELIGIBLE_MODEL);
        return true;
    }

    void RefreshBondingOverride(Ped player, Ped mount, bool mounted)
    {
        if (mount == 0 || !ENTITY::DOES_ENTITY_EXIST(mount))
        {
            g_bondingOverridePed.store(0, std::memory_order_relaxed);
            g_bondingOverrideActive.store(false, std::memory_order_relaxed);
            return;
        }

        const MountAdapter adapter(mount);
        if (!adapter.IsSupported())
        {
            g_bondingOverridePed.store(0, std::memory_order_relaxed);
            g_bondingOverrideActive.store(false, std::memory_order_relaxed);
            return;
        }

        g_bondingOverridePed.store(mount, std::memory_order_relaxed);
        bool active = mounted;
        if (!mounted)
        {
            const Vector3 playerPosition = ENTITY::GET_ENTITY_COORDS(player, TRUE, FALSE);
            const Vector3 mountPosition = ENTITY::GET_ENTITY_COORDS(mount, TRUE, FALSE);
            const float dx = playerPosition.x - mountPosition.x;
            const float dy = playerPosition.y - mountPosition.y;
            const float dz = playerPosition.z - mountPosition.z;
            active = (dx * dx + dy * dy + dz * dz) <=
                     (BONDING_OVERRIDE_RANGE * BONDING_OVERRIDE_RANGE);
        }

        g_bondingOverrideActive.store(active, std::memory_order_relaxed);
    }

    void ObserveBonding(const MountAdapter &adapter)
    {
        const int bondingPoints = adapter.GetBondingPoints();
        const int bondingLevel = adapter.GetBondingLevel();

        if (bondingPoints != g_lastBondingPoints || bondingLevel != g_lastBondingLevel)
        {
            Log("Mount bonding changed: ped=%d bonding_points=%d bonding_level=%d",
                adapter.GetPed(),
                bondingPoints,
                bondingLevel);

            g_lastBondingPoints = bondingPoints;
            g_lastBondingLevel = bondingLevel;
        }
    }

    void Log(const char *format, ...)
    {
        FILE *file = nullptr;
        fopen_s(&file, "StabledDonkey.log", "a");
        if (!file)
            return;

        va_list args;
        va_start(args, format);
        vfprintf(file, format, args);
        fprintf(file, "\n");
        va_end(args);
        fclose(file);
    }

    void InspectBondingGateNative()
    {
        // Build 1491.50 handler map: ENTITY::GET_ENTITY_MODEL at RVA 0xF8BCDC.
        // The file image is protected/encoded; inspect the live, executable
        // image before installing any detour so its prologue can be decoded.
        constexpr uintptr_t GET_ENTITY_MODEL_HANDLER_RVA = 0xF8BCDC;
        const auto imageBase = reinterpret_cast<uint8_t *>(GetModuleHandleW(nullptr));
        const auto target = imageBase + GET_ENTITY_MODEL_HANDLER_RVA;
        MEMORY_BASIC_INFORMATION memoryInfo{};
        if (VirtualQuery(target, &memoryInfo, sizeof(memoryInfo)) == 0)
        {
            Log("Bonding gate inspection failed: VirtualQuery error=%lu",
                GetLastError());
            return;
        }

        const DWORD pageProtection = memoryInfo.Protect & 0xFF;
        const bool executablePage = pageProtection == PAGE_EXECUTE ||
                                    pageProtection == PAGE_EXECUTE_READ ||
                                    pageProtection == PAGE_EXECUTE_READWRITE ||
                                    pageProtection == PAGE_EXECUTE_WRITECOPY;
        const auto regionStart = reinterpret_cast<uintptr_t>(memoryInfo.BaseAddress);
        const auto targetAddress = reinterpret_cast<uintptr_t>(target);
        if (memoryInfo.State != MEM_COMMIT || !executablePage ||
            targetAddress < regionStart ||
            memoryInfo.RegionSize - (targetAddress - regionStart) < 32)
        {
            Log("Bonding gate inspection skipped: target is not a readable executable region");
            return;
        }

        char bytes[3 * 32 + 1]{};
        char *cursor = bytes;
        for (int i = 0; i < 32; ++i)
            cursor += sprintf_s(cursor, 4, "%02X ", target[i]);

        Log("Bonding gate native candidate: rva=0x%08X address=%p region=%p region_size=%llu protection=0x%lX bytes=%s",
            static_cast<unsigned>(GET_ENTITY_MODEL_HANDLER_RVA),
            target,
            memoryInfo.BaseAddress,
            static_cast<unsigned long long>(memoryInfo.RegionSize),
            memoryInfo.Protect,
            bytes);
    }

    void InspectCurrentMount()
    {
        const Ped player = PLAYER::PLAYER_PED_ID();
        if (PED::IS_PED_ON_MOUNT(player))
        {
            const Ped mount = PED::GET_MOUNT(player);
            const MountAdapter adapter(mount);
            if (!adapter.IsSupported())
            {
                g_bondingOverridePed.store(0, std::memory_order_relaxed);
                g_bondingOverrideActive.store(false, std::memory_order_relaxed);
                return;
            }

            if (mount != g_observedMount)
            {
                g_observedMount = mount;
                g_lastBondingPoints = adapter.GetBondingPoints();
                g_lastBondingLevel = adapter.GetBondingLevel();

                Log("Mount detected: type=%d ped=%d model=0x%08X bonding_points=%d bonding_level=%d",
                    static_cast<int>(adapter.GetType()),
                    adapter.GetPed(),
                    adapter.GetModel(),
                    g_lastBondingPoints,
                    g_lastBondingLevel);
            }

            RefreshBondingOverride(player, mount, true);
            g_playerWasMounted = true;
            ObserveBonding(adapter);
            return;
        }

        if (g_playerWasMounted)
        {
            Log("Mount dismounted: ped=%d bonding_points=%d bonding_level=%d",
                g_observedMount,
                g_lastBondingPoints,
                g_lastBondingLevel);
            g_playerWasMounted = false;
        }

        if (g_observedMount == 0 || !ENTITY::DOES_ENTITY_EXIST(g_observedMount))
        {
            g_observedMount = 0;
            g_lastBondingPoints = -1;
            g_lastBondingLevel = -1;
            RefreshBondingOverride(player, 0, false);
            return;
        }

        const MountAdapter adapter(g_observedMount);
        if (adapter.IsSupported())
        {
            RefreshBondingOverride(player, g_observedMount, false);
            ObserveBonding(adapter);
        }
        else
        {
            RefreshBondingOverride(player, 0, false);
        }
    }

    void ScriptMain()
    {
        Log("StabledDonkey iniciado.");
        InspectBondingGateNative();
        InstallBondingGateHook();
        while (true)
        {
            InspectCurrentMount();
            const uint64_t aliasHits = g_modelAliasHits.load(std::memory_order_relaxed);
            if (aliasHits != g_loggedModelAliasHits)
            {
                Log("Bonding model gate bypass active: ped=%d active=%d vanilla_model_queries=%llu",
                    g_bondingOverridePed.load(std::memory_order_relaxed),
                    g_bondingOverrideActive.load(std::memory_order_relaxed) ? 1 : 0,
                    static_cast<unsigned long long>(aliasHits));
                g_loggedModelAliasHits = aliasHits;
            }

            const uint64_t activeQueries = g_activeModelQueries.load(std::memory_order_relaxed);
            if (activeQueries != g_loggedActiveModelQueries)
            {
                Log("Bonding gate diagnostics: tracked_mount=%d active=%d get_entity_model_calls=%llu target_models_seen=%llu",
                    g_bondingOverridePed.load(std::memory_order_relaxed),
                    g_bondingOverrideActive.load(std::memory_order_relaxed) ? 1 : 0,
                    static_cast<unsigned long long>(activeQueries),
                    static_cast<unsigned long long>(g_supportedModelsSeen.load(std::memory_order_relaxed)));
                g_loggedActiveModelQueries = activeQueries;
            }
            WAIT(1000);
        }
    }
}

void ScriptMain()
{
    StabledDonkey::ScriptMain();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
        scriptRegister(hModule, ScriptMain);
    else if (reason == DLL_PROCESS_DETACH)
        scriptUnregister(ScriptMain);

    return TRUE;
}
