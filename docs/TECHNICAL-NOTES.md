# Technical notes

**English** | [Español](NOTAS-TECNICAS.md)

How StabledDonkey works inside, what was tested and what was ruled out. Everything refers to RDR2 Story Mode PC **1.0.1491.50**. The game scripts were read decompiled (Halen84/RDR3-Decompiled-Scripts, folder 1491.50) and the natives in alloc8or/rdr3-nativedb-data.

## General idea

The game already ships almost everything a "horse" donkey needs:
- the catalog (`catalog_sp.ymt`) has the `HORSE_DONKEY` item (`0xEF006BE0`, model `A_C_Donkey_01`);
- the scripts know `BREED_DONKEY`, the "Other" class and the `HORSE_DONKEY` ↔ `A_C_DONKEY_01` mapping.

What blocks it is a few model data fields and some explicit checks in the scripts. The mod fixes both **in memory**, without touching game files. That is why the real donkey is stored in the stable and comes back as it was when the save loads: there is no substitute horse.

## 1. Model data (`src/native/ModelInfoPatch.cpp`)

In `metapeds.ymt`, horse, mule and donkey differ in some `CPedModelInfo` fields (0x380 bytes in this version). The module gets the live objects of the donkey, the mule and the Tennessee Walker, using the RDR2Playground model lookup signature and checking that each object holds the model hash. It then **discovers the offsets by comparing them**: there are no fixed offsets in the code. A field is only written if there is exactly one candidate and the current value is the expected one.

| Field (metapeds) | Horse | Donkey (original) | Offset found |
|---|---|---|---|
| `0x9ED31CFA` animal type | `AT_HORSE` | `AT_DONKEY` | 0x31C |
| `0x7BA672C7` | `HORSE` | `DONKEY` | 0x310 |
| `0x3CF2AF47` | `HORSE` | `default` | 0x188 |
| `0xB6F6DE8E` (3 floats) | 25/50/100 | 0/0/0 | 0x360 |
| `0xF45D226F` (byte) | 1 | 0 | 0x36C |
| `MotionTaskDataSetName`, `TaskDataName` | `HORSE` | `DONKEY` | 0x2BC, 0x2D4 |
| `DecisionMakerName` | `0xC87F6A22` | `0x98D8BAF4` | 0x2D8 |
| `0x3207FDF6`, `0xC4E3A8E3`, `0x2799400B` | horse | donkey | 0x120, 0x2DC, 0x314 |

The tasks and the DecisionMaker are what made mounted patting possible. The full set in the table is the configuration tested in game. With it the donkey moves, sounds and animates normally.

## 2. Script checks (`src/native/ScriptPatch.cpp`)

- `player_horse`: `func_392` (a `switch` with `A_C_DONKEY_01`) and `func_393` (mules) make the script ignore bonding, care and stats for those animals.
- `shop_horse_shop_sp`: `GET_ENTITY_MODEL(mount) == A_C_DONKEY_01` ×3 → `TF_SHOP_HORSE_KICK_OUT_DONKEY` ("you can't stable a donkey").

The module finds each script in memory through a unique string (`DEAD_HORSE_SADDLE_SWAP_LONG_LOOT_ANIM_DATA`, `TF_SHOP_HORSE_KICK_OUT_DONKEY`). It then changes only two bytecode shapes: small `switch`es made only of donkey or mule hashes, and the grouped comparisons inside the stable script. The hash is replaced by a marker (`0x5D0D0001`…). Every other use of the donkey hash (breed, catalog) is left intact.

Real opcodes on PC 1491.50 (different from the public console tables): `SWITCH`=0x3C with a u16 count, `LOCAL_U8_LOAD`=0x66, `PUSH_CONST_U32`=0x37, `IEQ`=0x0B.

Practical details:
- The scan only walks private memory. Mapped or write-combined (GPU) memory is extremely slow to read and used to hang the scan. The fast pass takes about 20 s after loading.
- ScriptHook restarts the mod's `ScriptMain` when a save loads, and the game reloads its scripts with the original checks. That is why `ScriptPatch::Reset()` scans again on every load, and it also rescans if a patched site changes.

## 3. Care prompts (`src/native/CarePrompts.cpp`)

On foot, the engine shows pat, brush and feed **greyed out** on the donkey, and the TAB-wheel brush is greyed out too. The interactions themselves work: `IS_ANIMAL_INTERACTION_POSSIBLE` = 1 and `TASK_ANIMAL_INTERACTION` runs.

Ruled out as the cause of the grey:
- Per-ped or global prompt modes (`_GET_PLAYER_UI_PROMPT_FOR_PED_IS_ENABLED`): all 0.
- Mount ownership (`_SET_PLAYER_OWNS_MOUNT`): it is set, but changes nothing.
- The `CPedModelInfo` data where the donkey differs from horse and mule: simple values, animation hashes, audio, layout, IK and the references to shared cards (0x1E0, 0x1F8, 0x200). None removes the grey. Only the scale (0.67) and a few donkey-only pointers were left untested, and changing those makes no sense.

Conclusion: the filter is in the engine code. Patching the executable was ruled out as fragile and bad for compatibility.

Solution: the mod's own prompts.
- On foot, inside the donkey's right-click menu, with the horse keys: `INPUT_INTERACT_LOCKON_ANIMAL` (G), `INPUT_INTERACT_HORSE_BRUSH` (B) and `INPUT_INTERACT_HORSE_FEED` (R). The game's grey prompts are hidden on that donkey with `_MODIFY_PLAYER_UI_PROMPT_FOR_PED(..., PP_MODE_HIDE)`.
- Brush and feed write the request of the `player_horse` item-use state machine (`Global_1900372`: [0] state, 1 = idle; [1] item; [2] ped; [5] request). They are the same writes the vanilla prompt handler does (`func_788`). So the game validates, plays the animation, uses up the item and applies cleaning, cores and bonding.
- Pat uses `TASK_ANIMAL_INTERACTION` with `INTERACTION_POSITIVESHORT`.
- Food: the same list as `player_horse` `func_789`. First 11 horse foods (hay, oatcakes, apple…) and, if none is left, 26 herbs (ginseng, sage, mint…), all accepted by its `func_724` check. Up to v0.5.0 the mod only had the first 11, and the prompt disappeared when only herbs were left.
- Prompt texts: the game's own labels, read with `DOES_TEXT_LABEL_EXIST` (`0x73C258C68D6F55B6`) and `_GET_LABEL_TEXT_2` (`0x3429670F9B9EF2D3`); English text if they are missing. Checked in Spanish (language 11 from `GET_CURRENT_LANGUAGE`): `INTERACT_HORSE` "Acariciar", `BRUSH_HORSE` "Cepillar", `FEED_HORSE` "Alimentar", `CALM_HORSE` "Tranquilizar", `REMOVE_SADDLE` "Quitar silla", `MOUNT_HORSE` "Montar caballo", `BREED_DONKEY` "Burro", `BREED_MULE` "Mula", `BREED_MULE_PAINTED` "Mula (manchada)". In English, `INTERACT_HORSE` is "Pat" (seen in the decompiled scripts); the other English texts are not checked. There is no "brush donkey" label; the mounted prompt uses `BRUSH_HORSE`.
- Mounted, Brush uses `INPUT_CONTEXT_X`, because the `INPUT_INTERACT_*` controls do not exist while mounted. It only appears with dirt ≥ 25 (attribute 16), the donkey standing still and no mission.

Limitation: during the animation the engine drops the lock-on target and does not take it back even if right click is held. There is no native to force it.

## Tested in game (v0.2.0)

- Natural bonding with the XP toast; "Show Info" card and stats in `Player → Horse`.
- Stable: stores and returns the real donkey with its name, tail and card, also after restarting the game.
- Mounted: pat (G), feed and Brush.
- On foot: pat, brush (cleans) and feed (uses up the food); bond +1 brushing or feeding and +5 patting.
- A wild donkey (Rampage) can be ridden, gains bond, shows on the map and answers the whistle.

## Mules (v0.3.0 to v0.5.0)

- In `metapeds` the mule is almost a horse. Only the animal type (`AT_MULE`), `0x7BA672C7` (empty), `0x3CF2AF47`, the floats, the flag and `0x2FC486F7` differ. `ModelInfoPatch` applies the same base fields as for the donkey (not the donkey-only ones).
- `player_horse` `func_393` blocked their bonding and care; `ScriptPatch` neutralises it like `func_392`.
- Once bonded, the original on-foot prompts (pat, brush, feed) **work on the mule**. So the donkey's grey belongs to the donkey (most likely its skeleton, `META_Base_Donkey_01`; the mule uses the horse one) and not to the data we share.
- Sometimes those mule prompts are greyed out (also with the v0.1 hook, and it varies between loads). The cause is not identified; in the diagnosis the mule showed up as a temporary horse with no registered owner and no active prompt modes. Since v0.4.0 the mule uses the mod's prompts (`CareMuleOnFoot`).
- The TAB-wheel brush is greyed out on donkey and mule but not on the horse. Giving them the horse's breed reference (`0xAAD9DD0F`, offset 0x370) was tried: it does not enable it and makes the card show the Tennessee Walker's stats. Ruled out; the mounted Brush is used instead.
- **Stable (v0.4.0):**
  - Each stable slot stores the **model**. The item is computed on the fly by the model → item functions (`shop_horse_shop_sp` `func_1318`, `player_horse` `func_542` and other copies), and the menu hides rows whose item is not valid. There is no `HORSE_MULE` item, so in the base game the mule is stored without a row or title.
  - Those functions start with `if (model == A_C_HORSE_MP_MANGY_BACKUP) return HORSE_BREED_MANGY_BACKUP;`, a multiplayer-only horse. In bytecode: `37 <0x30331B80> 15 08 00 37 <0x215F7CAE> 50 01 01` (the `if` uses the fused compare-and-jump opcode `0x15`, not `IEQ`). The mod rewrote both constants: mule → `HORSE_DONKEY`. It was applied in 6 sites.
  - Tested result (painted mule, 2026-11-26): row and price in the stable (title "Burro", breed "Mula"), main horse, whistle, map and persistence on load, outside and inside the stable.
  - In v0.4.0 only one model fitted (the regular mule).
  - The `switch` that follows that check is **sorted by value** (signed), so a case cannot be renamed without breaking the order. Rebuilding the table by sacrificing a real horse was tried and dropped because of its cost.
  - The default case (`return 0`) is not redirected either: the gang horses (Bill, Charles, Dutch…), several special horses and the multiplayer one have no item either, and the game keeps them out of the stable on purpose. With that change they could be stored as "donkey".
  - **v0.5.0 (tested 2026-11-26):** the 18 bytes of the multiplayer case are rewritten as a two-case `switch`: `66 00 | 3C 02 00 | <mule> <jump> | <painted mule> <jump> | 13` (NOP). Both cases jump to the already existing body of the donkey case (`37 <HORSE_DONKEY> 50 01 01`); every other model falls through to the original `switch` unchanged. The stack stays as before.
  - `switch` jumps count from the **end of each entry** (confirmed in the log: only one interpretation makes the ~81 cases land on `37 xxxx 50 01 01`; the mod detects it on every load). `NOP` = `0x13` comes from the opcode table of maybegreat48/GTA-V-Script-Decompiler (`ShuffledOpcodes`, RDR2 game), which matches every opcode already seen in memory.
  - Result: painted mule with its stable row, name and mane / tail changed in the shop, main horse, whistle, map and persistence on load, outside and inside the stable; the regular mule and the donkey work as before. In test1 it was written in 6 of 7 sites. The seventh (always at an address ending in `…FE56`, 0x3E56 into a 16 KB block) was not recognised and kept the v0.4.0 change (regular mule only). The test2 log gave the reason: with the right base, cases 0–78 of 81 land on `return const`, but case 79 (`A_C_HORSE_APPALOOSA_BLANKET`) jumps 3 bytes off the pattern of the other bodies. Since test3 up to 2 such cases are tolerated if the donkey case is exact (it is the only one used), and the log shows the bytes of the odd case. Test3 result (tested 2026-11-26): that case starts with `68 02 00` (J +2), skips two `13` (NOP) and reaches a normal `37 xxxx 50 01 01`, that is, compiler padding (probably a code page boundary). With that, all 7 sites are written (`both_mules` 7–8, `mule_only=0`).
  - The stable script's own mule check (`func_2186`) is left intact. It sits in the same memory region as the stable's marker string, unlike the `player_horse` ones.
- **Uninstalling (tested 2026-11-26):** a save with the mule as the main horse and a donkey, the painted mule and a horse in the stable. Without the mod, the "Manage Horses" menu is empty (hypothesis: the stable script cannot draw those slots and aborts the whole list). The main mule spawns, its original interactions work without giving bond, its card is that of a plain animal and the bond drops to level 1. Reinstalling the mod makes everything work again (except the painted mule, as before v0.5.0).
- When the player has no main horse left, `player_horse` uses a default horse (`func_970`: chestnut Tennessee Walker). That is not the mod.
- "Remove Saddle" (H, `INPUT_SADDLE_TRANSFER`) is an engine prompt with no equivalent native or task. `player_horse` only receives `EVENT_SADDLE_TRANSFER` once it has happened. It does not appear even with the mule as the main horse, so it does not depend on the skeleton.

## SuperDonkMode (v0.6.0, `src/native/SuperDonk.cpp`)

- Horse stats are attribute ranks (`ePedAttribute`: 0 health, 1 stamina, 5 speed, 6 acceleration). The scripts never set the base ranks of those four: the engine takes them from the breed. Saddles add **bonus** ranks to speed and acceleration, and `player_horse` rewrites those bonus ranks of the player's horse from its tack on every update (`func_662`–`func_664`). That is why the mod sets the **base** rank (`SET_ATTRIBUTE_BASE_RANK`, `0x5DA12E025D47D4E5`) and the tack still adds on top.
- The stable shows each stat as base rank + 1 out of 10 (`shop_horse_shop_sp`, `"HorseSpeedValue"`). The game's own default ranks, read in game with `GET_DEFAULT_ATTRIBUTE_RANK` (`0x958DD43D41F89A47`): White Arabian health 4, stamina 4, speed 5, acceleration 5; Dark Bay Turkoman 6, 4, 5, 4 (max rank 9). Plus 1 they match the published guides (5/5/6/6 and 7/5/6/5). Donkey and mules: 0 in all four.
- The mod checks once per second the donkey or mule being ridden and the last one ridden, and sets the ranks again if they differ.
- Tested in game (2026-11-26):
  - Horses and mules: max health and max stamina = 60 + 20 × rank (regular horse rank 2: 100 / 100; boosted mule rank 6 / 4: 180 / 140).
  - The TAB wheel and "Show Info" show the boosted stats; the stable and `Player → Horse` show the breed's defaults.
  - The donkey takes the four ranks (speed and acceleration apply), but its max health (251) and max stamina (300) do not change. Galloping drains about 6.3 stamina points every 5 s on every animal. What tires the animal is the normalised stamina (`_GET_PED_STAMINA_NORMALIZED`), whose hidden range is about 60 on a rank 2 horse, about 100 on a rank 4 mule and about 104–107 on the donkey **with or without** the boost. So the donkey's stamina is fixed at about rank 4 and its health is above any horse rank (rank 9 = 240).
  - There is no native to set max stamina. `_SET_STAMINA_DEPLETION_MULTIPLIER` could emulate more stamina on the donkey, but `player_horse` rewrites it on the player's horse from the tack; left out.

## Natives and globals used

| Use | Hash |
|---|---|
| `TASK_ANIMAL_INTERACTION` | `0xCD181A959CFDD7F4` |
| `_IS_ANIMAL_INTERACTION_RUNNING` | `0x7FC84E85D98F063D` |
| `GET_PLAYER_INTERACTION_TARGET_ENTITY` | `0x3EE1F7A8C32F24E1` |
| `_MODIFY_PLAYER_UI_PROMPT_FOR_PED` | `0xA3DB37EDF9A74635` |
| `_UI_PROMPT_*` (register, text, hold, group, visible, enabled) | see `CarePrompts.cpp` |
| `_INVENTORY_GET_INVENTORY_ITEM_COUNT_WITH_ITEMID` | `0xE787F05DFC977BDE` |
| `GET_ATTRIBUTE_BASE_RANK` (dirt = 16) | `0x147149F2E909323C` |
| `GET_MISSION_FLAG` | `0xB15CD1CF58771DE1` |
| `DOES_TEXT_LABEL_EXIST` | `0x73C258C68D6F55B6` |
| `_GET_LABEL_TEXT_2` | `0x3429670F9B9EF2D3` |
| `GET_CURRENT_LANGUAGE` | `0xDB917DA5C6835FCC` |
| `SET_ATTRIBUTE_BASE_RANK` | `0x5DA12E025D47D4E5` |
| `GET_MAX_ATTRIBUTE_RANK` | `0x704674A0535A471D` |
| `GET_DEFAULT_ATTRIBUTE_RANK` | `0x958DD43D41F89A47` |
| `GET_PED_MAX_HEALTH` | `0x4700A416E8324EF3` |
| `_GET_PED_MAX_STAMINA` | `0xCB42AFE2B613EE55` |
| `player_horse` item-use global | `Global_1900372` |
