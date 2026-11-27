#include "CarePrompts.h"

#include "Config.h"
#include "Log.h"
#include "mount/MountAdapter.h"

#include "main.h"
#include "natives.h"

#include <Windows.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>

// Care prompts for the donkey and the mules.
//
// The engine shows the vanilla pat / brush / feed prompts greyed out for the
// donkey (and the brush greyed in the TAB wheel) even though the interactions
// themselves work. The decision is made in engine code: no script flag or
// donkey model data unlocks it (see docs/NOTAS-TECNICAS.md). This module adds
// the mod's own prompts instead:
//   - on foot, inside the donkey's lock-on menu, with the vanilla keys
//     (the greyed vanilla prompts are hidden on the donkey), and
//   - a brush prompt while riding a donkey or a mule, when the animal is
//     dirty and standing still (the TAB-wheel brush is greyed on both; the
//     mules keep their working vanilla on-foot prompts).
// Brush and feed do not reimplement anything: they start the vanilla
// player_horse item-use sequence, so cleaning, item use, cores and bonding
// are the game's own.
namespace StabledDonkey::CarePrompts
{
    namespace
    {
        // Natives (alloc8or/rdr3-nativedb-data), called by hash because the
        // ScriptHookRDR2 SDK header leaves them unnamed.
        constexpr uint64_t NATIVE_PROMPT_REGISTER_BEGIN = 0x04F97DE45A519419;
        constexpr uint64_t NATIVE_PROMPT_REGISTER_END = 0xF7AA2696A22AD8B9;
        constexpr uint64_t NATIVE_PROMPT_SET_CONTROL_ACTION = 0xB5352B7494A08258;
        constexpr uint64_t NATIVE_PROMPT_SET_TEXT = 0x5DD02A8318420DD7;
        constexpr uint64_t NATIVE_PROMPT_SET_HOLD_MODE = 0x94073D5CA3F16B7B;
        constexpr uint64_t NATIVE_PROMPT_SET_PRIORITY = 0xCA24F528D0D16289; // ePromptPriority: Low 0 .. MissionCritical 3
        constexpr uint64_t NATIVE_PROMPT_HAS_HOLD_MODE_COMPLETED = 0xE0F65F0640EF0617;
        constexpr uint64_t NATIVE_PROMPT_SET_STANDARD_MODE = 0xCC6656799977741B;
        constexpr uint64_t NATIVE_PROMPT_HAS_STANDARD_MODE_COMPLETED = 0xC92AC953F0A982AE;
        constexpr uint64_t NATIVE_PROMPT_SET_GROUP = 0x2F11D3A254169EA4;
        constexpr uint64_t NATIVE_PROMPT_GET_GROUP_ID_FOR_TARGET_ENTITY = 0xB796970BD125FCE8;
        constexpr uint64_t NATIVE_PROMPT_SET_ENABLED = 0x8A0FB4D03A630D21;
        constexpr uint64_t NATIVE_PROMPT_SET_VISIBLE = 0x71215ACCFDE075EE;
        constexpr uint64_t NATIVE_PROMPT_IS_VALID = 0x347469FBDD1589A9;
        constexpr uint64_t NATIVE_PROMPT_DELETE = 0x00EDE88D4D13CF59;
        constexpr uint64_t NATIVE_PROMPT_RESTART_MODES = 0xDC6C55DFA2C24EE5;
        constexpr uint64_t NATIVE_MODIFY_PLAYER_UI_PROMPT_FOR_PED = 0xA3DB37EDF9A74635;
        constexpr uint64_t NATIVE_VAR_STRING = 0xFA925AC00EB830B9;
        constexpr uint64_t NATIVE_GET_HASH_KEY = 0xFD340785ADF8CFB7;
        constexpr uint64_t NATIVE_GET_MISSION_FLAG = 0xB15CD1CF58771DE1;
        constexpr uint64_t NATIVE_GET_ATTRIBUTE_BASE_RANK = 0x147149F2E909323C;
        constexpr uint64_t NATIVE_INVENTORY_ID_FROM_PED = 0x13D234A2A3F66E63;
        constexpr uint64_t NATIVE_INVENTORY_ITEM_COUNT = 0xE787F05DFC977BDE;
        constexpr uint64_t NATIVE_TASK_ANIMAL_INTERACTION = 0xCD181A959CFDD7F4;
        constexpr uint64_t NATIVE_GET_PLAYER_INTERACTION_TARGET_ENTITY = 0x3EE1F7A8C32F24E1;
        constexpr uint64_t NATIVE_DOES_TEXT_LABEL_EXIST = 0x73C258C68D6F55B6;
        constexpr uint64_t NATIVE_GET_LABEL_TEXT = 0x3429670F9B9EF2D3; // _GET_LABEL_TEXT_2
        constexpr uint64_t NATIVE_GET_CURRENT_LANGUAGE = 0xDB917DA5C6835FCC;

        // Prompt texts come from the game's own text labels (global.gxt2), so
        // they follow the game language. Key names: OpenIV-Team/
        // RAGE-StringsDatabase RDR2/TextKeys/global.txt; INTERACT_HORSE reads
        // "Pat" in the decompiled scripts (beat_spooked_horse.c). The English
        // fallback is used only if a label is missing.
        constexpr const char *LABEL_PAT = "INTERACT_HORSE";
        constexpr const char *LABEL_BRUSH = "BRUSH_HORSE";
        constexpr const char *LABEL_FEED = "FEED_HORSE";

        // player_horse.c (1491.50) item-use state machine, Global_1900372:
        //   [0] state: 0 reset, 1 idle, 2..6 running, 7 cleanup
        //   [1] item hash, [2] target ped, [5] request flag
        // The engine prompt handler (func_788) writes [2], [1] and [5] = 1;
        // the script then validates, starts TASK_ANIMAL_INTERACTION, removes
        // the item and applies its effects (the brush also cleans the horse).
        constexpr int ITEM_USE_GLOBAL = 1900372;
        constexpr int ITEM_USE_STATE = 0;
        constexpr int ITEM_USE_ITEM = 1;
        constexpr int ITEM_USE_TARGET = 2;
        constexpr int ITEM_USE_REQUEST = 5;

        constexpr Hash INTERACTION_POSITIVESHORT = 0x175897C0; // one pat (POSITIVELONG loops while locked on)
        constexpr int ATTRIBUTE_DIRT = 16;                     // player_horse.c func_652(0)
        constexpr int PROMPT_MODE_HIDE = 1;                    // ePromptMode: BLOCK 0, HIDE 1, GREY_OUT 2
        constexpr int HOLD_MS = 500;
        constexpr ULONGLONG ACTION_COOLDOWN_MS = 2500;
        constexpr ULONGLONG INVENTORY_POLL_MS = 500;
        constexpr ULONGLONG HIDE_REFRESH_MS = 250;

        // player_horse.c func_789: horse foods in vanilla preference order,
        // then the herbs it falls back to when none is left (all of them pass
        // its horse-edible check, func_724).
        const char *const FOODS[] = {
            "CONSUMABLE_HAYCUBE",
            "CONSUMABLE_OAT_CAKES",
            "CONSUMABLE_APPLE",
            "CONSUMABLE_BEETS",
            "CONSUMABLE_PEAR",
            "CONSUMABLE_PEACH",
            "CONSUMABLE_CELERY",
            "CONSUMABLE_CARROT",
            "CONSUMABLE_CORN",
            "CONSUMABLE_PEPPERMINT",
            "CONSUMABLE_SUGARCUBE",
            "CONSUMABLE_HERB_ALASKAN_GINSENG",
            "CONSUMABLE_HERB_AMERICAN_GINSENG",
            "CONSUMABLE_HERB_BAY_BOLETE",
            "CONSUMABLE_HERB_BLACK_BERRY",
            "CONSUMABLE_HERB_BLACK_CURRANT",
            "CONSUMABLE_HERB_BURDOCK_ROOT",
            "CONSUMABLE_HERB_CHANTERELLES",
            "CONSUMABLE_HERB_CREEPING_THYME",
            "CONSUMABLE_HERB_DESERT_SAGE",
            "CONSUMABLE_HERB_HUMMINGBIRD_SAGE",
            "CONSUMABLE_HERB_GOLDEN_CURRANT",
            "CONSUMABLE_HERB_OLEANDER_SAGE",
            "CONSUMABLE_HERB_PARASOL_MUSHROOM",
            "CONSUMABLE_HERB_RAMS_HEAD",
            "CONSUMABLE_HERB_RED_RASPBERRY",
            "CONSUMABLE_HERB_RED_SAGE",
            "CONSUMABLE_HERB_WILD_FEVERFEW",
            "CONSUMABLE_HERB_WILD_MINT",
            "CONSUMABLE_HERB_GINSENG",
            "CONSUMABLE_HERB_SAGE",
            "CONSUMABLE_HERB_WINTERGREEN_BERRY",
            "CONSUMABLE_HERB_CURRANT",
            "CONSUMABLE_HERB_OREGANO",
            "CONSUMABLE_HERB_WILD_CARROTS",
            "CONSUMABLE_HERB_COMMON_BULRUSH",
            "CONSUMABLE_HERB_ENGLISH_MACE",
        };

        struct Prompt
        {
            int handle = 0;
            bool hold = false; // hold mode (HOLD_MS) instead of a single press
        };

        Prompt g_pat, g_brush, g_feed, g_mountedBrush;
        Ped g_groupPed = 0;
        int g_groupId = 0;
        Ped g_hiddenPed = 0;
        ULONGLONG g_nextHide = 0;
        ULONGLONG g_cooldownUntil = 0;
        bool g_refreshAfterAction = false;
        bool g_mountedBrushForMule = false;
        Hash g_brushItem = 0;
        int g_brushCount = 0;
        Hash g_food = 0;
        ULONGLONG g_nextInventoryPoll = 0;

        Hash Joaat(const char *text)
        {
            return invoke<Hash>(NATIVE_GET_HASH_KEY, text);
        }

        bool LabelExists(const char *label)
        {
            return invoke<BOOL>(NATIVE_DOES_TEXT_LABEL_EXIST, label) != 0;
        }

        std::string GameText(const char *label, const char *fallback)
        {
            if (LabelExists(label))
            {
                const char *text = invoke<const char *>(NATIVE_GET_LABEL_TEXT, label);
                if (text && *text)
                    return text;
            }
            return fallback;
        }

        // Once per launch: the game language and the texts of the labels in
        // use or under study (vanilla horse prompts), for the log.
        void LogGameTexts()
        {
            static bool logged = false;
            if (logged)
                return;
            logged = true;
            Log("Care: game language=%d", invoke<int>(NATIVE_GET_CURRENT_LANGUAGE));
            for (const char *label : {LABEL_PAT, LABEL_BRUSH, LABEL_FEED, "CALM_HORSE", "REMOVE_SADDLE",
                                      "MOUNT_HORSE", "BREED_DONKEY", "BREED_MULE", "BREED_MULE_PAINTED"})
            {
                const bool exists = LabelExists(label);
                Log("Care: label %s exists=%d text=\"%s\"", label, exists ? 1 : 0,
                    exists ? GameText(label, "").c_str() : "");
            }
        }

        bool IsValid(const Prompt &prompt)
        {
            return prompt.handle != 0 && invoke<BOOL>(NATIVE_PROMPT_IS_VALID, prompt.handle) != 0;
        }

        void Delete(Prompt &prompt)
        {
            if (IsValid(prompt))
                invoke<Void>(NATIVE_PROMPT_DELETE, prompt.handle);
            prompt.handle = 0;
        }

        // groupId 0 = standalone prompt (shown in the corner), otherwise the
        // prompt joins the lock-on menu of that target. On-foot prompts fire
        // on a single press like the vanilla ones; the mounted brush is a
        // hold so it is not triggered by accident while riding.
        void Create(Prompt &prompt, const char *label, const std::string &input, int groupId, bool hold)
        {
            Delete(prompt);
            const int handle = invoke<int>(NATIVE_PROMPT_REGISTER_BEGIN);
            invoke<Any>(NATIVE_PROMPT_SET_CONTROL_ACTION, handle, Joaat(input.c_str()));
            const char *text = invoke<const char *>(NATIVE_VAR_STRING, 10, "LITERAL_STRING", label);
            invoke<Void>(NATIVE_PROMPT_SET_TEXT, handle, text);
            invoke<Void>(NATIVE_PROMPT_SET_ENABLED, handle, TRUE);
            invoke<Void>(NATIVE_PROMPT_SET_VISIBLE, handle, groupId != 0 ? TRUE : FALSE);
            if (hold)
                invoke<Void>(NATIVE_PROMPT_SET_HOLD_MODE, handle, HOLD_MS);
            else
                invoke<Void>(NATIVE_PROMPT_SET_STANDARD_MODE, handle, FALSE); // FALSE: completes on press
            prompt.hold = hold;
            // On a mule the active vanilla pat covered our prompts; a higher
            // priority keeps them in the lock-on menu.
            invoke<Void>(NATIVE_PROMPT_SET_PRIORITY, handle, g_config.carePromptPriority);
            if (groupId != 0)
                invoke<Void>(NATIVE_PROMPT_SET_GROUP, handle, groupId, 0);
            invoke<Void>(NATIVE_PROMPT_REGISTER_END, handle);
            prompt.handle = handle;
        }

        void SetVisible(const Prompt &prompt, bool visible)
        {
            if (IsValid(prompt))
                invoke<Void>(NATIVE_PROMPT_SET_VISIBLE, prompt.handle, visible ? TRUE : FALSE);
        }

        void SetEnabled(const Prompt &prompt, bool enabled)
        {
            if (IsValid(prompt))
                invoke<Void>(NATIVE_PROMPT_SET_ENABLED, prompt.handle, enabled ? TRUE : FALSE);
        }

        bool Completed(const Prompt &prompt)
        {
            if (!IsValid(prompt))
                return false;
            if (prompt.hold)
                return invoke<BOOL>(NATIVE_PROMPT_HAS_HOLD_MODE_COMPLETED, prompt.handle) != 0;
            return invoke<BOOL>(NATIVE_PROMPT_HAS_STANDARD_MODE_COMPLETED, prompt.handle, 0) != 0;
        }

        void DeleteGroupPrompts()
        {
            Delete(g_pat);
            Delete(g_brush);
            Delete(g_feed);
            g_groupPed = 0;
            g_groupId = 0;
        }

        void SetupGroupPrompts(Ped donkey)
        {
            const int groupId = invoke<int>(NATIVE_PROMPT_GET_GROUP_ID_FOR_TARGET_ENTITY, donkey);
            if (donkey == g_groupPed && groupId == g_groupId && IsValid(g_pat) && IsValid(g_brush) &&
                IsValid(g_feed))
                return;
            g_groupPed = donkey;
            g_groupId = groupId;
            LogGameTexts();
            Create(g_pat, GameText(LABEL_PAT, "Pat").c_str(), g_config.careInputPat, groupId, false);
            Create(g_brush, GameText(LABEL_BRUSH, "Brush").c_str(), g_config.careInputBrush, groupId, false);
            Create(g_feed, GameText(LABEL_FEED, "Feed").c_str(), g_config.careInputFeed, groupId, false);
        }

        // Hides the greyed vanilla prompts on the donkey so ours can use the
        // vanilla keys. The hide mode is per ped and does not touch horses.
        void HideVanillaPrompts(Ped donkey, ULONGLONG now)
        {
            if (donkey == g_hiddenPed && now < g_nextHide)
                return;
            g_hiddenPed = donkey;
            g_nextHide = now + HIDE_REFRESH_MS;
            const Player player = PLAYER::PLAYER_ID();
            // Mules keep working vanilla prompts; hiding all of them (as on the
            // donkey) made the engine drop the lock-on a second later (test
            // 2026-11-26), so they have their own, shorter list.
            const std::string &list = MountAdapter(donkey).IsMule() ? g_config.hideVanillaPromptTypesMule
                                                                    : g_config.hideVanillaPromptTypes;
            size_t start = 0;
            while (start < list.size())
            {
                size_t end = list.find_first_of(",; ", start);
                if (end == std::string::npos)
                    end = list.size();
                const int type = std::atoi(list.substr(start, end - start).c_str());
                start = end + 1;
                if (type > 0)
                    invoke<Void>(NATIVE_MODIFY_PLAYER_UI_PROMPT_FOR_PED, player, donkey, type, PROMPT_MODE_HIDE, TRUE);
            }
        }

        int ItemCount(Hash item)
        {
            const int inventory = invoke<int>(NATIVE_INVENTORY_ID_FROM_PED, PLAYER::PLAYER_PED_ID());
            int count = invoke<int>(NATIVE_INVENTORY_ITEM_COUNT, inventory, item, FALSE);
            if (count <= 0 && inventory != 1)
                count = invoke<int>(NATIVE_INVENTORY_ITEM_COUNT, 1, item, FALSE);
            return count;
        }

        void PollInventory(ULONGLONG now)
        {
            if (now < g_nextInventoryPoll)
                return;
            g_nextInventoryPoll = now + INVENTORY_POLL_MS;
            if (!g_brushItem)
                g_brushItem = Joaat("KIT_HORSE_BRUSH");
            g_brushCount = ItemCount(g_brushItem);
            g_food = 0;
            for (const char *name : FOODS)
            {
                const Hash item = Joaat(name);
                if (ItemCount(item) > 0)
                {
                    g_food = item;
                    break;
                }
            }
        }

        UINT64 *ItemUseSlot(int field)
        {
            return getGlobalPtr(ITEM_USE_GLOBAL + field);
        }

        // Script ints live in the low 32 bits of a global slot; the high bits
        // can hold leftovers (seen 0x271'00000005 while brushing).
        uint32_t SlotInt(const UINT64 *slot)
        {
            return static_cast<uint32_t>(*slot);
        }

        bool ItemUseIdle()
        {
            const UINT64 *state = ItemUseSlot(ITEM_USE_STATE);
            const UINT64 *request = ItemUseSlot(ITEM_USE_REQUEST);
            return state && request && SlotInt(state) == 1 && SlotInt(request) == 0;
        }

        // Same writes as the vanilla engine-prompt handler (player_horse func_788).
        bool RequestItemUse(Ped donkey, Hash item, const char *what)
        {
            UINT64 *state = ItemUseSlot(ITEM_USE_STATE);
            UINT64 *itemSlot = ItemUseSlot(ITEM_USE_ITEM);
            UINT64 *target = ItemUseSlot(ITEM_USE_TARGET);
            UINT64 *request = ItemUseSlot(ITEM_USE_REQUEST);
            if (!state || !itemSlot || !target || !request)
                return false;
            if (SlotInt(state) > 7)
            {
                Log("Care: %s refused, unexpected item-use state %u (unsupported game build?)", what, SlotInt(state));
                return false;
            }
            if (SlotInt(state) != 1 || SlotInt(request) != 0)
                return false;
            *target = static_cast<UINT64>(static_cast<uint32_t>(donkey));
            *itemSlot = static_cast<UINT64>(item);
            *request = 1;
            Log("Care: %s (item 0x%08X)", what, item);
            return true;
        }

        float Distance(Entity a, Entity b)
        {
            const Vector3 pa = ENTITY::GET_ENTITY_COORDS(a, TRUE, FALSE);
            const Vector3 pb = ENTITY::GET_ENTITY_COORDS(b, TRUE, FALSE);
            const float dx = pa.x - pb.x, dy = pa.y - pb.y, dz = pa.z - pb.z;
            return sqrtf(dx * dx + dy * dy + dz * dz);
        }

        bool IsOnFootCareTarget(Entity entity)
        {
            if (!entity || !ENTITY::DOES_ENTITY_EXIST(entity) || !ENTITY::IS_ENTITY_A_PED(entity))
                return false;
            const MountAdapter adapter(entity);
            // Any bond level: pat is offered before the bond, like vanilla
            // (it is how the bond starts); brush / feed need level 1.
            return (adapter.IsDonkey() && g_config.patchDonkey) ||
                   (adapter.IsMule() && g_config.patchMules && g_config.careMuleOnFoot);
        }

        // Short state-change log for the on-foot prompts (to find out why
        // they sometimes do not show on a mule).
        Ped g_loggedTarget = 0;
        int g_loggedState = -1;

        void LogOnFootState(Ped target, float distance, bool isNear, bool bonded, int groupId)
        {
            Entity lockOn = 0;
            if (!invoke<BOOL>(NATIVE_GET_PLAYER_INTERACTION_TARGET_ENTITY, PLAYER::PLAYER_ID(), &lockOn, FALSE, FALSE))
                lockOn = 0;
            const bool locked = lockOn == target;
            const int state = (isNear ? 1 : 0) | (bonded ? 2 : 0) | (IsValid(g_pat) ? 4 : 0) | (locked ? 8 : 0);
            if (target == g_loggedTarget && state == g_loggedState)
                return;
            g_loggedTarget = target;
            g_loggedState = state;
            Log("Care: on foot model=0x%08X locked=%d bond_level=%d dist=%.2f near=%d group=%d prompts=%d",
                ENTITY::GET_ENTITY_MODEL(target), locked ? 1 : 0, MountAdapter(target).GetBondingLevel(), distance,
                isNear ? 1 : 0, groupId, IsValid(g_pat) ? 1 : 0);
        }

        Entity LockOnTarget()
        {
            Entity target = 0;
            if (!invoke<BOOL>(NATIVE_GET_PLAYER_INTERACTION_TARGET_ENTITY, PLAYER::PLAYER_ID(), &target, FALSE, FALSE))
                return 0;
            return target;
        }

        void ActionStarted(ULONGLONG now)
        {
            g_cooldownUntil = now + ACTION_COOLDOWN_MS;
            g_refreshAfterAction = true;
        }

        void UpdateOnFoot(Ped player, Ped donkey, bool idle, ULONGLONG now)
        {
            SetupGroupPrompts(donkey);
            const float distance = Distance(player, donkey);
            const bool isNear = distance <= g_config.careMaxDistance;
            const bool bonded = MountAdapter(donkey).GetBondingLevel() >= 1;
            LogOnFootState(donkey, distance, isNear, bonded, g_groupId);
            SetVisible(g_pat, isNear);
            SetVisible(g_brush, isNear && bonded && g_brushCount > 0);
            SetVisible(g_feed, isNear && bonded && g_food != 0);
            SetEnabled(g_pat, isNear && idle);
            SetEnabled(g_brush, isNear && idle && bonded && g_brushCount > 0);
            SetEnabled(g_feed, isNear && idle && bonded && g_food != 0);
            if (!isNear)
                return;
            HideVanillaPrompts(donkey, now);
            if (!idle)
                return;

            if (Completed(g_pat))
            {
                invoke<Void>(NATIVE_TASK_ANIMAL_INTERACTION, player, donkey, INTERACTION_POSITIVESHORT,
                             static_cast<Hash>(0), FALSE);
                Log("Care: pat");
                ActionStarted(now);
            }
            else if (bonded && g_brushCount > 0 && Completed(g_brush))
            {
                if (RequestItemUse(donkey, g_brushItem, "brush"))
                    ActionStarted(now);
            }
            else if (bonded && g_food != 0 && Completed(g_feed))
            {
                if (RequestItemUse(donkey, g_food, "feed"))
                    ActionStarted(now);
            }
        }

        // Brush while riding. The TAB-wheel brush stays greyed on the donkey
        // and on the mules (it works on horses), so both get this prompt.
        void UpdateMounted(Ped mount, bool idle, ULONGLONG now)
        {
            const bool isMule = mount && MountAdapter(mount).IsMule();
            if (!IsValid(g_mountedBrush) || isMule != g_mountedBrushForMule)
            {
                g_mountedBrushForMule = isMule;
                LogGameTexts();
                Create(g_mountedBrush, GameText(LABEL_BRUSH, isMule ? "Brush mule" : "Brush donkey").c_str(),
                       g_config.careInputMountedBrush, 0, true);
            }

            const int dirt = mount ? invoke<int>(NATIVE_GET_ATTRIBUTE_BASE_RANK, mount, ATTRIBUTE_DIRT) : 0;
            // Only when it matters, so the key does not get in the way.
            const bool show = mount && g_brushCount > 0 && dirt >= g_config.careMountedBrushMinDirt &&
                              !invoke<BOOL>(NATIVE_GET_MISSION_FLAG) && ENTITY::GET_ENTITY_SPEED(mount) < 0.5f;
            SetVisible(g_mountedBrush, show);
            SetEnabled(g_mountedBrush, show && idle);
            if (show && idle && Completed(g_mountedBrush) && RequestItemUse(mount, g_brushItem, "mounted brush"))
                ActionStarted(now);
        }

        // Current mount if it is a supported animal patched by the config.
        Ped CareMount(Ped player)
        {
            if (!PED::IS_PED_ON_MOUNT(player))
                return 0;
            const Ped mount = PED::GET_MOUNT(player);
            const MountAdapter adapter(mount);
            if (adapter.IsDonkey() && g_config.patchDonkey)
                return mount;
            if (adapter.IsMule() && g_config.patchMules)
                return mount;
            return 0;
        }
    }

    void Update(Ped trackedDonkey)
    {
        if (!g_config.carePrompts)
            return;
        const Ped player = PLAYER::PLAYER_PED_ID();
        if (!player || !ENTITY::DOES_ENTITY_EXIST(player))
            return;

        // Prompts registered before a save load are gone.
        if (g_pat.handle && !IsValid(g_pat))
            DeleteGroupPrompts();
        if (g_mountedBrush.handle && !IsValid(g_mountedBrush))
            g_mountedBrush.handle = 0;

        // On-foot prompts for the donkey and (CareMuleOnFoot) the mules, whose
        // vanilla prompts grey out unpredictably. Like vanilla horses, only
        // once the animal has a bond.
        const Entity lockOn = LockOnTarget();
        Ped donkey = IsOnFootCareTarget(lockOn) ? lockOn : (IsOnFootCareTarget(trackedDonkey) ? trackedDonkey : 0);
        if (donkey && ENTITY::IS_ENTITY_DEAD(donkey))
            donkey = 0;
        const Ped mount = CareMount(player);
        if (!donkey && g_groupPed)
            DeleteGroupPrompts();
        if (!donkey && !mount)
        {
            SetVisible(g_mountedBrush, false);
            return;
        }

        const ULONGLONG now = GetTickCount64();
        // Not IS_ANIMAL_INTERACTION_RUNNING: after the first action it stays
        // true for as long as the lock-on is held (test 2026-11-27), which
        // kept the prompts greyed until the player locked on again. The
        // cooldown covers the pat animation; brush and feed also wait for the
        // vanilla item-use sequence.
        const bool idle = ItemUseIdle() && now >= g_cooldownUntil;

        // A lock-on menu prompt stays greyed after it fired; recreating ours
        // once the action is over brings them back.
        if (idle && g_refreshAfterAction)
        {
            g_refreshAfterAction = false;
            DeleteGroupPrompts();
            if (IsValid(g_mountedBrush))
                invoke<Void>(NATIVE_PROMPT_RESTART_MODES, g_mountedBrush.handle);
        }

        PollInventory(now);
        if (donkey && !PED::IS_PED_ON_MOUNT(player))
            UpdateOnFoot(player, donkey, idle, now);
        UpdateMounted(mount, idle, now);
    }
}
