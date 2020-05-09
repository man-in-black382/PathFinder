#pragma once

#include <cstdint>

namespace PathFinder
{

    struct DownsamplingCBContent
    {
        uint32_t SourceTextureIndex;
        uint32_t Destination0TextureIndex;
        uint32_t Destination1TextureIndex;
        uint32_t Destination2TextureIndex;
        uint32_t Destination3TextureIndex;
    };

}
