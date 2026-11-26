#pragma once

#include <cstddef>
#include <cstdint>

namespace StabledDonkey::Memory
{
    struct PatternResult
    {
        size_t matches = 0;
        uintptr_t firstAddress = 0;
    };

    // Scans the executable sections of the game image. Pattern bytes are hex
    // separated by spaces; "?" is a wildcard byte.
    PatternResult FindPattern(const char *pattern);

    // Number of bytes readable from address, following committed regions,
    // capped at maxBytes.
    size_t ReadableSpan(uintptr_t address, size_t maxBytes);

    bool IsReadable(uintptr_t address, size_t length);

    // Writes value at address, temporarily lifting page protection if needed.
    bool WriteBytes(uintptr_t address, const void *value, size_t length);

    uintptr_t ImageBase();
}
