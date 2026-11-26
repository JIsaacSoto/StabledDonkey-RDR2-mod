# Changelog

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
