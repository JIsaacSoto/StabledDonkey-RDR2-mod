#include "SaddlePrompts.h"

#include "Config.h"
#include "Log.h"
#include "mount/MountAdapter.h"

#include "main.h"
#include "natives.h"

#include <Windows.h>

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

// "Remove saddle" and "place saddle" for the donkey and the mules.
//
// Both are engine prompts on horses (INPUT_SADDLE_TRANSFER): the engine plays
// the animation, creates the saddle bundle (P_CS_SADDLEBUNDLE01X, carriable
// CARRIABLE_SADDLE_BUNDLE) and sends EVENT_SADDLE_TRANSFER, which player_horse
// (func_153) uses to move the saddle. The engine never offers them on the
// donkey or the mules, and no script-visible data, carrying flag or
// model-info field changes that (tests 2026-11-27). So the mod adds its own
// prompts (texts: the game's REMOVE_SADDLE / PLACE_SADDLE labels) and does
// what the vanilla scripts do:
//   - remove (main horse, horse slot 0): like the stable's "take saddle"
//     (shop_horse_shop_sp func_799) plus the vanilla tack strip
//     (player_horse func_468): create the bundle, strip the tack, no saddle
//     horse, saddle slot -1, the player carries it. The vanilla "place
//     saddle" then works on any horse.
//   - place (bundle carried): the vanilla equip branch of func_153: delete the
//     bundle, saddle slot 0, saddle horse, carrying flag 9, and the player's
//     tack (inventory SLOTID_HORSE_*, func_355) applied to the animal. On the
//     temp horse (slot 1) the two horse slots are swapped first, as func_116
//     does: slot data, slot peds, and the horse items in the player's
//     inventory (func_408), then active / temp horse.
namespace StabledDonkey::SaddlePrompts
{
    namespace
    {
        constexpr uint64_t NATIVE_PROMPT_REGISTER_BEGIN = 0x04F97DE45A519419;
        constexpr uint64_t NATIVE_PROMPT_REGISTER_END = 0xF7AA2696A22AD8B9;
        constexpr uint64_t NATIVE_PROMPT_SET_CONTROL_ACTION = 0xB5352B7494A08258;
        constexpr uint64_t NATIVE_PROMPT_SET_TEXT = 0x5DD02A8318420DD7;
        constexpr uint64_t NATIVE_PROMPT_SET_HOLD_MODE = 0x94073D5CA3F16B7B;
        constexpr uint64_t NATIVE_PROMPT_SET_PRIORITY = 0xCA24F528D0D16289;
        constexpr uint64_t NATIVE_PROMPT_HAS_HOLD_MODE_COMPLETED = 0xE0F65F0640EF0617;
        constexpr uint64_t NATIVE_PROMPT_SET_GROUP = 0x2F11D3A254169EA4;
        constexpr uint64_t NATIVE_PROMPT_GET_GROUP_ID_FOR_TARGET_ENTITY = 0xB796970BD125FCE8;
        constexpr uint64_t NATIVE_PROMPT_SET_ENABLED = 0x8A0FB4D03A630D21;
        constexpr uint64_t NATIVE_PROMPT_SET_VISIBLE = 0x71215ACCFDE075EE;
        constexpr uint64_t NATIVE_PROMPT_IS_VALID = 0x347469FBDD1589A9;
        constexpr uint64_t NATIVE_PROMPT_DELETE = 0x00EDE88D4D13CF59;
        constexpr uint64_t NATIVE_VAR_STRING = 0xFA925AC00EB830B9;
        constexpr uint64_t NATIVE_GET_HASH_KEY = 0xFD340785ADF8CFB7;
        constexpr uint64_t NATIVE_DOES_TEXT_LABEL_EXIST = 0x73C258C68D6F55B6;
        constexpr uint64_t NATIVE_GET_LABEL_TEXT = 0x3429670F9B9EF2D3;
        constexpr uint64_t NATIVE_GET_PLAYER_INTERACTION_TARGET_ENTITY = 0x3EE1F7A8C32F24E1;
        constexpr uint64_t NATIVE_GET_SADDLE_HORSE_FOR_PLAYER = 0xB48050D326E9A2F3;
        constexpr uint64_t NATIVE_SET_PED_AS_SADDLE_HORSE_FOR_PLAYER = 0xD2CB0FB0FDCB473D;
        constexpr uint64_t NATIVE_GET_ACTIVE_HORSE_FOR_PLAYER = 0x46FA0AE18F4C7FA9;
        constexpr uint64_t NATIVE_SET_PED_ACTIVE_PLAYER_HORSE = 0x8FBF9EDB378CCB8C;
        constexpr uint64_t NATIVE_GET_TEMP_PLAYER_HORSE = 0xD3F7445CEA2E5035;
        constexpr uint64_t NATIVE_SET_PED_AS_TEMP_PLAYER_HORSE = 0x227B06324234FB09;
        constexpr uint64_t NATIVE_IS_PED_CARRYING_SOMETHING = 0xA911EE21EDF69DAF;
        constexpr uint64_t NATIVE_GET_FIRST_ENTITY_PED_IS_CARRYING = 0xD806CD2A4F2C2996;
        constexpr uint64_t NATIVE_GET_ENTITY_CARRYING_FLAG = 0x808077647856DE62;
        constexpr uint64_t NATIVE_SET_ENTITY_CARRYING_FLAG = 0x18FF3110CF47115D;
        constexpr uint64_t NATIVE_GET_NUM_COMPONENTS_IN_PED = 0x90403E8107B60E81;
        constexpr uint64_t NATIVE_GET_SHOP_ITEM_COMPONENT_AT_INDEX = 0x77BA37622E22023B;
        constexpr uint64_t NATIVE_REMOVE_SHOP_ITEM_FROM_PED_BY_CATEGORY = 0xDF631E4BCE1B1FC4;
        constexpr uint64_t NATIVE_APPLY_SHOP_ITEM_TO_PED = 0xD3A7B003ED343FD9;
        constexpr uint64_t NATIVE_EQUIP_META_PED_OUTFIT = 0x1902C4CFCC5BE57C;
        constexpr uint64_t NATIVE_UPDATE_PED_VARIATION = 0xCC8CA3E88256E58F;
        constexpr uint64_t NATIVE_SET_PED_CONFIG_FLAG = 0x1913FE4CBF41C463;
        constexpr uint64_t NATIVE_REQUEST_MODEL = 0xFA28FE3A6246FC30;
        constexpr uint64_t NATIVE_HAS_MODEL_LOADED = 0x1283B8B89DD5D1B6;
        constexpr uint64_t NATIVE_SET_MODEL_AS_NO_LONGER_NEEDED = 0x4AD96EF928BD4F9A;
        constexpr uint64_t NATIVE_CREATE_OBJECT = 0x509D5878EB39E842;
        constexpr uint64_t NATIVE_DELETE_OBJECT = 0x931914268722C263;
        constexpr uint64_t NATIVE_SET_ENTITY_AS_MISSION_ENTITY = 0xDC19C288082E586E;
        constexpr uint64_t NATIVE_DETACH_CARRIABLE_ENTITY = 0xED00D72F81CF7278;
        constexpr uint64_t NATIVE_TASK_CARRIABLE = 0xF0B4F759F35CC7F5;
        constexpr uint64_t NATIVE_DOES_BLIP_EXIST = 0xCD82FA174080B3B1;
        constexpr uint64_t NATIVE_REMOVE_BLIP = 0xF2C3C9DA47AAA54A;
        constexpr uint64_t NATIVE_SET_SCRIPT_CAN_REMOVE_BLIPS = 0x8ABD939C2E5D00ED;
        // Inventory (player_horse func_330 / func_81 / func_719 / func_722 / func_918).
        constexpr uint64_t NATIVE_INVENTORY_IS_USING_BACKUP = 0x7C7E4AB748EA3B07;
        constexpr uint64_t NATIVE_INVENTORY_GET_GUID_FROM_ITEMID = 0x886DFD3E185C8A89;
        constexpr uint64_t NATIVE_INVENTORY_GET_CHILDREN_IN_SLOT_COUNT = 0x033EE4B89F3AC545;
        constexpr uint64_t NATIVE_INVENTORY_GET_ITEM_EQUIPPED_IN_SLOT = 0xBE012571B25F5ACA;
        constexpr uint64_t NATIVE_INVENTORY_GET_ITEM_EQUIPPED_IN_SLOT_BY_REF = 0x22E590F108289A9D;
        constexpr uint64_t NATIVE_INVENTORY_GET_FULL_ITEM_DATA = 0x025A1B1FB03FBF61;
        constexpr uint64_t NATIVE_INVENTORY_GET_INVENTORY_ITEM = 0x9700E8EFC4AB9089;
        constexpr uint64_t NATIVE_INVENTORY_IS_GUID_VALID = 0xB881CA836CC4B6D4;
        constexpr uint64_t NATIVE_INVENTORY_FITS_SLOT_ID = 0x780C5B9AE2819807;
        constexpr uint64_t NATIVE_INVENTORY_SWAP_ITEM = 0xF2753D691BCDA314;
        constexpr uint64_t NATIVE_INVENTORY_EQUIP_ITEM_WITH_GUID = 0x734311E2852760D0;

        // Script globals (1491.50 decompiled scripts). Ints are the low 32
        // bits of a slot.
        //   Global_40.f_1095.f_1[7 /*436*/]  horse slot data (array size at 1136)
        //   Global_40.f_1095.f_3054          saddle slot: 0 on the main horse, -1 off
        //   Global_40.f_1095.f_3054.f_1      saddle-off flag (1 while carried)
        //   Global_1900383[7 /*45*/]         horse slot peds (array size at 1900383),
        //                                    .f_25 / .f_26 blip state / blip
        //   Global_1900383.f_393 / .f_394    carried saddle bundle and its blip
        //   Global_1900383.f_442             saddle went onto another horse
        constexpr int G_HORSE_DATA_ARRAY = 1136;
        constexpr int G_HORSE_DATA_FIRST = G_HORSE_DATA_ARRAY + 1;
        constexpr int HORSE_DATA_SIZE = 436;
        constexpr int G_SADDLE_SLOT = 4189;
        constexpr int G_SADDLE_OFF = 4190;
        constexpr int G_HORSE_PEDS = 1900383;
        constexpr int G_HORSE_PED_FIRST = G_HORSE_PEDS + 1;
        constexpr int HORSE_PED_SIZE = 45;
        constexpr int HORSE_PED_BLIP_STATE = 25;
        constexpr int HORSE_PED_BLIP = 26;
        constexpr int G_SADDLE_BUNDLE = 1900776;
        constexpr int G_SADDLE_BUNDLE_BLIP = 1900777;
        constexpr int G_SADDLE_MOVED = 1900825;
        constexpr int HORSE_SLOT_COUNT = 7;

        constexpr Hash ITEM_CHARACTER = 0x4F31C2D3;           // CHARACTER
        constexpr Hash ITEM_HORSE_EQUIPMENT = 0x3309E71D;     // 856287005, func_81
        constexpr Hash SLOTID_NONE = 0xA1212100;
        constexpr Hash SLOTID_ACTIVE_HORSE = 0x6A386D80;      // horse slot 0
        constexpr Hash SLOTID_TEMP_HORSE = 0xCE638816;        // horse slot 1 (-832337898, func_720)
        constexpr Hash SLOTID_CARRIED_HORSE_EQUIPMENT = 0xF2F4AB81;
        // player_horse func_355: the player's tack slots.
        const char *const TACK_SLOTS[] = {"SLOTID_HORSE_BLANKET", "SLOTID_HORSE_SADDLE",  "SLOTID_HORSE_HORN",
                                          "SLOTID_HORSE_STIRRUP", "SLOTID_HORSE_BEDROLL", "SLOTID_HORSE_SADDLEBAG"};
        // player_horse func_468: the tack categories removed with the saddle.
        const char *const TACK_CATEGORIES[] = {"HORSE_BLANKETS",  "HORSE_SADDLES",  "SADDLE_HORNS",
                                               "SADDLE_STIRRUPS", "HORSE_BEDROLLS", "HORSE_SADDLEBAGS"};

        constexpr int PCF_CORPSE_IS_PERSISTENT = 186;
        constexpr int CARRYING_FLAG_CAN_CARRY_ANYTHING = 9;
        constexpr int HOLD_MS = 500;
        constexpr ULONGLONG COOLDOWN_MS = 1500;
        constexpr ULONGLONG MODEL_TIMEOUT_MS = 5000;

        struct Prompt
        {
            int handle = 0;
            int group = -1;
        };

        Prompt g_remove, g_place;
        bool g_loadingBundle = false;
        Ped g_removeTarget = 0;
        ULONGLONG g_deadline = 0;
        ULONGLONG g_cooldownUntil = 0;
        // Tack worn by the animal the mod took the saddle off (fallback).
        std::vector<Hash> g_tackWorn;
        Ped g_tackPed = 0;
        int g_globalsState = 0; // 0 unchecked, 1 ok, -1 unexpected layout
        std::string g_lastState;

        Hash Joaat(const char *text)
        {
            return invoke<Hash>(NATIVE_GET_HASH_KEY, text);
        }

        std::string Format(const char *format, ...)
        {
            char buffer[512];
            va_list args;
            va_start(args, format);
            std::vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);
            return buffer;
        }

        // Prompt texts are the game's own labels (global.gxt2), as on horses:
        // REMOVE_SADDLE "Remove Saddle" and PLACE_SADDLE "Saddle" (key found
        // by scanning every text key, test 2026-11-27).
        std::string GameText(const char *label)
        {
            if (invoke<BOOL>(NATIVE_DOES_TEXT_LABEL_EXIST, label))
            {
                const char *text = invoke<const char *>(NATIVE_GET_LABEL_TEXT, label);
                if (text && *text)
                    return text;
            }
            Log("Saddle: text label %s missing", label);
            return label;
        }

        int ReadGlobal(int index)
        {
            const UINT64 *slot = getGlobalPtr(index);
            return slot ? static_cast<int>(static_cast<uint32_t>(*slot)) : 0;
        }

        // Writes the low 32 bits only, like the script VM.
        void WriteGlobal(int index, int value)
        {
            UINT64 *slot = getGlobalPtr(index);
            if (slot)
                *reinterpret_cast<int32_t *>(slot) = value;
        }

        // Swaps two runs of whole global slots (COPY_SCRIPT_STRUCT through a
        // temp, as func_406 / func_407 do).
        bool SwapGlobals(int first, int second, int count)
        {
            for (int i = 0; i < count; ++i)
                if (!getGlobalPtr(first + i) || !getGlobalPtr(second + i))
                    return false;
            for (int i = 0; i < count; ++i)
                std::swap(*getGlobalPtr(first + i), *getGlobalPtr(second + i));
            return true;
        }

        Ped SlotPed(int slot)
        {
            return static_cast<Ped>(ReadGlobal(G_HORSE_PED_FIRST + slot * HORSE_PED_SIZE));
        }

        // Both horse-slot arrays must hold 7 entries, or this is not the
        // global layout of 1491.50 and nothing is written.
        bool GlobalsOk()
        {
            if (g_globalsState == 0)
            {
                const int dataSize = ReadGlobal(G_HORSE_DATA_ARRAY);
                const int pedSize = ReadGlobal(G_HORSE_PEDS);
                g_globalsState = (dataSize == HORSE_SLOT_COUNT && pedSize == HORSE_SLOT_COUNT) ? 1 : -1;
                Log("Saddle: globals %s (horse data array=%d, horse ped array=%d, main horse=%d, temp horse=%d, "
                    "saddle slot=%d, saddle off=%d, bundle=%d)",
                    g_globalsState > 0 ? "ok" : "unexpected, saddle prompts disabled", dataSize, pedSize, SlotPed(0),
                    SlotPed(1), ReadGlobal(G_SADDLE_SLOT), ReadGlobal(G_SADDLE_OFF), ReadGlobal(G_SADDLE_BUNDLE));
            }
            return g_globalsState > 0;
        }

        // ---- Inventory ------------------------------------------------------
        // Script structs are arrays of 8-byte slots; buffers are larger than
        // the scripts' structs. Item data: [0..3] GUID, [4] item, [5..8]
        // parent GUID, [9] slot id, [10] equipped flag.

        struct Guid
        {
            uint64_t v[4] = {};
            bool operator==(const Guid &o) const
            {
                return v[0] == o.v[0] && v[1] == o.v[1] && v[2] == o.v[2] && v[3] == o.v[3];
            }
        };

        struct ItemData
        {
            uint64_t v[64] = {};
            ItemData()
            {
                v[9] = SLOTID_NONE;
            }
        };

        int InventoryId()
        {
            return invoke<BOOL>(NATIVE_INVENTORY_IS_USING_BACKUP) ? 5 : 1; // func_666(0) in singleplayer
        }

        std::string GuidText(const Guid &guid)
        {
            return Format("%llX:%llX:%llX:%llX", guid.v[0], guid.v[1], guid.v[2], guid.v[3]);
        }

        // func_330: the player character's GUID.
        bool CharacterGuid(int inventory, Guid &out)
        {
            Guid none;
            return invoke<BOOL>(NATIVE_INVENTORY_GET_GUID_FROM_ITEMID, inventory, none.v, ITEM_CHARACTER, SLOTID_NONE,
                                out.v) != 0;
        }

        // func_719: the horse item in a horse slot of the character, and its
        // data (func_670).
        bool HorseItem(int inventory, const Guid &character, Hash slotId, Guid &guid, ItemData &data)
        {
            Guid parent = character;
            if (invoke<int>(NATIVE_INVENTORY_GET_CHILDREN_IN_SLOT_COUNT, inventory, parent.v, slotId) <= 0)
                return false;
            ItemData equipped;
            if (!invoke<int>(NATIVE_INVENTORY_GET_ITEM_EQUIPPED_IN_SLOT, inventory, parent.v, slotId, 0, equipped.v))
                return false;
            ItemData full;
            if (!invoke<BOOL>(NATIVE_INVENTORY_GET_FULL_ITEM_DATA, inventory, equipped.v, full.v, 29, 1))
                return false;
            for (int i = 0; i < 4; ++i)
                guid.v[i] = full.v[i];
            if (!invoke<BOOL>(NATIVE_INVENTORY_IS_GUID_VALID, guid.v))
                return false;
            return invoke<BOOL>(NATIVE_INVENTORY_GET_INVENTORY_ITEM, inventory, guid.v, data.v, FALSE) != 0;
        }

        std::string HorseItemText(const char *name, const Guid &guid, const ItemData &data)
        {
            return Format("%s guid=%s item=%08X slot=%08X equipped=%llu", name, GuidText(guid).c_str(),
                          static_cast<uint32_t>(data.v[4]), static_cast<uint32_t>(data.v[9]), data.v[10]);
        }

        // func_408 -> func_722: swaps the horse items of slots 0 and 1 in the
        // player's inventory. Reads and checks everything first; nothing is
        // written unless every check passes. Returns true once swapped.
        bool SwapHorseItems()
        {
            const int inventory = InventoryId();
            Guid character;
            if (!CharacterGuid(inventory, character))
            {
                Log("Saddle: swap refused, no character GUID (inventory %d)", inventory);
                return false;
            }
            Guid main, temp;
            ItemData mainData, tempData;
            const bool hasMain = HorseItem(inventory, character, SLOTID_ACTIVE_HORSE, main, mainData);
            const bool hasTemp = HorseItem(inventory, character, SLOTID_TEMP_HORSE, temp, tempData);
            Log("Saddle: horse items (inventory %d, character %s): %s | %s", inventory, GuidText(character).c_str(),
                hasMain ? HorseItemText("main", main, mainData).c_str() : "main missing",
                hasTemp ? HorseItemText("temp", temp, tempData).c_str() : "temp missing");
            if (!hasMain || !hasTemp || main == temp)
            {
                Log("Saddle: swap refused, horse items not found");
                return false;
            }
            if (static_cast<uint32_t>(mainData.v[9]) != SLOTID_ACTIVE_HORSE ||
                static_cast<uint32_t>(tempData.v[9]) != SLOTID_TEMP_HORSE)
            {
                Log("Saddle: swap refused, unexpected item slots");
                return false;
            }
            const Hash mainItem = static_cast<Hash>(mainData.v[4]);
            const Hash tempItem = static_cast<Hash>(tempData.v[4]);
            if (!invoke<BOOL>(NATIVE_INVENTORY_FITS_SLOT_ID, mainItem, SLOTID_TEMP_HORSE) ||
                !invoke<BOOL>(NATIVE_INVENTORY_FITS_SLOT_ID, tempItem, SLOTID_ACTIVE_HORSE))
            {
                Log("Saddle: swap refused, items do not fit the other slot");
                return false;
            }
            if (!invoke<BOOL>(NATIVE_INVENTORY_SWAP_ITEM, inventory, main.v, temp.v))
            {
                Log("Saddle: swap failed (_INVENTORY_SWAP_INVENTORY_ITEM), nothing changed");
                return false;
            }
            invoke<BOOL>(NATIVE_INVENTORY_EQUIP_ITEM_WITH_GUID, inventory, main.v, static_cast<BOOL>(tempData.v[10]));
            invoke<BOOL>(NATIVE_INVENTORY_EQUIP_ITEM_WITH_GUID, inventory, temp.v, static_cast<BOOL>(mainData.v[10]));

            Guid newMain, newTemp;
            ItemData newMainData, newTempData;
            const bool okMain = HorseItem(inventory, character, SLOTID_ACTIVE_HORSE, newMain, newMainData);
            const bool okTemp = HorseItem(inventory, character, SLOTID_TEMP_HORSE, newTemp, newTempData);
            Log("Saddle: horse items after swap: %s | %s",
                okMain ? HorseItemText("main", newMain, newMainData).c_str() : "main missing",
                okTemp ? HorseItemText("temp", newTemp, newTempData).c_str() : "temp missing");
            return true;
        }

        // func_315 for one slot: drop its blip; player_horse makes it again
        // (func_66) with the right style.
        void ClearSlotBlip(int slot)
        {
            const int base = G_HORSE_PED_FIRST + slot * HORSE_PED_SIZE;
            int blip = ReadGlobal(base + HORSE_PED_BLIP);
            if (blip && invoke<BOOL>(NATIVE_DOES_BLIP_EXIST, blip))
            {
                invoke<Void>(NATIVE_SET_SCRIPT_CAN_REMOVE_BLIPS, TRUE);
                invoke<Void>(NATIVE_REMOVE_BLIP, &blip);
                WriteGlobal(base + HORSE_PED_BLIP, 0);
            }
            WriteGlobal(base + HORSE_PED_BLIP_STATE, 0);
        }

        // func_116(1, 0): the temp horse becomes the main horse and back.
        bool PromoteTempHorse()
        {
            if (!SwapHorseItems())
                return false;
            ClearSlotBlip(0);
            ClearSlotBlip(1);
            SwapGlobals(G_HORSE_DATA_FIRST, G_HORSE_DATA_FIRST + HORSE_DATA_SIZE, HORSE_DATA_SIZE);
            SwapGlobals(G_HORSE_PED_FIRST, G_HORSE_PED_FIRST + HORSE_PED_SIZE, HORSE_PED_SIZE);
            const Player id = PLAYER::PLAYER_ID();
            const Ped main = SlotPed(0), temp = SlotPed(1);
            invoke<Void>(NATIVE_SET_PED_ACTIVE_PLAYER_HORSE, id, main);
            invoke<BOOL>(NATIVE_SET_PED_AS_TEMP_PLAYER_HORSE, id, temp);
            WriteGlobal(G_SADDLE_MOVED, 1);
            Log("Saddle: temp horse promoted: main=%d (model=0x%08X) temp=%d (model=0x%08X) active=%d temp_native=%d",
                main, ENTITY::GET_ENTITY_MODEL(main), temp, temp ? ENTITY::GET_ENTITY_MODEL(temp) : 0,
                invoke<Ped>(NATIVE_GET_ACTIVE_HORSE_FOR_PLAYER, id), invoke<Ped>(NATIVE_GET_TEMP_PLAYER_HORSE, id));
            return true;
        }

        // func_355 / func_686: the tack items equipped on the player's
        // carried horse equipment (the saddle the player owns).
        std::vector<Hash> PlayerTack()
        {
            std::vector<Hash> items;
            const int inventory = InventoryId();
            Guid character, equipment;
            if (!CharacterGuid(inventory, character) ||
                !invoke<BOOL>(NATIVE_INVENTORY_GET_GUID_FROM_ITEMID, inventory, character.v, ITEM_HORSE_EQUIPMENT,
                              SLOTID_CARRIED_HORSE_EQUIPMENT, equipment.v))
                return items;
            for (const char *slot : TACK_SLOTS)
            {
                Guid item;
                if (!invoke<BOOL>(NATIVE_INVENTORY_GET_ITEM_EQUIPPED_IN_SLOT_BY_REF, inventory, equipment.v, Joaat(slot),
                                  item.v))
                    continue;
                ItemData data;
                if (!invoke<BOOL>(NATIVE_INVENTORY_GET_INVENTORY_ITEM, inventory, item.v, data.v, TRUE))
                    continue;
                const Hash hash = static_cast<Hash>(data.v[4]);
                if (hash)
                    items.push_back(hash);
            }
            return items;
        }

        // ---- Prompts ---------------------------------------------------------

        bool IsValid(const Prompt &prompt)
        {
            return prompt.handle != 0 && invoke<BOOL>(NATIVE_PROMPT_IS_VALID, prompt.handle) != 0;
        }

        void Delete(Prompt &prompt)
        {
            if (IsValid(prompt))
                invoke<Void>(NATIVE_PROMPT_DELETE, prompt.handle);
            prompt.handle = 0;
            prompt.group = -1;
        }

        // Hold prompt, in the lock-on menu of a target (group != 0) or alone.
        void Ensure(Prompt &prompt, const char *label, int group)
        {
            if (IsValid(prompt) && prompt.group == group)
                return;
            Delete(prompt);
            const int handle = invoke<int>(NATIVE_PROMPT_REGISTER_BEGIN);
            invoke<Any>(NATIVE_PROMPT_SET_CONTROL_ACTION, handle, Joaat(g_config.saddleInput.c_str()));
            const std::string text = GameText(label);
            invoke<Void>(NATIVE_PROMPT_SET_TEXT, handle,
                         invoke<const char *>(NATIVE_VAR_STRING, 10, "LITERAL_STRING", text.c_str()));
            invoke<Void>(NATIVE_PROMPT_SET_ENABLED, handle, TRUE);
            invoke<Void>(NATIVE_PROMPT_SET_VISIBLE, handle, FALSE);
            invoke<Void>(NATIVE_PROMPT_SET_HOLD_MODE, handle, HOLD_MS);
            invoke<Void>(NATIVE_PROMPT_SET_PRIORITY, handle, g_config.carePromptPriority);
            if (group != 0)
                invoke<Void>(NATIVE_PROMPT_SET_GROUP, handle, group, 0);
            invoke<Void>(NATIVE_PROMPT_REGISTER_END, handle);
            prompt.handle = handle;
            prompt.group = group;
        }

        void Show(const Prompt &prompt, bool visible)
        {
            if (!IsValid(prompt))
                return;
            invoke<Void>(NATIVE_PROMPT_SET_VISIBLE, prompt.handle, visible ? TRUE : FALSE);
            invoke<Void>(NATIVE_PROMPT_SET_ENABLED, prompt.handle, visible ? TRUE : FALSE);
        }

        bool Completed(const Prompt &prompt)
        {
            return IsValid(prompt) && invoke<BOOL>(NATIVE_PROMPT_HAS_HOLD_MODE_COMPLETED, prompt.handle) != 0;
        }

        float Distance(Entity a, Entity b)
        {
            const Vector3 pa = ENTITY::GET_ENTITY_COORDS(a, TRUE, FALSE);
            const Vector3 pb = ENTITY::GET_ENTITY_COORDS(b, TRUE, FALSE);
            const float dx = pa.x - pb.x, dy = pa.y - pb.y, dz = pa.z - pb.z;
            return sqrtf(dx * dx + dy * dy + dz * dz);
        }

        bool IsSupportedAnimal(Ped ped)
        {
            if (!ped || !ENTITY::DOES_ENTITY_EXIST(ped) || !ENTITY::IS_ENTITY_A_PED(ped) || ENTITY::IS_ENTITY_DEAD(ped))
                return false;
            const MountAdapter adapter(ped);
            return (adapter.IsDonkey() && g_config.patchDonkey) || (adapter.IsMule() && g_config.patchMules);
        }

        // Shop items the ped wears (cached entries, singleplayer).
        std::vector<Hash> ShopItems(Ped ped)
        {
            static const Hash maneDefault = Joaat("HORSE_EQUIPMENT_MANE_DEFAULT");
            static const Hash tailDefault = Joaat("HORSE_EQUIPMENT_TAIL_DEFAULT");
            std::vector<Hash> items;
            const int count = invoke<int>(NATIVE_GET_NUM_COMPONENTS_IN_PED, ped);
            for (int i = 0; i < count && i < 64; ++i)
            {
                BOOL status = FALSE;
                Hash wearable = 0;
                const Hash item =
                    invoke<Hash>(NATIVE_GET_SHOP_ITEM_COMPONENT_AT_INDEX, ped, i, FALSE, &status, &wearable);
                if (item && item != maneDefault && item != tailDefault &&
                    std::find(items.begin(), items.end(), item) == items.end())
                    items.push_back(item);
            }
            return items;
        }

        std::string ItemList(const std::vector<Hash> &items)
        {
            std::string out;
            for (Hash item : items)
                out += Format("%s%08X", out.empty() ? "" : ",", item);
            return out.empty() ? "-" : out;
        }

        Entity CarriedEntity(Ped player)
        {
            if (!invoke<BOOL>(NATIVE_IS_PED_CARRYING_SOMETHING, player))
                return 0;
            const Entity carried = invoke<Entity>(NATIVE_GET_FIRST_ENTITY_PED_IS_CARRYING, player);
            return carried && ENTITY::DOES_ENTITY_EXIST(carried) ? carried : 0;
        }

        void UpdateVariation(Ped ped)
        {
            // player_horse func_362 (tack applied or removed).
            invoke<Void>(NATIVE_UPDATE_PED_VARIATION, ped, FALSE, TRUE, TRUE, TRUE, TRUE);
        }

        // ---- Actions ----------------------------------------------------------

        void DoRemove(Ped player, Ped target)
        {
            const Hash model = Joaat("P_CS_SADDLEBUNDLE01X");
            const std::vector<Hash> before = ShopItems(target);

            // shop_horse_shop_sp func_799: the bundle next to the player.
            const Vector3 at = ENTITY::GET_ENTITY_COORDS(player, TRUE, FALSE);
            const Object bundle =
                invoke<Object>(NATIVE_CREATE_OBJECT, model, at.x + 1.0f, at.y, at.z, TRUE, TRUE, FALSE, FALSE, TRUE);
            invoke<Void>(NATIVE_SET_MODEL_AS_NO_LONGER_NEEDED, model);
            if (!bundle || !ENTITY::DOES_ENTITY_EXIST(bundle))
            {
                Log("Saddle: remove failed, bundle not created");
                return;
            }

            // player_horse func_468 + func_362 with no tack.
            for (const char *category : TACK_CATEGORIES)
                invoke<Void>(NATIVE_REMOVE_SHOP_ITEM_FROM_PED_BY_CATEGORY, target, Joaat(category), 0, FALSE);
            UpdateVariation(target);
            g_tackWorn = before;
            g_tackPed = target;

            const Player id = PLAYER::PLAYER_ID();
            invoke<Void>(NATIVE_SET_PED_CONFIG_FLAG, target, PCF_CORPSE_IS_PERSISTENT, TRUE);
            WriteGlobal(G_SADDLE_BUNDLE, bundle);
            invoke<Void>(NATIVE_SET_PED_AS_SADDLE_HORSE_FOR_PLAYER, id, 0);
            WriteGlobal(G_SADDLE_SLOT, -1);
            WriteGlobal(G_SADDLE_OFF, 1);
            invoke<Void>(NATIVE_TASK_CARRIABLE, bundle, Joaat("CARRIABLE_SADDLE_BUNDLE"), player, 0, 0);
            Log("Saddle: removed from model=0x%08X ped=%d bundle=%d worn=%s", ENTITY::GET_ENTITY_MODEL(target), target,
                bundle, ItemList(before).c_str());
        }

        void DoPlace(Ped target, Entity bundle)
        {
            // player_horse func_153, equip branch, horse in slot 0.
            invoke<Void>(NATIVE_DETACH_CARRIABLE_ENTITY, bundle, FALSE, FALSE);
            invoke<Void>(NATIVE_SET_ENTITY_AS_MISSION_ENTITY, bundle, TRUE, TRUE);
            Object object = bundle;
            invoke<Void>(NATIVE_DELETE_OBJECT, &object);
            int blip = ReadGlobal(G_SADDLE_BUNDLE_BLIP);
            if (blip && invoke<BOOL>(NATIVE_DOES_BLIP_EXIST, blip))
            {
                invoke<Void>(NATIVE_SET_SCRIPT_CAN_REMOVE_BLIPS, TRUE);
                invoke<Void>(NATIVE_REMOVE_BLIP, &blip);
                WriteGlobal(G_SADDLE_BUNDLE_BLIP, 0);
            }
            WriteGlobal(G_SADDLE_BUNDLE, 0);

            if (!invoke<BOOL>(NATIVE_GET_ENTITY_CARRYING_FLAG, target, CARRYING_FLAG_CAN_CARRY_ANYTHING))
                invoke<Void>(NATIVE_SET_ENTITY_CARRYING_FLAG, target, CARRYING_FLAG_CAN_CARRY_ANYTHING, TRUE);
            invoke<Void>(NATIVE_SET_PED_CONFIG_FLAG, target, PCF_CORPSE_IS_PERSISTENT, FALSE);
            invoke<Void>(NATIVE_SET_PED_AS_SADDLE_HORSE_FOR_PLAYER, PLAYER::PLAYER_ID(), target);
            WriteGlobal(G_SADDLE_SLOT, 0);
            WriteGlobal(G_SADDLE_OFF, 0);

            // The player's tack from the inventory, as vanilla (func_355 +
            // func_696); else what the animal wore when the mod took it off;
            // else the model's generic saddle (func_361).
            const std::vector<Hash> inventoryTack = PlayerTack();
            const char *source = "inventory";
            std::vector<Hash> tack = inventoryTack;
            if (tack.empty() && g_tackPed == target)
            {
                tack = g_tackWorn;
                source = "worn before";
            }
            for (const char *category : TACK_CATEGORIES)
                invoke<Void>(NATIVE_REMOVE_SHOP_ITEM_FROM_PED_BY_CATEGORY, target, Joaat(category), 0, FALSE);
            if (tack.empty())
            {
                source = "generic META_HORSE_SADDLE_ONLY";
                invoke<Void>(NATIVE_EQUIP_META_PED_OUTFIT, target, Joaat("META_HORSE_SADDLE_ONLY"));
            }
            for (Hash item : tack)
                invoke<Void>(NATIVE_APPLY_SHOP_ITEM_TO_PED, target, item, FALSE, FALSE, FALSE);
            UpdateVariation(target);
            Log("Saddle: placed on model=0x%08X ped=%d tack from %s=%s (inventory=%s, worn before=%s)",
                ENTITY::GET_ENTITY_MODEL(target), target, source, ItemList(tack).c_str(),
                ItemList(inventoryTack).c_str(), g_tackPed == target ? ItemList(g_tackWorn).c_str() : "-");
        }

        void Finish(ULONGLONG now)
        {
            g_loadingBundle = false;
            g_removeTarget = 0;
            g_cooldownUntil = now + COOLDOWN_MS;
            // A lock-on menu prompt stays greyed once it fired.
            Delete(g_remove);
            Delete(g_place);
        }

        void UpdateLoading(Ped player, ULONGLONG now)
        {
            if (!IsSupportedAnimal(g_removeTarget))
            {
                Log("Saddle: remove cancelled, animal gone");
                Finish(now);
            }
            else if (invoke<BOOL>(NATIVE_HAS_MODEL_LOADED, Joaat("P_CS_SADDLEBUNDLE01X")))
            {
                DoRemove(player, g_removeTarget);
                Finish(now);
            }
            else if (now >= g_deadline)
            {
                Log("Saddle: remove failed, bundle model not loaded");
                Finish(now);
            }
        }

        void LogState(const std::string &state)
        {
            if (state == g_lastState)
                return;
            g_lastState = state;
            Log("Saddle: %s", state.c_str());
        }
    }

    void Update(Ped trackedMount)
    {
        (void)trackedMount;
        if (!g_config.saddlePrompts)
            return;
        const Ped player = PLAYER::PLAYER_PED_ID();
        if (!player || !ENTITY::DOES_ENTITY_EXIST(player))
            return;

        // Prompts registered before a save load are gone.
        if (g_remove.handle && !IsValid(g_remove))
            g_remove = Prompt{};
        if (g_place.handle && !IsValid(g_place))
            g_place = Prompt{};

        const ULONGLONG now = GetTickCount64();
        if (g_loadingBundle)
        {
            Show(g_remove, false);
            Show(g_place, false);
            UpdateLoading(player, now);
            return;
        }

        if (ENTITY::IS_ENTITY_DEAD(player) || PED::IS_PED_ON_MOUNT(player) || !GlobalsOk())
        {
            Show(g_remove, false);
            Show(g_place, false);
            return;
        }

        // Main horse (slot 0) and, with SaddleTempHorse, the temp horse
        // (slot 1), when they are a donkey / mule. The temp horse only while
        // there is a main horse to swap with: with an empty slot 0 (main
        // horse sold) vanilla moves the horse item instead, not supported.
        const Ped mainHorse = SlotPed(0);
        const Ped tempHorse = SlotPed(1);
        const bool mainOk = IsSupportedAnimal(mainHorse);
        const bool tempOk = g_config.saddleTempHorse && mainHorse && ENTITY::DOES_ENTITY_EXIST(mainHorse) &&
                            IsSupportedAnimal(tempHorse);
        if (!mainOk && !tempOk)
        {
            Show(g_remove, false);
            Show(g_place, false);
            return;
        }

        const Player id = PLAYER::PLAYER_ID();
        Entity lockOn = 0;
        if (!invoke<BOOL>(NATIVE_GET_PLAYER_INTERACTION_TARGET_ENTITY, id, &lockOn, FALSE, FALSE))
            lockOn = 0;
        const float mainDistance = mainOk ? Distance(player, mainHorse) : 1e9f;
        const float tempDistance = tempOk ? Distance(player, tempHorse) : 1e9f;
        // The animal the player faces: the locked-on one, else the nearest.
        Ped target = 0;
        if (mainOk && lockOn == mainHorse)
            target = mainHorse;
        else if (tempOk && lockOn == tempHorse)
            target = tempHorse;
        else
            target = mainDistance <= tempDistance ? (mainOk ? mainHorse : 0) : (tempOk ? tempHorse : 0);
        const bool isTemp = target != 0 && target == tempHorse;
        const bool locked = target && lockOn == target;
        const bool isNear = target && (isTemp ? tempDistance : mainDistance) <= g_config.careMaxDistance;
        const Ped saddleHorse = invoke<Ped>(NATIVE_GET_SADDLE_HORSE_FOR_PLAYER, id);
        const int saddleSlot = ReadGlobal(G_SADDLE_SLOT);
        const Entity carried = CarriedEntity(player);
        const bool carriesSaddle = carried && ENTITY::GET_ENTITY_MODEL(carried) == Joaat("P_CS_SADDLEBUNDLE01X");

        const bool canRemove = !isTemp && locked && isNear && !carried && saddleHorse == target && saddleSlot == 0;
        const bool canPlace = isNear && carriesSaddle && saddleHorse == 0 && saddleSlot == -1;
        LogState(Format("main=%d (0x%08X) temp=%d (0x%08X) target=%s locked=%d near=%d saddle_horse=%d "
                        "saddle_slot=%d saddle_off=%d bundle=%d carried=%d%s -> remove=%d place=%d",
                        mainHorse, mainHorse ? ENTITY::GET_ENTITY_MODEL(mainHorse) : 0, tempHorse,
                        tempHorse ? ENTITY::GET_ENTITY_MODEL(tempHorse) : 0,
                        !target ? "-" : (isTemp ? "temp" : "main"), locked ? 1 : 0, isNear ? 1 : 0, saddleHorse,
                        saddleSlot, ReadGlobal(G_SADDLE_OFF), ReadGlobal(G_SADDLE_BUNDLE), carried,
                        carried ? Format(" (model=0x%08X)", ENTITY::GET_ENTITY_MODEL(carried)).c_str() : "",
                        canRemove ? 1 : 0, canPlace ? 1 : 0));

        const bool ready = now >= g_cooldownUntil;
        const int group = locked ? invoke<int>(NATIVE_PROMPT_GET_GROUP_ID_FOR_TARGET_ENTITY, target) : 0;
        if (canRemove)
            Ensure(g_remove, "REMOVE_SADDLE", group);
        if (canPlace)
            Ensure(g_place, "PLACE_SADDLE", group);
        Show(g_remove, canRemove && ready);
        Show(g_place, canPlace && ready);
        if (!ready)
            return;

        if (canRemove && Completed(g_remove))
        {
            invoke<Void>(NATIVE_REQUEST_MODEL, Joaat("P_CS_SADDLEBUNDLE01X"), FALSE);
            g_loadingBundle = true;
            g_removeTarget = target;
            g_deadline = now + MODEL_TIMEOUT_MS;
            Show(g_remove, false);
        }
        else if (canPlace && Completed(g_place))
        {
            Show(g_place, false);
            if (isTemp)
            {
                if (!PromoteTempHorse())
                {
                    Log("Saddle: place on the temp horse refused");
                    Finish(now);
                    return;
                }
            }
            DoPlace(target, carried);
            Finish(now);
        }
    }
}
