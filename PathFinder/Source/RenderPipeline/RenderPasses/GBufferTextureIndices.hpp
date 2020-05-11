#pragma once

#include <cstdint>

namespace PathFinder
{

    struct GBufferTextureIndices
    {
        uint32_t AlbedoMetalnessTextureIndex = 0;
        uint32_t RoughnessTextureIndex = 0;
        uint32_t NormalTextureIndex = 0;
        uint32_t MotionTextureIndex = 0;
        // 16 byte boundary
        uint32_t TypeAndMaterialTextureIndex = 0;
        uint32_t ViewDepthTextureIndex = 0;
        uint32_t DepthStencilTextureIndex = 0;
        uint32_t __Pad0 = 0;
        // 16 byte boundary
    };

}
