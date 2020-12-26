#include "Buffer.hpp"
#include "CopyRequestManager.hpp"

namespace Memory
{

    Buffer::Buffer(
        const HAL::BufferProperties& properties, 
        GPUResource::AccessStrategy accessStrategy, 
        ResourceStateTracker* stateTracker,
        SegregatedPoolsResourceAllocator* resourceAllocator, 
        PoolDescriptorAllocator* descriptorAllocator, 
        CopyRequestManager* copyRequestManager)
        :
        GPUResource(accessStrategy, stateTracker, resourceAllocator, descriptorAllocator, copyRequestManager),
        mRequstedStride{ properties.Stride },
        mProperties{ properties }
    {
        if (accessStrategy == GPUResource::AccessStrategy::Automatic)
        {
            mBufferPtr = resourceAllocator->AllocateBuffer(properties);
            mGetterBufferPtr = mBufferPtr.get();

            if (mStateTracker) 
                mStateTracker->StartTrakingResource(mBufferPtr.get());
        }
    }

    Buffer::Buffer(
        const HAL::BufferProperties& properties, 
        ResourceStateTracker* stateTracker, 
        SegregatedPoolsResourceAllocator* resourceAllocator, 
        PoolDescriptorAllocator* descriptorAllocator, 
        CopyRequestManager* copyRequestManager,
        const HAL::Device& device, 
        const HAL::Heap& mainResourceExplicitHeap, 
        uint64_t explicitHeapOffset)
        :
        GPUResource(AccessStrategy::Automatic, stateTracker, resourceAllocator, descriptorAllocator, copyRequestManager),
        mRequstedStride{ properties.Stride },
        mProperties{ properties }
    {
        mBufferPtr = SegregatedPoolsResourceAllocator::BufferPtr{
            new HAL::Buffer{ device, properties, mainResourceExplicitHeap, explicitHeapOffset },
            [](HAL::Buffer* buffer) { delete buffer; }
        };

        mGetterBufferPtr = mBufferPtr.get();

        if (mStateTracker)
            mStateTracker->StartTrakingResource(mBufferPtr.get());
    }

    Buffer::~Buffer()
    {
        if (mStateTracker && mBufferPtr) 
            mStateTracker->StopTrakingResource(mBufferPtr.get());
    }

    const HAL::CBDescriptor* Buffer::GetCBDescriptor() const
    {
        // Descriptor needs to be created either if it does not exist yet
        // or we're only using upload buffers (Direct Access) and no descriptors were 
        // created for upload buffer of this frame
        //
        if (!mCBDescriptor || (mAccessStrategy == GPUResource::AccessStrategy::DirectUpload && mCBDescriptorRequestFrameNumber != mFrameNumber))
        {
            mCBDescriptor = mDescriptorAllocator->AllocateCBDescriptor(*HALBuffer(), mRequstedStride);
            mCBDescriptorRequestFrameNumber = mFrameNumber;
        }

        return mCBDescriptor.get();
    }

    const HAL::UADescriptor* Buffer::GetUADescriptor() const
    {
        assert_format(mAccessStrategy != GPUResource::AccessStrategy::DirectUpload,
            "Direct Access buffers cannot have Unordered Access descriptors since they're always in GenericRead state");

        if (!mUADescriptor)
        {
            mUADescriptor = mDescriptorAllocator->AllocateUADescriptor(*HALBuffer(), mRequstedStride);
        }

        return mUADescriptor.get();
    }

    const HAL::SRDescriptor* Buffer::GetSRDescriptor() const
    {
        // Descriptor needs to be created either if it does not exist yet
        // or we're only using upload buffers (Direct Access) and no descriptors were 
        // created for upload buffer of this frame
        //
        if (!mSRDescriptor || (!mBufferPtr && mSRDescriptorRequestFrameNumber != mFrameNumber))
        {
            mSRDescriptor = mDescriptorAllocator->AllocateSRDescriptor(*HALBuffer(), mRequstedStride);
            mSRDescriptorRequestFrameNumber = mFrameNumber;
        }

        return mSRDescriptor.get();
    }

    const HAL::Buffer* Buffer::HALBuffer() const
    {
        return mGetterBufferPtr;
    }

    const HAL::Resource* Buffer::HALResource() const
    {
        return HALBuffer();
    }

    void Buffer::BeginFrame(uint64_t frameNumber)
    {
        GPUResource::BeginFrame(frameNumber);

        HAL::Buffer* newCurrentBuffer = nullptr;

        if (mAccessStrategy == GPUResource::AccessStrategy::DirectUpload)
            newCurrentBuffer = CurrentFrameUploadBuffer();
        else if (mAccessStrategy == GPUResource::AccessStrategy::DirectReadback)
            newCurrentBuffer = CurrentFrameReadbackBuffer();

        if (newCurrentBuffer)
            mGetterBufferPtr = newCurrentBuffer;
    }

    uint64_t Buffer::ResourceSizeInBytes() const
    {
        return mProperties.Size;
    }

    void Buffer::ApplyDebugName()
    {
        GPUResource::ApplyDebugName();

        if (mBufferPtr)
        {
            mBufferPtr->SetDebugName(mDebugName);
        }
    }

    CopyRequestManager::CopyCommand Buffer::GetUploadCommands()
    {
        return[&](HAL::CopyCommandListBase& cmdList)
        {
            if (mAccessStrategy != GPUResource::AccessStrategy::DirectUpload)
            {
                cmdList.CopyBufferRegion(*CurrentFrameUploadBuffer(), *HALBuffer(), 0, HALBuffer()->ElementCapacity(), 0);
            }
        };
    }

    CopyRequestManager::CopyCommand Buffer::GetReadbackCommands()
    {
        return[&](HAL::CopyCommandListBase& cmdList)
        {
            cmdList.CopyBufferRegion(*HALBuffer(), *CurrentFrameReadbackBuffer(), 0, HALBuffer()->ElementCapacity(), 0);
        };
    }

}
