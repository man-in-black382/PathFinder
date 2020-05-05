#pragma once

#include "../Foundation/Name.hpp"
#include "../Memory/GPUResourceProducer.hpp"

#include <unordered_set>

namespace PathFinder
{

    struct PipelineResourceStoragePass
    {
        // Constant buffers for each pass that require it.
        Memory::GPUResourceProducer::BufferPtr PassConstantBuffer;

        // Memory offset for pass constant buffer in current frame.
        // Used to place pass data in different memory locations
        // as a versioning mechanism for multiple draws/dispatches in one render pass.
        uint64_t PassConstantBufferMemoryOffset = 0;

        // Size of data last uploaded to pass constant buffer. Used to offset 
        // the constant buffer after a draw/dispatch.
        uint64_t LastSetConstantBufferDataSize = 0;

        // 
        bool IsAllowedToAdvanceConstantBufferOffset = false;

        // Debug buffer for each pass.
        Memory::GPUResourceProducer::BufferPtr PassDebugBuffer;

        // Resource names scheduled for each pass
        std::unordered_set<Foundation::Name> ScheduledResourceNames;

        // Resource aliasing barriers for the pass
        HAL::ResourceBarrierCollection AliasingBarriers;

        // UAV barriers to be applied after each draw/dispatch in
        // a pass that makes unordered accesses to resources
        HAL::ResourceBarrierCollection UAVBarriers;
    };

}
