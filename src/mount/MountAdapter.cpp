#include "MountAdapter.h"

#include "natives.h"

#include <atomic>

namespace StabledDonkey
{
    namespace
    {
        thread_local bool g_mountAdapterModelQuery = false;
    }

    bool IsMountAdapterModelQueryActive()
    {
        return g_mountAdapterModelQuery;
    }

    namespace
    {
        constexpr Hash DONKEY_MODEL = 0x69A37A7B;
        constexpr Hash MULE_MODEL = 0xB6A7CE35;
        constexpr Hash PAINTED_MULE_MODEL = 0x23685521;
        constexpr int BONDING_ATTRIBUTE = 7;
    }

    MountAdapter::MountAdapter(Ped ped)
        : m_ped(ped)
    {
    }

    bool MountAdapter::IsValid() const
    {
        return m_ped != 0;
    }

    bool MountAdapter::IsSupported() const
    {
        return GetType() != MountType::None;
    }

    Ped MountAdapter::GetPed() const
    {
        return m_ped;
    }

    Hash MountAdapter::GetModel() const
    {
        if (!IsValid())
            return 0;

        const bool previous = g_mountAdapterModelQuery;
        g_mountAdapterModelQuery = true;
        const Hash model = ENTITY::GET_ENTITY_MODEL(m_ped);
        g_mountAdapterModelQuery = previous;
        return model;
    }

    MountType MountAdapter::GetType() const
    {
        switch (GetModel())
        {
        case DONKEY_MODEL:
            return MountType::Donkey;
        case MULE_MODEL:
            return MountType::Mule;
        case PAINTED_MULE_MODEL:
            return MountType::PaintedMule;
        default:
            return MountType::None;
        }
    }

    bool MountAdapter::IsDonkey() const
    {
        return GetType() == MountType::Donkey;
    }

    bool MountAdapter::IsMule() const
    {
        return GetType() == MountType::Mule;
    }

    bool MountAdapter::IsPaintedMule() const
    {
        return GetType() == MountType::PaintedMule;
    }

    int MountAdapter::GetBondingPoints() const
    {
        if (!IsValid())
            return 0;

        return static_cast<int>(ATTRIBUTE::_0x219DA04BAA9CB065(m_ped, BONDING_ATTRIBUTE));
    }

    int MountAdapter::GetBondingLevel() const
    {
        if (!IsValid())
            return 0;

        return static_cast<int>(ATTRIBUTE::_0xA4C8E23E29040DE0(m_ped, BONDING_ATTRIBUTE));
    }

}
