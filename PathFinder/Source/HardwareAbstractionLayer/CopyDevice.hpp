#pragma once

#include "CommandQueue.hpp"

#include <memory>

namespace HAL
{

    class CopyDevice
    {
    public:
        CopyDevice(const Device* device);

        template <class T> std::unique_ptr<BufferResource<T>> QueueResourceCopyToDefaultHeap(std::shared_ptr<BufferResource<T>> buffer);
        std::unique_ptr<TextureResource> QueueResourceCopyToDefaultHeap(std::shared_ptr<TextureResource> texture);

        void CopyResources();

    private:
        const Device* mDevice;
        CopyCommandAllocator mCommandAllocator;
        CopyCommandList mCommandList;
        CopyCommandQueue mCommandQueue;
        Fence mFence;

        std::vector<std::shared_ptr<Resource>> mResourcesToCopy;
    };

}

#include "CopyDevice.inl"