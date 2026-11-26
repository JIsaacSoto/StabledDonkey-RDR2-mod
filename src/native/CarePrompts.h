#pragma once

#include "types.h"

namespace StabledDonkey::CarePrompts
{
    // Call every script tick. trackedDonkey is the last supported mount (donkey or mule) the
    // player rode (may be 0); the donkey the player is locked on to wins.
    void Update(Ped trackedDonkey);
}
