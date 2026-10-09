# Changelog

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
