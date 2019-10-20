#pragma once

#include <cstdint>

namespace PathFinder 
{

    struct Material
    {
        uint32_t AlbedoMapIndex = 0;
        uint32_t NormalMapIndex = 0;
        uint32_t RoughnessMapIndex = 0;
        uint32_t MetalnessMapIndex = 0;
        uint32_t AOMapIndex = 0;
    };

}
