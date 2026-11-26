#include "Memory.h"

#include <Windows.h>

#include <algorithm>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

namespace StabledDonkey::Memory
{
    namespace
    {
        struct PatternByte
        {
            uint8_t value;
            bool wildcard;
        };

        std::vector<PatternByte> Parse(const char *text)
        {
            std::vector<PatternByte> result;
            std::istringstream input(text);
            std::string token;
            while (input >> token)
            {
                if (token == "?" || token == "??")
                {
                    result.push_back({0, true});
                    continue;
                }
                unsigned long value = std::strtoul(token.c_str(), nullptr, 16);
                if (value > 0xFF)
                    return {};
                result.push_back({static_cast<uint8_t>(value), false});
            }
            return result;
        }

        bool IsReadableProtection(DWORD protect)
        {
            if ((protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0)
                return false;
            const DWORD base = protect & 0xFF;
            return base == PAGE_READONLY || base == PAGE_READWRITE || base == PAGE_WRITECOPY ||
                   base == PAGE_EXECUTE_READ || base == PAGE_EXECUTE_READWRITE ||
                   base == PAGE_EXECUTE_WRITECOPY;
        }
    }

    uintptr_t ImageBase()
    {
        return reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    }

    PatternResult FindPattern(const char *patternText)
    {
        PatternResult result{};
        const std::vector<PatternByte> pattern = Parse(patternText);
        const auto *base = reinterpret_cast<const uint8_t *>(ImageBase());
        if (pattern.empty() || !base)
            return result;

        const auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE)
            return result;
        const auto *nt = reinterpret_cast<const IMAGE_NT_HEADERS64 *>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE)
            return result;

        const auto *section = IMAGE_FIRST_SECTION(nt);
        for (WORD s = 0; s < nt->FileHeader.NumberOfSections; ++s, ++section)
        {
            if ((section->Characteristics & IMAGE_SCN_MEM_EXECUTE) == 0)
                continue;
            const uint8_t *start = base + section->VirtualAddress;
            const size_t size = ReadableSpan(reinterpret_cast<uintptr_t>(start),
                                             section->Misc.VirtualSize);
            if (size < pattern.size())
                continue;

            for (size_t offset = 0; offset <= size - pattern.size(); ++offset)
            {
                if (start[offset] != pattern[0].value)
                    continue;
                bool equal = true;
                for (size_t i = 1; i < pattern.size(); ++i)
                {
                    if (!pattern[i].wildcard && start[offset + i] != pattern[i].value)
                    {
                        equal = false;
                        break;
                    }
                }
                if (!equal)
                    continue;
                if (result.matches == 0)
                    result.firstAddress = reinterpret_cast<uintptr_t>(start + offset);
                ++result.matches;
            }
        }
        return result;
    }

    size_t ReadableSpan(uintptr_t address, size_t maxBytes)
    {
        size_t total = 0;
        uintptr_t cursor = address;
        while (total < maxBytes)
        {
            MEMORY_BASIC_INFORMATION info{};
            if (VirtualQuery(reinterpret_cast<const void *>(cursor), &info, sizeof(info)) == 0 ||
                info.State != MEM_COMMIT || !IsReadableProtection(info.Protect))
                break;
            const uintptr_t regionEnd = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
            if (regionEnd <= cursor)
                break;
            const size_t available = static_cast<size_t>(regionEnd - cursor);
            const size_t take = std::min(available, maxBytes - total);
            total += take;
            cursor += take;
        }
        return total;
    }

    bool IsReadable(uintptr_t address, size_t length)
    {
        return address != 0 && ReadableSpan(address, length) == length;
    }

    bool WriteBytes(uintptr_t address, const void *value, size_t length)
    {
        if (!IsReadable(address, length))
            return false;
        auto *target = reinterpret_cast<void *>(address);
        DWORD oldProtection = 0;
        if (!VirtualProtect(target, length, PAGE_READWRITE, &oldProtection))
            return false;
        std::memcpy(target, value, length);
        DWORD ignored = 0;
        VirtualProtect(target, length, oldProtection, &ignored);
        return true;
    }
}
