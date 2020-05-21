#pragma once

#include <cstdint>

namespace PathFinder
{

    struct DownsamplingCBContent
    {
        enum class Filter : uint32_t
        { 
            Average = 0, Min = 1, Max = 2 
        };

        Filter FilterType;
        uint32_t SourceTexIdx;
        uint32_t Destination0TexIdx;
        uint32_t Destination1TexIdx;
        uint32_t Destination2TexIdx;
        uint32_t Destination3TexIdx;
    };

}
