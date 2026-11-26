#pragma once

#include "types.h"

namespace StabledDonkey::DonkeyCare
{
    // Call every script tick. trackedDonkey is the last supported mount the
    // player rode (may be 0); the donkey the player is locked on to wins.
    void Update(Ped trackedDonkey);
}
