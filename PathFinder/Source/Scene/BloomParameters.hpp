#pragma once

#include <cstdint>

namespace PathFinder 
{

    struct BloomParameters
    {
        uint32_t SmallBlurRadius = 6;
        uint32_t SmallBlurSigma = 1.0;
        uint32_t MediumBlurRadius = 20;
        uint32_t MediumBlurSigma = 2.0;
        uint32_t LargeBlurRadius = 60;
        uint32_t LargeBlurSigma = 15.0;

        uint32_t SmallBloomWeight = 2;
        uint32_t MediumBloomWeight = 1;
        uint32_t LargeBloomWeight = 1;
    };

}
