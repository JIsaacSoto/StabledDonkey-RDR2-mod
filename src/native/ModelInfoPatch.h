#pragma once

#include "types.h"

#include <cstddef>

namespace StabledDonkey::NativeHorse
{
    // Call every script tick. Resolves the game's model-info lookup, discovers
    // the field offsets by comparing donkey / mule / horse model infos, writes
    // the horse values once and re-applies them if the game restores them.
    void Update();

    // Short "animal_type=..." text for a model, for the mount log.
    const char *Describe(Hash model, char *buffer, size_t size);
}
