#pragma once

#include "types.h"

namespace StabledDonkey::SaddlePrompts
{
    // Call every script tick. "Remove saddle" and "place saddle" prompts for a
    // donkey / mule that is the player's main horse.
    void Update(Ped trackedMount);
}
