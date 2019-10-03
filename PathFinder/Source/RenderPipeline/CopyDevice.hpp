#pragma once

#include "../HardwareAbstractionLayer/CommandQueue.hpp"

#include <memory>

namespace PathFinder
{

    class CopyDevice
    {
    public:
        CopyDevice(const HAL::Device* device);

        template <class T> void QueueBufferToTextureCopy(std::shared_ptr<HAL::BufferResource<T>> buffer, const HAL::TextureResource& texture, const ResourceFootprint& footprint);
        template <class T> std::unique_ptr<HAL::BufferResource<T>> QueueResourceCopyToDefaultHeap(std::shared_ptr<HAL::BufferResource<T>> buffer);
        std::unique_ptr<HAL::TextureResource> QueueResourceCopyToDefaultHeap(std::shared_ptr<HAL::TextureResource> texture);
        
        void CopyResources();

    private:
        bool IsCopyableState(HAL::ResourceState state);

        const HAL::Device* mDevice;
        HAL::CopyCommandAllocator mCommandAllocator;
        HAL::CopyCommandList mCommandList;
        HAL::CopyCommandQueue mCommandQueue;
        HAL::Fence mFence;

        std::vector<std::shared_ptr<HAL::Resource>> mResourcesToCopy;
    };

}

#include "CopyDevice.inl"