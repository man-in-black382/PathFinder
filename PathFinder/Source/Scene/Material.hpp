#pragma once

#include <cstdint>

namespace PathFinder 
{

    struct Material
    {
        uint32_t AlbedoMapSRVIndex = 0;
        uint32_t NormalMapSRVIndex = 0;
        uint32_t RoughnessMapSRVIndex = 0;
        uint32_t MetalnessMapSRVIndex = 0;
        uint32_t AOMapSRVIndex = 0;
        uint32_t DisplacementMapSRVIndex = 0;
        uint32_t DistanceMapSRVIndex = 0;
        uint32_t DistanceMapUAVIndex = 0;
    };

}
