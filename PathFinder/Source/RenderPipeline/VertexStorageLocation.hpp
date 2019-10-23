#pragma once

#include <cstdint>

namespace PathFinder
{

    struct VertexStorageLocation
    {
        uint32_t VertexBufferOffset = 0;
        uint32_t VertexCount = 0;
        uint32_t IndexBufferOffset = 0;
        uint32_t IndexCount = 0;
        uint16_t BottomAccelerationStructureIndex = 0;
    };

}
