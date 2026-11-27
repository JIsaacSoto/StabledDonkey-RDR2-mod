# StabledDonkey

**English** | [Español](README.es.md)

**Donkeys and mules as horses in Red Dead Redemption 2 (Story Mode): bonding, care, stats, stable and persistence.**

StabledDonkey makes the donkey and the mule work like any other horse breed, using the game's own systems. It does not replace their models with horses or modify any game files: every change is applied in memory at startup.

## What it does (v0.6.0)

### Donkey

- **Natural bonding:** riding, feeding, brushing and patting give bonding XP, with the game's own toast and level-ups.
- **Info card and stats:** "Show Info" and `Player → Horse` show the donkey's card with its data.
- **Stable:** you can store the donkey, set it as your main horse and take it out again. The real donkey comes back, with its name and appearance, also after restarting the game.
- **On-foot care:** lock on to the donkey (right click) to get **Pat (G)**, **Brush (B)** and **Feed (R)**, the same keys as a horse. As in the base game, Pat is available from the start (it is how the bond begins), while Brush and Feed need some bond. Brushing cleans it, and feeding uses up the food and applies its effects, just like in the game.
- **Mounted care:** pat (G) and feed work as on a horse. To brush, a **Brush (R)** prompt appears when the donkey is dirty, standing still and you are not in a mission.
- Feeding uses the same food as the game: horse food first and, if you have none left, the herbs the game also accepts (ginseng, sage, mint…).
- The prompt texts are the game's own, so they show in your game language.
- It answers the whistle, shows on the map as your horse and has the lead, follow, stay and flee options.

### Mule and painted mule

- **Natural bonding**, which the base game blocked for them: riding, feeding, brushing and patting give XP and raise the level.
- **Info card and stats** in "Show Info" and `Player → Horse`. It shows on the map as your horse.
- **Stable:** the mule and the painted mule can be stored, set as the main horse, answer the whistle and are kept on save load, both outside and inside the stable. In the stable their title is the donkey's (they use the donkey's item), but their breed is still shown as mule or painted mule.
- **On-foot care:** the mod's **Pat (G)**, **Brush (B)** and **Feed (R)**, because the mule's own prompts are sometimes greyed out. Pat is available from the start; Brush and Feed need some bond.
- **Mounted care:** pat and feed as on a horse, and **Brush (R)** when it is dirty.

### SuperDonkMode (optional)

Off by default (`SuperDonkMode=0`). When on, the donkey or mule you ride gets better stats through the game's own attribute natives, like a stat boost mod. By default it gets the speed and acceleration of the White Arabian and the health and stamina of the Dark Bay Turkoman; every value can be changed in the `.ini`.

- **Mules:** all four stats apply (health, stamina, speed and acceleration).
- **Donkey:** speed and acceleration apply. Its health and stamina are fixed by its model and do not change, but they are already about a Turkoman's (stamina) or higher (health).
- The TAB wheel and "Show Info" show the boosted stats. The stable and `Player → Horse` show the breed's original stats.
- Saddle bonuses still add on top, as on any horse.

## Known limitations

- The on-foot care prompts belong to the mod. After each on-foot action you have to release and lock on to the animal again (right click) to repeat.
- In the stable, the title of both mules is the donkey's: the game has no catalog item for mules, so the mod gives them the donkey's. Their breed is still shown as mule.
- The TAB-wheel brush is greyed out on donkeys and mules (it works on horses); use the mounted Brush instead.
- The "Remove Saddle" option does not appear on the donkey or the mule, not even as the main horse.
- In "Show Info", the coat field of the donkey and the mule shows the text `~lp~` (the game has no coat text for them). It is cosmetic only.
- SuperDonkMode cannot raise the donkey's health or stamina (see above).
- Stable Expansion compatibility: not tested.

## Requirements

- Red Dead Redemption 2 **Story Mode** on PC, tested version **1.0.1491.50** (Steam).
- [ScriptHookRDR2](http://www.dev-c.com/rdr2/scripthookrdr2/) (includes the ASI loader).
- **Do not use in Red Dead Online.**

The mod finds what it needs in memory through signatures and comparisons, with no fixed offsets. On another game version, any part it cannot find safely turns itself off and says so in the log. Even so, it is only tested on 1.0.1491.50.

## Installation

1. Install ScriptHookRDR2.
2. Copy `StabledDonkey.asi` (and, if you want to change options, `StabledDonkey.ini`) into the game folder, next to `RDR2.exe`.
3. Start Story Mode. After the save loads, the mod takes about 20 seconds to prepare the scripts. It does this again whenever you load another save.

If you are updating and use your own `StabledDonkey.ini`, take the new `.ini`. A `StableMuleModel` line from v0.4.0 is ignored and can be removed.

### Uninstalling

**Before removing the mod, take every donkey and mule out of the stable** (or sell them). Then delete `StabledDonkey.asi` and `StabledDonkey.ini`.

The mod does not modify game files. The only thing left in your save is what the game itself stores: the stable animals and the main horse. Tested without the mod (2026-11-26):

- **With a donkey or a mule inside the stable, the "Manage Horses" menu is empty** (it only lets you go back). Without the mod the game cannot show them. Once the mod is installed again, the stable works again.
- A mule kept as the main horse still spawns and accepts the original interactions, but it no longer gives bonding points, its card is that of a plain animal and its bond goes back to level 1.

## Configuration

`StabledDonkey.ini` is optional; without it the default values are used. Its comments are in English and Spanish.

| Option | Default | What it does |
|---|---|---|
| `PatchDonkey` | `1` | Treats the donkey as a horse (model data and scripts). |
| `PatchMules` | `1` | The same for the mule and the painted mule. |
| `StableMules` | `1` | Stable and persistence for the mule and the painted mule. |
| `CarePrompts` | `1` | The mod's care prompts (on foot; mounted brush). |
| `CareMuleOnFoot` | `1` | The mod's on-foot prompts for mules too (0 = the original ones). |
| `CareInputPat` / `CareInputBrush` / `CareInputFeed` | G / B / R | Controls for the on-foot prompts. |
| `CareInputMountedBrush` | `INPUT_CONTEXT_X` (R) | Control for the mounted Brush. |
| `CareMountedBrushMinDirt` | `25` | Minimum dirt (0–100) to show the mounted Brush. |
| `CareMaxDistance` | `2.2` | Maximum distance (m) for the on-foot prompts. |
| `HideVanillaPromptTypes` | `5,44,47,49,50,52` | Game prompts hidden on the donkey. |
| `HideVanillaPromptTypesMule` | `49,50` | Game prompts hidden on the mule (only brush and feed). |
| `CarePromptPriority` | `2` | Priority of the mod's prompts over the game's (0–3). |
| `SuperDonkMode` | `0` | Stat boost for the donkey and mules you ride (1 = on). |
| `SuperDonkHealth` / `SuperDonkStamina` | `6` / `4` | Health and stamina rank (0–9; the stable shows rank + 1). Default: Dark Bay Turkoman. Mules only. `-1` = unchanged. |
| `SuperDonkSpeed` / `SuperDonkAcceleration` | `5` / `5` | Speed and acceleration rank (0–9). Default: White Arabian. `-1` = unchanged. |

## Building from source

Requirements: Visual Studio 2022 or later (or Build Tools) with C++ and CMake, and the ScriptHookRDR2 SDK.

1. Download the SDK from the official page and extract it into `sdk/ScriptHookRDR2` (that folder is not committed; the SDK cannot be redistributed).
2. Run `build.cmd`, or:

```powershell
cmake -S . -B build -A x64 -DSCRIPTHOOK_RDR2_SDK="C:/path/to/ScriptHookRDR2"
cmake --build build --config Release
```

The output is `build/Release/StabledDonkey.asi`.

## How it works

Summary; the details are in [docs/TECHNICAL-NOTES.md](docs/TECHNICAL-NOTES.md) ([español](docs/NOTAS-TECNICAS.md)).

- `src/native/ModelInfoPatch.cpp`: writes the horse values (animal type and other fields) into the in-memory model data of the donkey and the mules. The offsets are discovered by comparing donkey, mule and horse.
- `src/native/ScriptPatch.cpp`: removes the explicit "is it a donkey / a mule?" checks from the loaded scripts (`player_horse`, `shop_horse_shop_sp`) and maps both mules to a catalog item for the stable. It keeps the breed and catalog data the game uses to save and restore the animals.
- `src/native/CarePrompts.cpp`: the care prompts. Brush and feed start the game's own item-use sequence.
- `src/native/SuperDonk.cpp`: SuperDonkMode, the optional stat boost.
- `src/main.cpp`: startup, main loop and logging of the mount and the bond.

## Log

The mod writes `StabledDonkey.log` in the game folder. It is reset on every start. If you report a problem, attach it and say what you did and with which animal.

## Credits

- Decompiled scripts: [Halen84/RDR3-Decompiled-Scripts](https://github.com/Halen84/RDR3-Decompiled-Scripts).
- Native database: [alloc8or/rdr3-nativedb-data](https://github.com/alloc8or/rdr3-nativedb-data).
- Controls and prompt types: [femga/rdr3_discoveries](https://github.com/femga/rdr3_discoveries) and [Halen84/RDR3-Native-Flags-And-Enums](https://github.com/Halen84/RDR3-Native-Flags-And-Enums).
- Script opcode table: [maybegreat48/GTA-V-Script-Decompiler](https://github.com/maybegreat48/GTA-V-Script-Decompiler).
- Text label names: [OpenIV-Team/RAGE-StringsDatabase](https://github.com/OpenIV-Team/RAGE-StringsDatabase).
- Model lookup signature: the RDR2Playground project.

## License

This project does not include a reuse license yet. All rights to the source code are reserved by its author until an explicit license is published. External components keep their own terms.
