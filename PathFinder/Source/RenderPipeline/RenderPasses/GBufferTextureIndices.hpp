#pragma once

#include <cstdint>

namespace PathFinder
{

    struct GBufferTextureIndices
    {
        uint32_t AlbedoMetalnessTexIdx = 0;
        uint32_t NormalRoughnessTexIdx = 0;
        uint32_t MotionTexIdx = 0;
        uint32_t TypeAndMaterialTexIdx = 0;
        // 16 byte boundary
        uint32_t ViewDepthTexIdx = 0;
        uint32_t DepthStencilTexIdx = 0;
        uint32_t __Pad0 = 0;
        uint32_t __Pad1 = 0;
        // 16 byte boundary
    };

}
