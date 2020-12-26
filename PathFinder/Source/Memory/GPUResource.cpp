#include "GPUResource.hpp"

namespace Memory
{

    GPUResource::GPUResource(
        AccessStrategy accessStrategy,
        ResourceStateTracker* stateTracker,
        SegregatedPoolsResourceAllocator* resourceAllocator,
        PoolDescriptorAllocator* descriptorAllocator,
        CopyRequestManager* copyRequestManager)
        :
        mAccessStrategy{ accessStrategy },
        mStateTracker{ accessStrategy == AccessStrategy::Automatic ? stateTracker : nullptr },
        mResourceAllocator{ resourceAllocator },
        mDescriptorAllocator{ descriptorAllocator },
        mCopyRequestManager{ copyRequestManager } {}

    GPUResource::~GPUResource() {}

    void GPUResource::RequestWrite()
    {
        assert_format(mAccessStrategy != AccessStrategy::DirectReadback, "DirectReadback resource does not support CPU writes");

        // Upload is already requested in current frame
        if (!mUploadBuffers.empty() && mUploadBuffers.back().second == mFrameNumber)
        {
            return;
        }

        AllocateNewUploadBuffer();

        if (mAccessStrategy != AccessStrategy::DirectUpload)
        {
            mCopyRequestManager->RequestUpload(HALResource(), GetUploadCommands());
        }
    }

    void GPUResource::RequestRead()
    {
        assert_format(mAccessStrategy != AccessStrategy::DirectUpload, "DirectUpload resource does not support CPU reads");

        // Readback is already requested in current frame
        if (!mReadbackBuffers.empty() && mReadbackBuffers.back().second == mFrameNumber)
        {
            return;
        }

        AllocateNewReadbackBuffer();

        if (mAccessStrategy != AccessStrategy::DirectReadback)
        {
            mCopyRequestManager->RequestReadback(HALResource(), GetReadbackCommands());
        }
    }

    void GPUResource::RequestNewState(HAL::ResourceState newState)
    {
        if (mStateTracker) 
            mStateTracker->RequestTransition(HALResource(), newState);
    }

    void GPUResource::RequestNewSubresourceStates(const ResourceStateTracker::SubresourceStateList& newStates)
    {
        if (mStateTracker) 
            mStateTracker->RequestTransitions(HALResource(), newStates);
    }

    void GPUResource::BeginFrame(uint64_t frameNumber)
    {
        mFrameNumber = frameNumber;

        if (mAccessStrategy == AccessStrategy::DirectUpload)
        {
            // Direct upload resources must have at least one upload buffer at all times
            if (mCompletedUploadBuffer)
            {
                // Either reuse a completed upload buffers
                mCompletedUploadBuffer->SetDebugName(StringFormat("%s Upload Buffer [Frame %d]", mDebugName.c_str(), mFrameNumber));
                mUploadBuffers.emplace(std::move(mCompletedUploadBuffer), mFrameNumber);
            }
            else
            {
                // Or allocate a new one if none are completed yet
                AllocateNewUploadBuffer();
            }  
        }
        else if (mAccessStrategy == AccessStrategy::DirectReadback)
        {
            // Direct readback resources must have at least one readback buffer at all times
            if (mCompletedReadbackBuffer)
            {
                // Either reuse a completed readback buffer
                mCompletedReadbackBuffer->SetDebugName(StringFormat("%s Readback Buffer [Frame %d]", mDebugName.c_str(), mFrameNumber));
                mReadbackBuffers.emplace(std::move(mCompletedReadbackBuffer), mFrameNumber);
            }
            else
            {
                // Or allocate a new one if none are completed yet
                AllocateNewReadbackBuffer();
            }
        }
        else
        {
            // For automatic access strategy we can get rid of the memory until a read or write operation is requested.
            // A window to read back the data is after frame end but before new frame start.
            mCompletedUploadBuffer = nullptr;
            mCompletedReadbackBuffer = nullptr;
        }
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
        auto properties = HAL::BufferProperties::Create<uint8_t>(ResourceSizeInBytes());
        mUploadBuffers.emplace(mResourceAllocator->AllocateBuffer(properties, HAL::CPUAccessibleHeapType::Upload), mFrameNumber);
        mUploadBuffers.back().first->SetDebugName(StringFormat("%s Upload Buffer [Frame %d]", mDebugName.c_str(), mFrameNumber));
    }

    void GPUResource::AllocateNewReadbackBuffer()
    {
        auto properties = HAL::BufferProperties::Create<uint8_t>(ResourceSizeInBytes());
        mReadbackBuffers.emplace(mResourceAllocator->AllocateBuffer(properties, HAL::CPUAccessibleHeapType::Readback), mFrameNumber);
        mReadbackBuffers.back().first->SetDebugName(StringFormat("%s Readback Buffer [Frame %d]", mDebugName.c_str(), mFrameNumber));
    }

}
