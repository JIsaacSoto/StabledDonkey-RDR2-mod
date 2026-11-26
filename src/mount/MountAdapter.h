#pragma once

#include "types.h"

namespace StabledDonkey
{
    // Model hashes of the animals the mod supports.
    namespace Models
    {
        constexpr Hash DONKEY = 0x69A37A7B;       // A_C_Donkey_01
        constexpr Hash MULE = 0xB6A7CE35;         // A_C_HorseMule_01
        constexpr Hash PAINTED_MULE = 0x23685521; // A_C_HorseMulePainted_01
    }

    enum class MountType
    {
        None,
        Donkey,
        Mule,
        PaintedMule
    };

    // Thin wrapper over a ped handle for the supported animals.
    class MountAdapter
    {
    public:
        explicit MountAdapter(Ped ped);

        bool IsValid() const;
        bool IsSupported() const;

        Ped GetPed() const;
        Hash GetModel() const;
        MountType GetType() const;

        bool IsDonkey() const;
        bool IsMule() const;

        int GetBondingPoints() const;
        int GetBondingLevel() const;

    private:
        Ped m_ped;
    };
}
