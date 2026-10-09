#pragma once

#include "types.h"

namespace StabledDonkey
{
    bool IsMountAdapterModelQueryActive();

    enum class MountType
    {
        None,
        Donkey,
        Mule,
        PaintedMule
    };

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
        bool IsPaintedMule() const;

        int GetBondingPoints() const;
        int GetBondingLevel() const;

    private:
        Ped m_ped;
    };
}
