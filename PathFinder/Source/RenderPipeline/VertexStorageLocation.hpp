#pragma once

#include <cstdint>

namespace PathFinder
{

    struct VertexStorageLocation
    {
        uint64_t VertexBufferOffset = 0;
        uint64_t VertexCount = 0;
        uint64_t IndexBufferOffset = 0;
        uint64_t IndexCount = 0;
    };

}
