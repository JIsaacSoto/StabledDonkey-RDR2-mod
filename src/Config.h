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
        // Same for the mule and the painted mule (not finished yet).
        bool patchMules = false;

        // The engine greys out the vanilla pat / brush / feed prompts on the
        // donkey, so the mod adds its own (lock-on menu on foot, brush while
        // riding). Brush and feed run the vanilla player_horse item use.
        bool carePrompts = true;
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

        void Load(HMODULE module);
        void LogValues() const;
    };

    extern Config g_config;
}
