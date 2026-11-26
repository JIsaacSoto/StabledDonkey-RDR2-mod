#pragma once

#include <Windows.h>

#include <string>

namespace StabledDonkey
{
    // Options read from StabledDonkey.ini next to the .asi. Every key is
    // optional; a missing file or key keeps the default below.
    struct Config
    {
        // Donkey treated as a horse: model data in memory + vanilla script
        // checks (bonding, care, stats, stable).
        bool patchDonkey = true;
        // Same for the mule and the painted mule: bonding, care, stats.
        bool patchMules = true;
        // Let the stable store a mule by mapping it to the HORSE_DONKEY
        // catalog item (rewrites the multiplayer-only mangy horse case of the
        // model -> item functions; no HORSE_MULE item exists). Only one mule
        // model fits: "mule" (A_C_HorseMule_01) or "painted"
        // (A_C_HorseMulePainted_01, only seen in one mission).
        bool stableMules = true;
        std::string stableMuleModel = "mule";

        // The engine greys out the vanilla pat / brush / feed prompts on the
        // donkey, so the mod adds its own (lock-on menu on foot) and a brush
        // prompt while riding a donkey or a mule. Brush and feed run the
        // vanilla player_horse item use.
        bool carePrompts = true;
        // Also use the mod's on-foot prompts on bonded mules (their vanilla
        // ones grey out unpredictably); the vanilla ones are hidden.
        bool careMuleOnFoot = true;
        // Control names (femga/rdr3_discoveries Controls). Defaults are the
        // vanilla horse keys: G pat, B brush, R feed.
        std::string careInputPat = "INPUT_INTERACT_LOCKON_ANIMAL";
        std::string careInputBrush = "INPUT_INTERACT_HORSE_BRUSH";
        std::string careInputFeed = "INPUT_INTERACT_HORSE_FEED";
        // Mounted brush (R). INPUT_INTERACT_* controls do not exist on a mount.
        std::string careInputMountedBrush = "INPUT_CONTEXT_X";
        // On-foot prompts only this close, like vanilla (metres).
        float careMaxDistance = 2.2f;
        // Mounted brush only from this dirt level (0-100, attribute 16).
        int careMountedBrushMinDirt = 25;
        // Vanilla ePromptType values hidden on the donkey (PP_MODE_HIDE) so
        // the greyed vanilla prompts do not take the keys of ours.
        std::string hideVanillaPromptTypes = "5,44,47,49,50,52";
        // Same for the mules: only brush and feed, so their lock-on holds.
        std::string hideVanillaPromptTypesMule = "49,50";
        // Priority of the mod's prompts (ePromptPriority: 0 low, 1 normal,
        // 2 high, 3 mission critical).
        int carePromptPriority = 2;

        void Load(HMODULE module);
        void LogValues() const;
    };

    extern Config g_config;
}
