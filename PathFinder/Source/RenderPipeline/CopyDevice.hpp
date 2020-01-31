#pragma once

#include "../HardwareAbstractionLayer/CommandQueue.hpp"
#include "../HardwareAbstractionLayer/ResourceFootprint.hpp"

#include "../Memory/PoolCommandListAllocator.hpp"
#include "../Memory/SegregatedPoolsResourceAllocator.hpp"

#include <memory>

namespace PathFinder
{

    //class CopyDevice
    //{
    //public:
    //    CopyDevice(const HAL::Device& device, Memory::PoolCommandListAllocator* commandListAllocator, Memory::SegregatedPoolsResourceAllocator* resourceAllocator);

    //    // No ownership transfer

    //    void QueueBufferToBufferCopy(
    //        const HAL::Buffer& source, 
    //        const HAL::Buffer& destination,
    //        uint64_t sourceOffset, uint64_t objectCount, uint64_t destinationOffset);

    //    void QueueBufferToTextureCopy(
    //        const HAL::Buffer& buffer,
    //        const HAL::Texture& texture,
    //        const HAL::ResourceFootprint& footprint);
    //    
    //    // For cases when a resource may not be needed after copy, ownership is shared until copy is completed

    //    void QueueBufferToBufferCopy(
    //        std::shared_ptr<HAL::Buffer> source,
    //        std::shared_ptr<HAL::Buffer> destination,
    //        uint64_t sourceOffset, uint64_t objectCount, uint64_t destinationOffset);

    //    void QueueBufferToTextureCopy(
    //        std::shared_ptr<HAL::Buffer> buffer,
    //        std::shared_ptr<HAL::Texture> texture,
    //        const HAL::ResourceFootprint& footprint);

    //    std::shared_ptr<HAL::Buffer> QueueResourceCopyToDefaultMemory(std::shared_ptr<HAL::Buffer> buffer);
    //    std::shared_ptr<HAL::Texture> QueueResourceCopyToDefaultMemory(std::shared_ptr<HAL::Texture> texture);

    //    std::shared_ptr<HAL::Buffer> QueueResourceCopyToReadbackMemory(std::shared_ptr<HAL::Buffer> buffer);
    //    std::shared_ptr<HAL::Buffer> QueueResourceCopyToReadbackMemory(std::shared_ptr<HAL::Texture> texture);
    //    
    //    void ExecuteCommands(const HAL::Fence* fenceToWaitFor = nullptr, const HAL::Fence* fenceToSignal = nullptr);

    //private:
    //    Memory::PoolCommandListAllocator* mCommandListAllocator;
    //    Memory::SegregatedPoolsResourceAllocator* mResourceAllocator;

    //    HAL::CopyCommandQueue mCommandQueue;
    //    std::unique_ptr<HAL::CopyCommandList> mCommandList;
    //    uint64_t mLastFenceValue = 0;
    //    uint64_t mCurrentFrameIndex = 0;

    //    std::vector<std::vector<std::shared_ptr<HAL::Resource>>> mResourcesToCopy;
    //};

}

#include "CopyDevice.inl"