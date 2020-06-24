#include "GPUResource.hpp"

namespace Memory
{

    GPUResource::GPUResource(
        UploadStrategy uploadStrategy,
        ResourceStateTracker* stateTracker,
        SegregatedPoolsResourceAllocator* resourceAllocator,
        PoolDescriptorAllocator* descriptorAllocator,
        CopyCommandListProvider* commandListProvider)
        :
        mUploadStrategy{ uploadStrategy },
        mStateTracker{ uploadStrategy == UploadStrategy::DirectAccess ? nullptr : stateTracker },
        mResourceAllocator{ resourceAllocator },
        mDescriptorAllocator{ descriptorAllocator },
        mCommandListProvider{ commandListProvider } {}

    GPUResource::~GPUResource() {}

    void GPUResource::RequestWrite(HAL::CopyCommandListBase* customCmdList)
    {
        // Upload is already requested in current frame
        if (!mUploadBuffers.empty() && mUploadBuffers.back().second == mFrameNumber)
        {
            return;
        }

        AllocateNewUploadBuffer();

        if (mUploadStrategy != UploadStrategy::DirectAccess)
        {
            const ResourceStateTracker::SubresourceStateList& previousStates = mStateTracker->ResourceCurrentStates(HALResource());

            HAL::ResourceBarrierCollection barriers = mStateTracker->TransitionToStateImmediately(HALResource(), HAL::ResourceState::CopyDestination);
            HAL::CopyCommandListBase* commandList = customCmdList ? customCmdList : mCommandListProvider->CommandList();
            commandList->InsertBarriers(barriers);

            RecordUploadCommands();
            // Request to apply old state after copy
            mStateTracker->RequestTransitions(HALResource(), previousStates);
        }
    }

    void GPUResource::RequestRead(HAL::CopyCommandListBase* customCmdList)
    {
        assert_format(mUploadStrategy != UploadStrategy::DirectAccess, "DirectAccess upload resource does not support reads");

        // Readback is already requested in current frame
        if (!mReadbackBuffers.empty() && mReadbackBuffers.back().second == mFrameNumber)
        {
            return;
        }

        AllocateNewReadbackBuffer();

        const ResourceStateTracker::SubresourceStateList& previousStates = mStateTracker->ResourceCurrentStates(HALResource());

        HAL::ResourceBarrierCollection barriers = mStateTracker->TransitionToStateImmediately(HALResource(), HAL::ResourceState::CopyDestination);
        HAL::CopyCommandListBase* commandList = customCmdList ? customCmdList : mCommandListProvider->CommandList();
        commandList->InsertBarriers(barriers);
        
        RecordReadbackCommands();
        // Request to apply old state after copy
        mStateTracker->RequestTransitions(HALResource(), previousStates);
    }

    void GPUResource::RequestNewState(HAL::ResourceState newState)
    {
        if (mStateTracker) mStateTracker->RequestTransition(HALResource(), newState);
    }

    void GPUResource::RequestNewSubresourceStates(const ResourceStateTracker::SubresourceStateList& newStates)
    {
        if (mStateTracker) mStateTracker->RequestTransitions(HALResource(), newStates);
    }

    void GPUResource::BeginFrame(uint64_t frameNumber)
    {
        mFrameNumber = frameNumber;

        if (mUploadStrategy == UploadStrategy::DirectAccess)
        {
            // Direct upload resources must have at least one upload buffer at all times
            if (mCompletedUploadBuffer)
            {
                // Either reuse a completed upload buffers
                mCompletedUploadBuffer->SetDebugName(StringFormat("%s Upload Buffer [Frame %d]", mDebugName.c_str(), mFrameNumber));
                mUploadBuffers.emplace(std::move(mCompletedUploadBuffer), mFrameNumber);
            }
            else if (!mUploadBuffers.empty() && mUploadBuffers.back().second == mFrameNumber)
            {
                // Or allocate a new one if none are completed yet
                AllocateNewUploadBuffer();
            }  
        }
        else
        {
            // For other upload strategies we can get rid of the memory until a write operation is requested
            mCompletedUploadBuffer = nullptr;
        }
        
        // Readback memory we just free unconditionally since reading upload only resources is not permitted in the first place.
        // A window to read back the data is after frame end but before new frame start.
        mCompletedReadbackBuffer = nullptr;
    }

    void GPUResource::EndFrame(uint64_t frameNumber)
    {
        // Release upload buffers for completed frames
        while (!mUploadBuffers.empty() && mUploadBuffers.front().second <= frameNumber)
        {
            mCompletedUploadBuffer = std::move(mUploadBuffers.front().first);
            mUploadBuffers.pop();
        }

        // Get freshest completed readback buffer. Discard the rest.
        while (!mReadbackBuffers.empty() && mReadbackBuffers.front().second <= frameNumber)
        {
            mCompletedReadbackBuffer = std::move(mReadbackBuffers.front().first);
            mReadbackBuffers.pop();
        }
    }

    void GPUResource::SetDebugName(const std::string& name)
    {
        mDebugName = name;
        ApplyDebugName();
    }

    const HAL::Resource* GPUResource::HALResource() const
    {
        return nullptr;
    }

    HAL::Buffer* GPUResource::CurrentFrameUploadBuffer()
    {
        return !mUploadBuffers.empty() && mUploadBuffers.back().second == mFrameNumber ? 
            mUploadBuffers.back().first.get() : nullptr;
    }

    const HAL::Buffer* GPUResource::CurrentFrameUploadBuffer() const
    {
        return !mUploadBuffers.empty() && mUploadBuffers.back().second == mFrameNumber ?
            mUploadBuffers.back().first.get() : nullptr;
    }

    HAL::Buffer* GPUResource::CurrentFrameReadbackBuffer()
    {
        return !mReadbackBuffers.empty() && mReadbackBuffers.back().second == mFrameNumber ?
            mReadbackBuffers.back().first.get() : nullptr;
    }

    const HAL::Buffer* GPUResource::CurrentFrameReadbackBuffer() const
    {
        return !mReadbackBuffers.empty() && mReadbackBuffers.back().second == mFrameNumber ?
            mReadbackBuffers.back().first.get() : nullptr;
    }

    void GPUResource::ApplyDebugName()
    {
    }

    void GPUResource::AllocateNewUploadBuffer()
    {
        HAL::Buffer::Properties properties{ ResourceSizeInBytes() };
        mUploadBuffers.emplace(mResourceAllocator->AllocateBuffer(properties, HAL::CPUAccessibleHeapType::Upload), mFrameNumber);
        mUploadBuffers.back().first->SetDebugName(StringFormat("%s Upload Buffer [Frame %d]", mDebugName.c_str(), mFrameNumber));
    }

    void GPUResource::AllocateNewReadbackBuffer()
    {
        HAL::Buffer::Properties properties{ ResourceSizeInBytes() };
        mReadbackBuffers.emplace(mResourceAllocator->AllocateBuffer(properties, HAL::CPUAccessibleHeapType::Readback), mFrameNumber);
        mReadbackBuffers.back().first->SetDebugName(StringFormat("%s Readback Buffer [Frame %d]", mDebugName.c_str(), mFrameNumber));
    }

}
