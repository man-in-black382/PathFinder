#include "ResourceScheduler.hpp"

namespace PathFinder
{

    MipSet MipSet::Empty()
    {
        return MipSet{};
    }

    MipSet MipSet::Explicit(const MipList& mips)
    {
        MipSet set{};
        set.Combination = mips;
        return set;
    }

    MipSet MipSet::IndexFromStart(uint32_t index)
    {
        MipSet set{};
        set.Combination = MipSet::MipVariant{};
        set.Combination->emplace<2>(index);
        return set;
    }

    MipSet MipSet::IndexFromEnd(uint32_t index)
    {
        MipSet set{};
        set.Combination = MipSet::MipVariant{};
        set.Combination->emplace<3>(index);
        return set;
    }

    MipSet MipSet::FirstMip()
    {
        return IndexFromStart(0);
    }

    MipSet MipSet::LastMip()
    {
        return IndexFromEnd(0);
    }

    MipSet MipSet::AllMips()
    {
        return MipSet::Range(0, std::nullopt);
    }

    MipSet MipSet::Range(uint32_t firstMip, std::optional<uint32_t> lastMip)
    {
        MipSet set{};
        set.Combination = MipRange{ firstMip, lastMip };
        return set;
    }

}
