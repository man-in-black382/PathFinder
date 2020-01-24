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
 

    void CopyDevice::QueueBufferToTextureCopy(
        const HAL::Buffer& buffer,
        const HAL::Texture& texture,
        const HAL::ResourceFootprint& footprint)
    {
        for (const HAL::SubresourceFootprint& subresourceFootprint : footprint.SubresourceFootprints())
        {
            mRingCommandList.CurrentCommandList().CopyBufferToTexture(buffer, texture, subresourceFootprint);
        }
    }

    void CopyDevice::QueueBufferToBufferCopy(
        const HAL::Buffer& source,
        const HAL::Buffer& destination,
        uint64_t sourceOffset, uint64_t objectCount, uint64_t destinationOffset)
    {
        mRingCommandList.CurrentCommandList().CopyBufferRegion(source, destination, sourceOffset, objectCount, destinationOffset);
    }

    void CopyDevice::QueueBufferToTextureCopy(
        std::shared_ptr<HAL::Buffer> buffer,
        std::shared_ptr<HAL::Texture> texture,
        const HAL::ResourceFootprint& footprint)
    {
        QueueBufferToTextureCopy(*buffer, *texture, footprint);

        // Hold in RAM until actual copy is performed
        mResourcesToCopy[mCurrentFrameIndex].push_back(buffer);
        mResourcesToCopy[mCurrentFrameIndex].push_back(texture);
    }

    void CopyDevice::QueueBufferToBufferCopy(
        std::shared_ptr<HAL::Buffer> source,
        std::shared_ptr<HAL::Buffer> destination,
        uint64_t sourceOffset, uint64_t objectCount, uint64_t destinationOffset)
    {
        QueueBufferToBufferCopy(*source, *destination, sourceOffset, objectCount, destinationOffset);

        // Hold in RAM until actual copy is performed
        mResourcesToCopy[mCurrentFrameIndex].push_back(source);
        mResourcesToCopy[mCurrentFrameIndex].push_back(destination);
    }

    std::shared_ptr<HAL::Buffer> CopyDevice::QueueResourceCopyToDefaultMemory(std::shared_ptr<HAL::Buffer> buffer)
    {
        auto emptyClone = std::make_shared<HAL::Buffer>(
            *mDevice, buffer->Size(), HAL::ResourceState::CopyDestination, buffer->ExpectedStates());

        mRingCommandList.CurrentCommandList().CopyResource(*buffer, *emptyClone);

        // Hold in RAM until actual copy is performed
        mResourcesToCopy[mCurrentFrameIndex].push_back(buffer);
        mResourcesToCopy[mCurrentFrameIndex].push_back(emptyClone);

        return emptyClone;
    }

    std::shared_ptr<HAL::Buffer> CopyDevice::QueueResourceCopyToReadbackMemory(std::shared_ptr<HAL::Buffer> buffer)
    {
        auto emptyClone = std::make_shared<HAL::Buffer>(
            *mDevice, buffer->Size(), HAL::CPUAccessibleHeapType::Readback);

        mRingCommandList.CurrentCommandList().CopyResource(*buffer, *emptyClone);

        // Hold in RAM until actual copy is performed
        mResourcesToCopy[mCurrentFrameIndex].push_back(buffer);
        mResourcesToCopy[mCurrentFrameIndex].push_back(emptyClone);

        return emptyClone;
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

    std::shared_ptr<HAL::Buffer> CopyDevice::QueueResourceCopyToReadbackMemory(std::shared_ptr<HAL::Texture> texture)
    {
        HAL::ResourceFootprint textureFootprint{ *texture };

        auto emptyClone = std::make_shared<HAL::Buffer>(
            *mDevice, textureFootprint.TotalSizeInBytes(), HAL::CPUAccessibleHeapType::Readback);

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
