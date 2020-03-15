#pragma once

#include <cstdint>

namespace PathFinder 
{

    struct BloomParameters
    {
        uint32_t SmallBlurRadius = 12;
        uint32_t SmallBlurSigma = 4;
        uint32_t MediumBlurRadius = 28;
        uint32_t MediumBlurSigma = 8;
        uint32_t LargeBlurRadius = 56;
        uint32_t LargeBlurSigma = 16;

        uint32_t SmallBloomWeight = 2;
        uint32_t MediumBloomWeight = 1;
        uint32_t LargeBloomWeight = 1;
    };

}
