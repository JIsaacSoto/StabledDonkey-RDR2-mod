# Changelog

**English** | [Español](CHANGELOG.es.md)

## v0.8.0 — 2026-11-27

Remove Saddle and Saddle on the donkey and the mules.

### Added

- **Remove Saddle (H, hold)** on the main donkey or mule, in its lock-on menu. The player carries the saddle as with a horse, and the game's own "Saddle" prompt puts it on any horse.
- **Saddle (H, hold)** with a saddle in the hands, next to the donkey or mule: the player's saddle and tack from the inventory, as the game does on horses. The saddlebag items travel with the saddle.
- Saddling the donkey or mule that is the temp horse makes it the main horse, and the old main horse the temp horse, as the game does with horses. Kept on save load (`SaddleTempHorse=1`).
- New options: `SaddlePrompts`, `SaddleInput`, `SaddleTempHorse`. The prompt texts are the game's labels `REMOVE_SADDLE` and `PLACE_SADDLE`.
- Tested in game: donkey, mule and painted mule; saddle from a horse to a donkey or mule and back; temp horse to main; save and load.
- Checked: pelts and carcasses can be stowed on the donkey and the mule, several at once, as on a horse.

### Changed

- Pat, Brush and Feed are not offered while the player carries something (a saddle, a pelt, a body), as on horses.

### Known limitations

- No saddle animation (on horses the engine plays a cutscene). A donkey or mule outside the player's horse slots cannot be saddled in the field, nor the temp one when there is no main horse; the stable works in both cases.

## v0.7.0 — 2026-11-27

On-foot care without locking on again.

### Changed

- The on-foot Pat, Brush and Feed prompts fire on a single press, like the game's. The mounted Brush is still a hold, so it does not fire by accident while riding.

### Fixed

- After each on-foot action the prompts stayed greyed until the animal was locked on again (right click). The game does not drop the lock-on, as was thought: it keeps the animal interaction "running" for as long as the lock-on is held, and the mod waited for it to end. Now the prompts come back as soon as the action is over (2.5 s, and for Brush and Feed when the game has finished using the item). Tested on the donkey, the mule and the painted mule.
- The item-use state of `player_horse` is read as a 32-bit value (leftover high bits were seen while brushing).

## v0.6.0 — 2026-11-26

SuperDonkMode: an optional stat boost for the donkey and the mules.

### Added

- `SuperDonkMode` (off by default): raises the base attribute ranks of the donkey or mule the player rides with the game's attribute natives (`SET_ATTRIBUTE_BASE_RANK`). Defaults are the game's own default ranks: speed 5 and acceleration 5 of the White Arabian, health 6 and stamina 4 of the Dark Bay Turkoman (the stable shows rank + 1: 6 / 6 and 7 / 5). Every value is set in the ini (`SuperDonkHealth`, `SuperDonkStamina`, `SuperDonkSpeed`, `SuperDonkAcceleration`; `-1` leaves a stat unchanged). Saddle bonuses still add on top.
- Tested: on mules all four stats apply (max health 60 → 180, max stamina 60 → 140). The TAB wheel and "Show Info" show the boosted stats; the stable and `Player → Horse` show the breed's original stats.

### Known limitations

- On the donkey only speed and acceleration apply. Its max health (251) and stamina are fixed by its model and ignore the ranks; tested in game, its stamina already lasts about like rank 4 (the Turkoman's) and its health is higher than any horse rank gives (rank 9 = 240).

## v0.5.0 — 2026-11-26

The painted mule joins the stable, the prompts use the game's own texts and the documentation is now in English and Spanish.

### Added

- The painted mule can be stabled too, together with the regular mule (both use the `HORSE_DONKEY` item). The multiplayer-only case at the start of each model -> item function is rewritten as a two-case switch (mule, painted mule) that jumps to the existing donkey case; every other horse falls through to the original switch, so horses without an item (gang horses and others) are still refused as in vanilla. Tested: stable row, renaming and mane / tail changes, main horse, whistle, map and persistence on load, inside and outside the stable.
- The care prompt texts come from the game's own text labels (`INTERACT_HORSE`, `BRUSH_HORSE`, `FEED_HORSE`), so they follow the game language. English fallback if a label is missing.

### Changed

- The mounted brush prompt shows the game's brush text (`BRUSH_HORSE`, "Cepillar" in Spanish) instead of the hardcoded "Cepillar burro" / "Cepillar mula".
- Documentation in English and Spanish: `README.md` / `README.es.md`, `CHANGELOG.md` / `CHANGELOG.es.md`, `docs/TECHNICAL-NOTES.md` / `docs/NOTAS-TECNICAS.md`, bilingual `StabledDonkey.ini` comments and `README.txt` / `LEEME.txt` in the release package.

### Removed

- `StableMuleModel`: both mules are now stabled.

### Fixed

- Feed did not appear when the only food left was herbs. It now uses the same food list as the game: horse food first, then the 26 herbs vanilla falls back to (ginseng, sage, mint...).
- One of the seven model -> item functions was not recognised (one case jumps over compiler padding), so it only knew the regular mule. Up to 2 odd cases are now tolerated when the donkey case is exact; all seven are rewritten.

### Documented

- Uninstall test: without the mod, a stable that holds a donkey or a mule shows an empty "Manage horses" menu (it works again once the mod is back). A mule kept as the main horse still spawns with its vanilla interactions, but gives no bonding points, shows a plain animal card and its bond drops to level 1. Take donkeys and mules out of the stable (or sell them) before uninstalling.

## v0.4.0 — 2026-11-26

The regular mule can now be stabled, set as the main horse and kept across save loads.

### Added

- Mule stable support (`StableMules=1`). The game has no catalog item for mules, so a stabled mule was saved without a menu row (a vanilla bug). The model -> item functions (`player_horse`, `shop_horse_shop_sp` and other scripts with the same code) start with a case for a multiplayer-only horse; that case is rewritten in memory to map the mule to the `HORSE_DONKEY` item. The mule then gets its stable row, can be set as the main horse, answers the whistle, shows on the map and is restored on load. Its breed still reads "Mula".
- `StableMuleModel` chooses which mule gets that single slot: `mule` (default) or `painted`.
- The mod's own on-foot care prompts (G pat, B brush, R feed) for mules (`CareMuleOnFoot=1`), because their vanilla prompts grey out unpredictably. On mules only the vanilla brush and feed are hidden (`HideVanillaPromptTypesMule=49,50`): hiding all of them made the engine drop the lock-on.
- `CarePromptPriority` (default 2, high) so the mod's prompts are not covered by the active vanilla ones.

### Changed

- Like vanilla horses, on-foot Pat is offered before the bond (it starts the bond), while Brush and Feed need bond level 1.

### Known limitations

- Only one mule model fits the stable mapping. The painted mule (one mission only) keeps the vanilla stable bug.
- In the stable the mule's title reads "Burro" (it uses the donkey item).
- After an on-foot action the animal must be locked on again (right click) to repeat; this now applies to mules too.
- "Remove saddle" still does not appear, even with the mule as the main horse. The TAB-wheel brush stays greyed out.

## v0.3.0 — 2026-11-25

Mules join the donkey: natural bonding and care for the mule and the painted mule.

### Added

- Mule and painted mule support, now on by default (`PatchMules=1`). The same in-memory model data patch, plus the `player_horse` mule check removed, so mules gain bonding XP from riding, feeding, brushing and patting, level up, show their info card and stats, and appear as the temporary horse.
- Once bonded, mules use the vanilla on-foot pat / brush / feed prompts.
- "Cepillar mula" (R) while riding a dirty mule, like "Cepillar burro".

### Changed

- The care prompts module is now `src/native/CarePrompts.cpp` (was `DonkeyCare.cpp`).
- The stable script's own mule check is left intact, so the stable treats mules exactly as vanilla does.

### Known limitations

- Do not store mules in the stable. Vanilla already lets a mule in, but there is no catalog item for it and the game saves it broken (no breed, blank menu, unselectable slot). This is a vanilla bug.
- The vanilla on-foot mule prompts are sometimes greyed out; the cause is not known yet.
- The TAB-wheel brush stays greyed out on donkey and mules (it works on horses).
- The info card coat field shows `~lp~` for donkey and mules (cosmetic).

## v0.2.0 — 2026-11-25

The donkey now works as a real horse breed, with every change applied in memory and no game files touched.

### Added

- In-memory model data patch: the donkey's ped model info gets the horse animal type and related fields. Offsets are discovered at runtime by comparing the donkey, mule and horse model infos.
- In-memory script patch: removes the vanilla "is this a donkey?" checks from the loaded `player_horse` and `shop_horse_shop_sp` bytecode. Breed and catalog data stay intact. The scripts are patched again after every save load and whenever the game reloads them.
- Stable support: the real donkey can be stabled, set as the main horse and restored with its name and appearance, also after restarting the game.
- Care prompts for the donkey. On foot, Pat (G), Brush (B) and Feed (R) appear in the lock-on menu. While riding, "Cepillar burro" (R) appears when the donkey is dirty. Brush and feed run the vanilla player_horse item-use sequence: cleaning, item use, cores and bonding.
- Optional `StabledDonkey.ini` with documented settings.
- `docs/NOTAS-TECNICAS.md` with the technical findings.

### Removed

- The v0.1 Tennessee Walker proxy (`GET_ENTITY_MODEL` hook) and the experimental stable bridge (`StabledDonkey.bridge`, F7 entity swap). They are no longer needed and caused the Tennessee Walker name and stats to show up.

### Known limitations

- The engine keeps the vanilla care prompts and the TAB-wheel brush greyed out on the donkey. The mod's own prompts replace them, but after each on-foot action the donkey must be locked on again.
- "Remove saddle" is not available on the donkey.
- Mules are not supported yet (`PatchMules=0`).
- Only verified with RDR2 Steam 1.0.1491.50.

## v0.1.0 — 2026-10-09

Initial public release focused on natural horse-bonding rewards for supported donkey and mule models.

- Let vanilla horse-care reward logic recognize the supported animal models during the active mount interaction window.
- Preserve the game's own bonding point updates, XP toast, and level-up notification; the mod does not add bonding points or draw a replacement toast.
- Track supported mount model, bonding points, and bonding level in `StabledDonkey.log`.
- Confirm donkey bonding through mounted feeding, brushing with the brush item, and riding in Story Mode.
- Confirm the donkey can appear as the temporary horse and can use the vanilla horse information card after bonding.

### Known limitations

- Donkey on-foot context actions (feed, brush, pet) remain disabled or missing.
- Stable persistence and Stable Expansion compatibility are not implemented.
- Some UI surfaces may identify the donkey as a Tennessee Walker.
- The current game-build-specific native hook has only been verified with RDR2 Steam 1.0.1491.50.
