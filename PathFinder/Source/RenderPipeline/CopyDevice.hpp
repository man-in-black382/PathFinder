#pragma once

#include "../HardwareAbstractionLayer/CommandQueue.hpp"
#include "../HardwareAbstractionLayer/ResourceFootprint.hpp"

#include <memory>

namespace PathFinder
{

    class CopyDevice
    {
    public:
        CopyDevice(const HAL::Device* device);

        template <class T> void QueueBufferToTextureCopy(std::shared_ptr<HAL::BufferResource<T>> buffer, std::shared_ptr<HAL::TextureResource> texture, const HAL::ResourceFootprint& footprint);
        
        template <class T> std::shared_ptr<HAL::BufferResource<T>> QueueResourceCopyToDefaultMemory(std::shared_ptr<HAL::BufferResource<T>> buffer);
        std::shared_ptr<HAL::TextureResource> QueueResourceCopyToDefaultMemory(std::shared_ptr<HAL::TextureResource> texture);

        template <class T> std::shared_ptr<HAL::BufferResource<T>> QueueResourceCopyToReadbackMemory(std::shared_ptr<HAL::BufferResource<T>> buffer);
        std::shared_ptr<HAL::BufferResource<uint8_t>> QueueResourceCopyToReadbackMemory(std::shared_ptr<HAL::TextureResource> texture);
        
        void CopyResources();
        void WaitFence(HAL::Fence& fence);
        void SignalFence(HAL::Fence& fence);

    private:
        const HAL::Device* mDevice;
        HAL::CopyCommandAllocator mCommandAllocator;
        HAL::CopyCommandList mCommandList;
        HAL::CopyCommandQueue mCommandQueue;
        HAL::Fence mFence;

        std::vector<std::shared_ptr<HAL::Resource>> mResourcesToCopy;
    };

}

#include "CopyDevice.inl"