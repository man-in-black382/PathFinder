#include "CopyDevice.hpp"

namespace PathFinder
{

    CopyDevice::CopyDevice(const HAL::Device* device)
        : mDevice{ device },
        mCommandAllocator{ *mDevice },
        mCommandList{ *mDevice, mCommandAllocator },
        mCommandQueue{ *device },
        mFence{ *mDevice } {}
 
    std::unique_ptr<HAL::TextureResource> CopyDevice::QueueResourceCopyToDefaultHeap(std::shared_ptr<HAL::TextureResource> texture)
    {
        assert_format(IsCopyableState(texture->InitialStates()), "Resource must be in a copyable state");

        auto emptyClone = std::make_unique<HAL::TextureResource>(
            *mDevice, texture->Format(), texture->Kind(), texture->Dimensions(),
            texture->OptimizedClearValue(), HAL::ResourceState::CopyDestination, texture->ExpectedStates()
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

    bool CopyDevice::IsCopyableState(HAL::ResourceState state)
    {
        return EnumMaskBitSet(state, HAL::ResourceState::Common) ||
            EnumMaskBitSet(state, HAL::ResourceState::GenericRead) ||
            EnumMaskBitSet(state, HAL::ResourceState::CopySource);
    }

}
