#pragma once

#include <cstdint>

namespace PathFinder 
{

    struct Material
    {
        uint16_t AlbedoMapIndex = 0;
        uint16_t NormalMapIndex = 0;
        uint16_t RoughnessMapIndex = 0;
        uint16_t MetalnessMapIndex = 0;
        uint16_t AOMapIndex = 0;
    };

}
