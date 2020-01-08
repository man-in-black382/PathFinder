#pragma once

#include "../HardwareAbstractionLayer/CommandQueue.hpp"
#include "../HardwareAbstractionLayer/RingCommandList.hpp"
#include "../HardwareAbstractionLayer/ResourceFootprint.hpp"

#include <memory>

namespace PathFinder
{

    class CopyDevice
    {
    public:
        CopyDevice(const HAL::Device* device, uint8_t simultaneousFramesInFlight);

        // No ownership transfer

        template <class T> void QueueBufferToBufferCopy(
            const HAL::BufferResource<T>& source, 
            const HAL::BufferResource<T>& destination,
            uint64_t sourceOffset, uint64_t objectCount, uint64_t destinationOffset);

        template <class T> void QueueBufferToTextureCopy(
            const HAL::BufferResource<T>& buffer,
            const HAL::TextureResource& texture,
            const HAL::ResourceFootprint& footprint);
        
        // For cases when a resource may not be needed after copy, ownership is shared until copy is completed

        template <class T> void QueueBufferToBufferCopy(
            std::shared_ptr<HAL::BufferResource<T>> source,
            std::shared_ptr<HAL::BufferResource<T>> destination,
            uint64_t sourceOffset, uint64_t objectCount, uint64_t destinationOffset);

        template <class T> void QueueBufferToTextureCopy(
            std::shared_ptr<HAL::BufferResource<T>> buffer,
            std::shared_ptr<HAL::TextureResource> texture,
            const HAL::ResourceFootprint& footprint);

        template <class T> std::shared_ptr<HAL::BufferResource<T>> QueueResourceCopyToDefaultMemory(std::shared_ptr<HAL::BufferResource<T>> buffer);
        std::shared_ptr<HAL::TextureResource> QueueResourceCopyToDefaultMemory(std::shared_ptr<HAL::TextureResource> texture);

        template <class T> std::shared_ptr<HAL::BufferResource<T>> QueueResourceCopyToReadbackMemory(std::shared_ptr<HAL::BufferResource<T>> buffer);
        std::shared_ptr<HAL::BufferResource<uint8_t>> QueueResourceCopyToReadbackMemory(std::shared_ptr<HAL::TextureResource> texture);
        
        void ExecuteCommands(const HAL::Fence* fenceToWaitFor = nullptr, const HAL::Fence* fenceToSignal = nullptr);
        void ResetCommandList();

        void BeginFrame(uint64_t newFrameNumber);
        void EndFrame(uint64_t completedFrameNumber);

    private:
        const HAL::Device* mDevice;
        HAL::CopyCommandQueue mCommandQueue;
        HAL::CopyRingCommandList mRingCommandList;
        uint8_t mSimultaneousFramesInFlight = 1;
        uint64_t mLastFenceValue = 0;
        uint64_t mCurrentFrameIndex = 0;

        std::vector<std::vector<std::shared_ptr<HAL::Resource>>> mResourcesToCopy;
    };

}

#include "CopyDevice.inl"