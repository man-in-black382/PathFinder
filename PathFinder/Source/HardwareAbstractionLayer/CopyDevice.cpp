#include "CopyDevice.hpp"

namespace HAL
{

    CopyDevice::CopyDevice(const Device* device)
        : mDevice{ device },
        mCommandAllocator{ *mDevice },
        mCommandList{ *mDevice, mCommandAllocator },
        mCommandQueue{ *device },
        mFence{ *mDevice } {}
 
    std::unique_ptr<TextureResource> CopyDevice::QueueResourceCopyToDefaultHeap(std::shared_ptr<TextureResource> texture)
    {
        auto emptyClone = std::make_unique<TextureResource>(
            *mDevice, texture->Format(), texture->Kind(), texture->Dimensions(),
            texture->OptimizedClearValue(), texture->InitialStates(), texture->ExpectedStates()
        );

        mCommandList.CopyResource(*texture, *emptyClone);

        // Hold in RAM until actual copy is performed
        mResourcesToCopy.push_back(texture);

        return std::move(emptyClone);
    }

    void CopyDevice::CopyResources()
    {
        mCommandList.Close();
        mCommandQueue.ExecuteCommandList(mCommandList);
        mFence.IncreaseExpectedValue();
        mCommandQueue.SignalFence(mFence);
        mFence.StallCurrentThreadUntilCompletion();
        mCommandAllocator.Reset();
        mCommandList.Reset(mCommandAllocator);

        mResourcesToCopy.clear();
    }

}
