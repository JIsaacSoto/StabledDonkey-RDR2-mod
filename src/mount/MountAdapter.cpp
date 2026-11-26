#include "MountAdapter.h"

#include "natives.h"

namespace StabledDonkey
{
    namespace
    {
        constexpr int BONDING_ATTRIBUTE = 7; // PA_BONDING
    }

    MountAdapter::MountAdapter(Ped ped)
        : m_ped(ped)
    {
    }

    bool MountAdapter::IsValid() const
    {
        return m_ped != 0 && ENTITY::DOES_ENTITY_EXIST(m_ped);
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
        return IsValid() ? ENTITY::GET_ENTITY_MODEL(m_ped) : 0;
    }

    MountType MountAdapter::GetType() const
    {
        switch (GetModel())
        {
        case Models::DONKEY:
            return MountType::Donkey;
        case Models::MULE:
            return MountType::Mule;
        case Models::PAINTED_MULE:
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
        const MountType type = GetType();
        return type == MountType::Mule || type == MountType::PaintedMule;
    }

    int MountAdapter::GetBondingPoints() const
    {
        // ATTRIBUTE::GET_ATTRIBUTE_POINTS
        return IsValid() ? static_cast<int>(ATTRIBUTE::_0x219DA04BAA9CB065(m_ped, BONDING_ATTRIBUTE)) : 0;
    }

    int MountAdapter::GetBondingLevel() const
    {
        // ATTRIBUTE::GET_ATTRIBUTE_RANK
        return IsValid() ? static_cast<int>(ATTRIBUTE::_0xA4C8E23E29040DE0(m_ped, BONDING_ATTRIBUTE)) : 0;
    }
}
