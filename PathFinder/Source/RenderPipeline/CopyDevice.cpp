#include "CopyDevice.hpp"

namespace PathFinder
{

    CopyDevice::CopyDevice(const HAL::Device* device)
        : mDevice{ device },
        mCommandAllocator{ *mDevice },
        mCommandList{ *mDevice, mCommandAllocator },
        mCommandQueue{ *device },
        mFence{ *mDevice } 
    {
        mCommandQueue.SetDebugName("Copy_Device_Cmd_Queue");
    }
 
    std::shared_ptr<HAL::TextureResource> CopyDevice::QueueResourceCopyToDefaultMemory(std::shared_ptr<HAL::TextureResource> texture)
    {
        auto emptyClone = std::make_shared<HAL::TextureResource>(
            *mDevice, texture->Format(), texture->Kind(), texture->Dimensions(),
            texture->OptimizedClearValue(), HAL::ResourceState::CopyDestination, texture->ExpectedStates()
        );

        mCommandList.CopyResource(*texture, *emptyClone);

        // Hold in RAM until actual copy is performed
        mResourcesToCopy.push_back(texture);
        mResourcesToCopy.push_back(emptyClone);

        return emptyClone;
    }

    std::shared_ptr<HAL::BufferResource<uint8_t>> CopyDevice::QueueResourceCopyToReadbackMemory(std::shared_ptr<HAL::TextureResource> texture)
    {
        HAL::ResourceFootprint textureFootprint{ *texture };

        auto emptyClone = std::make_shared<HAL::BufferResource<uint8_t>>(
            *mDevice, textureFootprint.TotalSizeInBytes(), 1, HAL::CPUAccessibleHeapType::Readback);

        for (const HAL::SubresourceFootprint& subresourceFootprint : textureFootprint.SubresourceFootprints())
        {
            mCommandList.CopyTextureToBuffer(*texture, *emptyClone, subresourceFootprint);
        }

        // Hold in RAM until actual copy is performed
        mResourcesToCopy.push_back(texture);
        mResourcesToCopy.push_back(emptyClone);

        return emptyClone;
    }

    void CopyDevice::CopyResources()
    {
        if (mResourcesToCopy.empty()) return;

        mCommandList.Close();
        mCommandQueue.ExecuteCommandList(mCommandList);
        mFence.IncreaseExpectedValue();
        mCommandQueue.SignalFence(mFence);
        mFence.StallCurrentThreadUntilCompletion();
        
        mCommandAllocator.Reset();
        mCommandList.Reset(mCommandAllocator);

        mResourcesToCopy.clear();
    }

    void CopyDevice::WaitFence(HAL::Fence& fence)
    {
        mCommandQueue.WaitFence(fence);
    }

    void CopyDevice::SignalFence(HAL::Fence& fence)
    {
        mCommandQueue.SignalFence(fence);
    }

}
