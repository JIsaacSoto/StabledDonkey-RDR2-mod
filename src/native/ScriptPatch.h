#pragma once

namespace StabledDonkey::ScriptPatch
{
    // Call every script tick (ScriptHook thread). Schedules background scans
    // of the game's script bytecode and neutralizes the vanilla "is this a
    // donkey/mule?" checks once a scan completes.
    void Update();

    // Call at the start of every ScriptMain run (ScriptHook restarts it when
    // a save is loaded) so the freshly loaded game scripts are patched again.
    void Reset();
}
