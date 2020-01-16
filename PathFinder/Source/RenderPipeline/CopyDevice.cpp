#include "CopyDevice.hpp"

namespace PathFinder
{

    CopyDevice::CopyDevice(const HAL::Device* device, uint8_t simultaneousFramesInFlight)
        : mDevice{ device }, 
        mRingCommandList{ *device, simultaneousFramesInFlight }, 
        mCommandQueue{ *device },
        mSimultaneousFramesInFlight{ simultaneousFramesInFlight }
    {
        mResourcesToCopy.resize(simultaneousFramesInFlight);
        mCommandQueue.SetDebugName("Copy Device Command Queue");
        mRingCommandList.SetDebugName("Copy Device");
    }
 
    std::shared_ptr<HAL::Texture> CopyDevice::QueueResourceCopyToDefaultMemory(std::shared_ptr<HAL::Texture> texture)
    {
        auto emptyClone = std::make_shared<HAL::Texture>(
            *mDevice, texture->Format(), texture->Kind(), texture->Dimensions(),
            texture->OptimizedClearValue(), HAL::ResourceState::CopyDestination, texture->ExpectedStates()
        );

        mRingCommandList.CurrentCommandList().CopyResource(*texture, *emptyClone);

        // Hold in RAM until actual copy is performed
        mResourcesToCopy[mCurrentFrameIndex].push_back(texture);
        mResourcesToCopy[mCurrentFrameIndex].push_back(emptyClone);

        return emptyClone;
    }

    std::shared_ptr<HAL::Buffer<uint8_t>> CopyDevice::QueueResourceCopyToReadbackMemory(std::shared_ptr<HAL::Texture> texture)
    {
        HAL::ResourceFootprint textureFootprint{ *texture };

        auto emptyClone = std::make_shared<HAL::Buffer<uint8_t>>(
            *mDevice, textureFootprint.TotalSizeInBytes(), 1, HAL::CPUAccessibleHeapType::Readback);

        for (const HAL::SubresourceFootprint& subresourceFootprint : textureFootprint.SubresourceFootprints())
        {
            mRingCommandList.CurrentCommandList().CopyTextureToBuffer(*texture, *emptyClone, subresourceFootprint);
        }

        // Hold in RAM until actual copy is performed
        mResourcesToCopy[mCurrentFrameIndex].push_back(texture);
        mResourcesToCopy[mCurrentFrameIndex].push_back(emptyClone);

        return emptyClone;
    }

    void CopyDevice::ExecuteCommands(const HAL::Fence* fenceToWaitFor, const HAL::Fence* fenceToSignal)
    {
        if (fenceToWaitFor)
        {
            mCommandQueue.WaitFence(*fenceToWaitFor);
        }

        auto& commandList = mRingCommandList.CurrentCommandList();

        commandList.Close();
        mCommandQueue.ExecuteCommandList(commandList);

        if (fenceToSignal)
        {
            mCommandQueue.SignalFence(*fenceToSignal);
        }
    }

    void CopyDevice::ResetCommandList()
    {
        mRingCommandList.CurrentCommandList().Reset(mRingCommandList.CurrentCommandAllocator());
        mResourcesToCopy[mCurrentFrameIndex].clear();
    }

    void CopyDevice::BeginFrame(uint64_t newFrameNumber)
    {
        mRingCommandList.PrepareCommandListForNewFrame(newFrameNumber);
        mCurrentFrameIndex = (newFrameNumber - mLastFenceValue) % mSimultaneousFramesInFlight;
    }

    void CopyDevice::EndFrame(uint64_t completedFrameNumber)
    {
        mResourcesToCopy[mCurrentFrameIndex].clear();
        mRingCommandList.ReleaseAndResetForCompletedFrames(completedFrameNumber);
        mLastFenceValue = completedFrameNumber;
    }

}
