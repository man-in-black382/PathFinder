#pragma once

#include "../Foundation/Name.hpp"

namespace PathFinder
{

    struct ResourceKey
    {
    public:
        ResourceKey(Foundation::Name resourceName)
            : mResourceName{ resourceName } {}

        ResourceKey(Foundation::Name resourceName, uint64_t indexInArray)
            : mResourceName{ resourceName }, mIndexInArray{ indexInArray } {}

    private:
        Foundation::Name mResourceName;
        uint64_t mIndexInArray = 0;

    public:
        inline auto ResourceName() const { return mResourceName; }
        inline auto IndexInArray() const { return mIndexInArray; }
    };

}
