#include "ModelInfoPatch.h"

#include "Config.h"
#include "Log.h"
#include "Memory.h"
#include "mount/MountAdapter.h"

#include <Windows.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// Model data patch. In metapeds.ymt the horse, the mule and the donkey differ
// in a few CPedModelInfo fields, most importantly the animal type (AT_HORSE /
// AT_MULE / AT_DONKEY). This module finds those fields in the live model
// infos and writes the horse values into the donkey (and, optionally, the
// mules). No game file is touched.
//
// Offsets are never hardcoded: they are discovered by comparing the donkey,
// the mule and the Tennessee Walker model infos, and a field is only written
// when exactly one offset matches and the current value is the expected one.
namespace StabledDonkey::NativeHorse
{
    namespace
    {
        constexpr Hash WALKER_MODEL = 0x400B3937; // A_C_Horse_TennesseeWalker_Chestnut (reference horse)

        constexpr uint32_t AT_HORSE = 0xF6DB2E94;  // joaat("AT_HORSE")
        constexpr uint32_t AT_MULE = 0x2BA9553F;   // joaat("AT_MULE")
        constexpr uint32_t AT_DONKEY = 0x1F023F3D; // joaat("AT_DONKEY")
        constexpr uint32_t HASH_HORSE = 0x95A6F147;
        constexpr uint32_t HASH_DONKEY = 0xCFFC8D0B;
        constexpr uint32_t HASH_DEFAULT = 0xE4DF46D5;

        // CPedModelInfo is 0x380 bytes in 1491.50; a wider window reaches the
        // next model info and makes matches ambiguous.
        constexpr size_t WINDOW_BYTES = 0x380;
        constexpr size_t HEADER_BYTES = 0x100;
        constexpr size_t NOT_FOUND = SIZE_MAX;
        constexpr int MAX_ATTEMPTS = 30;
        constexpr ULONGLONG RETRY_MS = 2000;
        constexpr ULONGLONG RECHECK_MS = 5000;

        // CModelInfo lookup (hash, out index) -> CBaseModelInfo*. Signature
        // from the public RDR2Playground project. Only trusted when unique in
        // the image and when the returned object contains the model hash.
        constexpr const char *LOOKUP_SIGNATURE =
            "48 89 5C 24 ? 89 4C 24 08 57 48 83 EC 20 33 DB 48 8B FA 85 C9 74 73 48 8D 54 24 ? 48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8D 48 04 48 F7 D8 48 1B D2 48 23 D1 74 53 8B 0A 0F B7 C1 48 85 FF 74 02 89 07 3B 05 ? ? ? ? 7D 2F 83 E1 1F 4C 8B C0 8B D1 48 8B C8 48 8B 05 ? ? ? ? 48 C1 E9 05 8B 0C 88 0F A3 D1 73 11 4C 0F AF 05 ? ? ? ? 4C 03 05 ? ? ? ? EB 03 4C 8B C3 4D 85";

        using GetModelInfoFn = uintptr_t (*)(Hash model, int *index);

        enum class Phase
        {
            Resolve,
            Discover,
            Patched,
            Failed
        };

        struct Offsets
        {
            size_t animalType = NOT_FOUND; // metapeds 0x9ED31CFA
            size_t horseHashA = NOT_FOUND; // metapeds 0x7BA672C7 (HORSE / empty / DONKEY)
            size_t horseHashB = NOT_FOUND; // metapeds 0x3CF2AF47 (HORSE / default / default)
            size_t floats = NOT_FOUND;     // metapeds 0xB6F6DE8E (25/50/100 vs 0/0/0)
            size_t flagByte = NOT_FOUND;   // metapeds 0xF45D226F (1 vs 0)
        };

        // Donkey-only fields set to the horse value (the mule already has it).
        // Tested together in game (2026-11-25): with them the donkey can be
        // patted while riding and behaves normally. Each one is located by
        // value: donkey value on the donkey, horse value on horse and mule.
        struct DonkeyField
        {
            const char *name;
            uint32_t donkeyValue;
            uint32_t horseValue;
            size_t offset;
        };
        DonkeyField g_donkeyFields[] = {
            {"MotionTaskDataSet", HASH_DONKEY, HASH_HORSE, NOT_FOUND}, // first DONKEY -> HORSE task field
            {"TaskData", HASH_DONKEY, HASH_HORSE, NOT_FOUND},          // second DONKEY -> HORSE task field
            {"DecisionMaker", 0x98D8BAF4, 0xC87F6A22, NOT_FOUND},
            {"Creature3207", 0x3ADB3357, 0xE4DF46D5, NOT_FOUND},       // metapeds 0x3207FDF6
            {"InteractionC4", 0x878D0AD0, HASH_HORSE, NOT_FOUND},      // metapeds 0xC4E3A8E3
            {"Interaction27", 0xA122F760, HASH_HORSE, NOT_FOUND},      // metapeds 0x2799400B
        };

        struct Target
        {
            Hash model;
            const char *name;
            uint32_t originalAnimalType;
            bool isDonkey;
            uintptr_t info = 0;
            bool enabled = false;
            int reapplied = 0;
        };

        Phase g_phase = Phase::Resolve;
        GetModelInfoFn g_getModelInfo = nullptr;
        ULONGLONG g_nextAttemptAt = 0;
        int g_attempts = 0;
        Offsets g_offsets{};
        uint32_t g_muleHashA = 0;
        Target g_targets[] = {
            {Models::DONKEY, "A_C_Donkey_01", AT_DONKEY, true},
            {Models::MULE, "A_C_HorseMule_01", AT_MULE, false},
            {Models::PAINTED_MULE, "A_C_HorseMulePainted_01", AT_MULE, false},
        };

        const char *AnimalTypeName(uint32_t value)
        {
            switch (value)
            {
            case AT_HORSE:
                return "AT_HORSE";
            case AT_MULE:
                return "AT_MULE";
            case AT_DONKEY:
                return "AT_DONKEY";
            default:
                return "?";
            }
        }

        bool CallLookup(Hash model, uintptr_t &info)
        {
            info = 0;
            int index = 0xFFFF;
            __try
            {
                info = g_getModelInfo(model, &index);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return false;
            }
            return true;
        }

        uintptr_t GetModelInfo(Hash model)
        {
            uintptr_t info = 0;
            if (!g_getModelInfo || !CallLookup(model, info) || !Memory::IsReadable(info, HEADER_BYTES))
                return 0;
            for (size_t offset = 0; offset < HEADER_BYTES; offset += 4)
            {
                uint32_t value = 0;
                std::memcpy(&value, reinterpret_cast<const void *>(info + offset), 4);
                if (value == model)
                    return info;
            }
            return 0;
        }

        std::vector<uint8_t> Snapshot(uintptr_t info)
        {
            const size_t size = Memory::ReadableSpan(info, WINDOW_BYTES) & ~static_cast<size_t>(3);
            std::vector<uint8_t> bytes(size);
            if (size)
                std::memcpy(bytes.data(), reinterpret_cast<const void *>(info), size);
            return bytes;
        }

        uint32_t U32(const std::vector<uint8_t> &bytes, size_t offset)
        {
            uint32_t value = 0;
            std::memcpy(&value, bytes.data() + offset, 4);
            return value;
        }

        float F32(const std::vector<uint8_t> &bytes, size_t offset)
        {
            float value = 0;
            std::memcpy(&value, bytes.data() + offset, 4);
            return value;
        }

        uint32_t LiveU32(uintptr_t info, size_t offset)
        {
            uint32_t value = 0;
            std::memcpy(&value, reinterpret_cast<const void *>(info + offset), 4);
            return value;
        }

        size_t PickUnique(const std::vector<size_t> &candidates, const char *field)
        {
            if (candidates.size() == 1)
                return candidates[0];
            Log("Model patch: field %s not patched (%zu candidates, needs exactly 1)", field, candidates.size());
            return NOT_FOUND;
        }

        bool Resolve()
        {
            const Memory::PatternResult lookup = Memory::FindPattern(LOOKUP_SIGNATURE);
            if (lookup.matches != 1)
            {
                Log("Model patch: model-info lookup signature matches=%zu (needs 1); unsupported game build",
                    lookup.matches);
                return false;
            }
            g_getModelInfo = reinterpret_cast<GetModelInfoFn>(lookup.firstAddress);
            return true;
        }

        bool Discover()
        {
            const uintptr_t walker = GetModelInfo(WALKER_MODEL);
            const uintptr_t donkey = GetModelInfo(Models::DONKEY);
            const uintptr_t mule = GetModelInfo(Models::MULE);
            if (!walker || !donkey || !mule)
                return false;

            g_targets[0].info = donkey;
            g_targets[1].info = mule;
            g_targets[2].info = GetModelInfo(Models::PAINTED_MULE);

            const std::vector<uint8_t> W = Snapshot(walker);
            const std::vector<uint8_t> D = Snapshot(donkey);
            const std::vector<uint8_t> M = Snapshot(mule);
            const size_t size = std::min({W.size(), D.size(), M.size()});

            std::vector<size_t> animal, hashA, hashB, floats, donkeyTasks;
            for (size_t o = 0; o + 4 <= size; o += 4)
            {
                const uint32_t d = U32(D, o), m = U32(M, o), w = U32(W, o);
                if (d == AT_DONKEY && m == AT_MULE && w == AT_HORSE)
                    animal.push_back(o);
                if (d == HASH_DONKEY && w == HASH_HORSE && m != HASH_HORSE && m != HASH_DONKEY)
                    hashA.push_back(o);
                if (d == HASH_DEFAULT && m == HASH_DEFAULT && w == HASH_HORSE)
                    hashB.push_back(o);
                // DONKEY on the donkey and HORSE on mule and horse:
                // MotionTaskDataSetName and TaskDataName.
                if (d == HASH_DONKEY && w == HASH_HORSE && m == HASH_HORSE)
                    donkeyTasks.push_back(o);
                if (o + 12 <= size)
                {
                    const float w0 = F32(W, o), w1 = F32(W, o + 4), w2 = F32(W, o + 8);
                    const bool horseLevels = (w0 == 25.0f && w1 == 50.0f && w2 == 100.0f) ||
                                             (w0 == 15.0f && w1 == 35.0f && w2 == 100.0f);
                    const bool zeros = U32(D, o) == 0 && U32(D, o + 4) == 0 && U32(D, o + 8) == 0 &&
                                       U32(M, o) == 0 && U32(M, o + 4) == 0 && U32(M, o + 8) == 0;
                    if (horseLevels && zeros)
                        floats.push_back(o);
                }
            }

            g_offsets.animalType = PickUnique(animal, "animal_type");
            g_offsets.horseHashA = PickUnique(hashA, "horse_hash_a");
            g_offsets.horseHashB = PickUnique(hashB, "horse_hash_b");
            g_offsets.floats = PickUnique(floats, "levels");
            if (g_offsets.horseHashA != NOT_FOUND)
                g_muleHashA = U32(M, g_offsets.horseHashA);

            if (donkeyTasks.size() == 2)
            {
                g_donkeyFields[0].offset = donkeyTasks[0];
                g_donkeyFields[1].offset = donkeyTasks[1];
            }
            else
            {
                Log("Model patch: donkey task fields not patched (%zu candidates, needs exactly 2)", donkeyTasks.size());
            }
            for (DonkeyField &field : g_donkeyFields)
            {
                if (field.donkeyValue == HASH_DONKEY)
                    continue; // task fields, located above
                std::vector<size_t> candidates;
                for (size_t o = 0; o + 4 <= size; o += 4)
                {
                    if (U32(D, o) == field.donkeyValue && U32(W, o) == field.horseValue &&
                        U32(M, o) == field.horseValue)
                        candidates.push_back(o);
                }
                field.offset = PickUnique(candidates, field.name);
            }

            // In metapeds the 0xF45D226F flag directly follows the float array.
            if (g_offsets.floats != NOT_FOUND)
            {
                std::vector<size_t> flag;
                const size_t begin = g_offsets.floats + 12;
                const size_t end = std::min(size, g_offsets.floats + 0x20);
                for (size_t o = begin; o < end; ++o)
                {
                    if (W[o] == 1 && D[o] == 0 && M[o] == 0)
                        flag.push_back(o);
                }
                g_offsets.flagByte = PickUnique(flag, "horse_flag");
            }
            return true;
        }

        int g_writes = 0;

        void WriteU32(const Target &target, size_t offset, uint32_t expected, uint32_t value, const char *field)
        {
            if (offset == NOT_FOUND)
                return;
            const uint32_t current = LiveU32(target.info, offset);
            if (current == value)
                return;
            if (current != expected)
            {
                Log("Model patch: %s.%s skipped (current=0x%08X expected=0x%08X)", target.name, field, current,
                    expected);
                return;
            }
            if (Memory::WriteBytes(target.info + offset, &value, sizeof(value)))
                ++g_writes;
            else
                Log("Model patch: %s.%s write FAILED", target.name, field);
        }

        void ApplyTarget(const Target &target)
        {
            if (!target.info)
                return;
            WriteU32(target, g_offsets.animalType, target.originalAnimalType, AT_HORSE, "animal_type");
            const uint32_t originalHashA = target.isDonkey ? HASH_DONKEY : g_muleHashA;
            WriteU32(target, g_offsets.horseHashA, originalHashA, HASH_HORSE, "horse_hash_a");
            WriteU32(target, g_offsets.horseHashB, HASH_DEFAULT, HASH_HORSE, "horse_hash_b");

            if (g_offsets.floats != NOT_FOUND)
            {
                const float levels[3] = {25.0f, 50.0f, 100.0f};
                float current[3]{};
                std::memcpy(current, reinterpret_cast<const void *>(target.info + g_offsets.floats), sizeof(current));
                if (current[0] == 0.0f && current[1] == 0.0f && current[2] == 0.0f &&
                    Memory::WriteBytes(target.info + g_offsets.floats, levels, sizeof(levels)))
                    ++g_writes;
            }
            if (g_offsets.flagByte != NOT_FOUND &&
                *reinterpret_cast<const uint8_t *>(target.info + g_offsets.flagByte) == 0)
            {
                const uint8_t one = 1;
                if (Memory::WriteBytes(target.info + g_offsets.flagByte, &one, 1))
                    ++g_writes;
            }

            if (target.isDonkey)
            {
                for (const DonkeyField &field : g_donkeyFields)
                    WriteU32(target, field.offset, field.donkeyValue, field.horseValue, field.name);
            }
        }
    }

    void Update()
    {
        const ULONGLONG now = GetTickCount64();
        if (g_phase == Phase::Failed || now < g_nextAttemptAt)
            return;
        if (!g_config.patchDonkey && !g_config.patchMules)
        {
            g_phase = Phase::Failed;
            return;
        }

        switch (g_phase)
        {
        case Phase::Resolve:
            if (!Resolve())
            {
                g_phase = Phase::Failed;
                return;
            }
            g_phase = Phase::Discover;
            [[fallthrough]];

        case Phase::Discover:
            if (!Discover())
            {
                if (++g_attempts >= MAX_ATTEMPTS)
                {
                    Log("Model patch: model infos never became available");
                    g_phase = Phase::Failed;
                }
                g_nextAttemptAt = now + RETRY_MS;
                return;
            }
            g_targets[0].enabled = g_config.patchDonkey;
            g_targets[1].enabled = g_config.patchMules;
            g_targets[2].enabled = g_config.patchMules;
            for (const Target &target : g_targets)
            {
                if (target.enabled)
                    ApplyTarget(target);
            }
            Log("Model patch: applied (%d writes) donkey=%d mules=%d", g_writes, g_config.patchDonkey ? 1 : 0,
                g_config.patchMules ? 1 : 0);
            g_phase = Phase::Patched;
            g_nextAttemptAt = now + RECHECK_MS;
            return;

        case Phase::Patched:
            // Model infos are created at boot; make sure nothing restores them.
            g_nextAttemptAt = now + RECHECK_MS;
            for (Target &target : g_targets)
            {
                if (!target.enabled || !target.info || g_offsets.animalType == NOT_FOUND ||
                    !Memory::IsReadable(target.info + g_offsets.animalType, 4) ||
                    LiveU32(target.info, g_offsets.animalType) != target.originalAnimalType)
                    continue;
                if (target.reapplied++ < 5)
                    Log("Model patch: %s was restored by the game; reapplying", target.name);
                ApplyTarget(target);
            }
            return;

        case Phase::Failed:
            return;
        }
    }

    const char *Describe(Hash model, char *buffer, size_t size)
    {
        if (!buffer || size == 0)
            return buffer;
        buffer[0] = '\0';
        if (g_phase != Phase::Patched || g_offsets.animalType == NOT_FOUND)
        {
            std::snprintf(buffer, size, "animal_type=unpatched");
            return buffer;
        }
        const uintptr_t info = GetModelInfo(model);
        if (!info || !Memory::IsReadable(info + g_offsets.animalType, 4))
        {
            std::snprintf(buffer, size, "animal_type=?");
            return buffer;
        }
        std::snprintf(buffer, size, "animal_type=%s", AnimalTypeName(LiveU32(info, g_offsets.animalType)));
        return buffer;
    }
}
