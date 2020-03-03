#include "Buffer.hpp"

namespace Memory
{

    Buffer::~Buffer()
    {
        if (mStateTracker && mBufferPtr) mStateTracker->StopTrakingResource(HALBuffer());
    }

    const HAL::CBDescriptor* Buffer::GetOrCreateCBDescriptor()
    {
        if (!mCBDescriptor || !mBufferPtr)
        {
            mCBDescriptor = mDescriptorAllocator->AllocateCBDescriptor(*HALBuffer(), mRequstedStride);
        }

        return mCBDescriptor.get();
    }

    const HAL::UADescriptor* Buffer::GetOrCreateUADescriptor()
    {
        assert_format(mUploadStrategy != GPUResource::UploadStrategy::DirectAccess,
            "Direct Access buffers cannot have Unordered Access descriptors since they're always in GenericRead state");

        if (!mUADescriptor)
        {
            mUADescriptor = mDescriptorAllocator->AllocateUADescriptor(*HALBuffer(), mRequstedStride);
        }

        return mUADescriptor.get();
    }

    const HAL::SRDescriptor* Buffer::GetOrCreateSRDescriptor()
    {
        if (!mSRDescriptor || !mBufferPtr)
        {
            mSRDescriptor = mDescriptorAllocator->AllocateSRDescriptor(*HALBuffer(), mRequstedStride);
        }

        return mSRDescriptor.get();
    }

    void Buffer::RequestWrite()
    {
        GPUResource::RequestWrite();

        if (mUploadStrategy != GPUResource::UploadStrategy::DirectAccess)
        {
            mCommandListProvider->CommandList()->CopyBufferRegion(*CurrentFrameUploadBuffer(), *HALBuffer(), 0, HALBuffer()->ElementCapacity(), 0);
        }
    }

    void Buffer::RequestRead()
    {
        GPUResource::RequestRead();
        mCommandListProvider->CommandList()->CopyBufferRegion(*HALBuffer(), *CurrentFrameReadbackBuffer(), 0, HALBuffer()->ElementCapacity(), 0);
    }

    const HAL::Buffer* Buffer::HALBuffer() const
    {
        return mUploadStrategy == GPUResource::UploadStrategy::Automatic ? 
            mBufferPtr.get() : CurrentFrameUploadBuffer();
    }

    const HAL::Resource* Buffer::HALResource() const
    {
        return HALBuffer();
    }

    uint64_t Buffer::ResourceSizeInBytes() const
    {
        return mBufferPtr ? mBufferPtr->TotalMemory() : mUploadBuffers.front().first->TotalMemory();
    }

    void Buffer::ApplyDebugName()
    {
        GPUResource::ApplyDebugName();

        if (mBufferPtr)
        {
            mBufferPtr->SetDebugName(mDebugName);
        }
    }

}
