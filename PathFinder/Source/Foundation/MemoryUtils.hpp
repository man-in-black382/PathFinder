#pragma once

namespace Foundation
{
    namespace MemoryUtils
    {
        inline uint64_t Align(uint64_t memorySize, uint64_t alignment)
        {
            return (memorySize + alignment - 1) & ~(alignment - 1);
        }
    }
}
