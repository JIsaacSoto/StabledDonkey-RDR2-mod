#pragma once

#include "types.h"

namespace StabledDonkey::SuperDonk
{
    // Optional stat boost (SuperDonkMode). Call every script tick with the
    // last supported mount the player rode (may be 0).
    void Update(Ped trackedMount);
}
