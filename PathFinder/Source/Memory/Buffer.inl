#include "../Foundation/MemoryUtils.hpp"
#include "../Foundation/Assert.hpp"

namespace Memory
{

    template <class Element>
    Buffer::Buffer(
        const HAL::Buffer::Properties<Element>& properties, 
        GPUResource::UploadStrategy uploadStrategy, 
        ResourceStateTracker* stateTracker,
        SegregatedPoolsResourceAllocator* resourceAllocator, 
        PoolDescriptorAllocator* descriptorAllocator,
        CopyCommandListProvider* commandListProvider)
        :
        GPUResource(uploadStrategy, stateTracker, resourceAllocator, descriptorAllocator, commandListProvider)
    {
        if (uploadStrategy == GPUResource::UploadStrategy::Automatic)
        {
            mBufferPtr = resourceAllocator->AllocateBuffer(properties);
            if (mStateTracker) mStateTracker->StartTrakingResource(HALBuffer());
        }
        else
        {
            mUploadBuffers.emplace(resourceAllocator->AllocateBuffer(properties, HAL::CPUAccessibleHeapType::Upload), 0);
        }
    }

    template <class Element>
    Buffer::Buffer(
        const HAL::Buffer::Properties<Element>& properties, 
        ResourceStateTracker* stateTracker, 
        SegregatedPoolsResourceAllocator* resourceAllocator, 
        PoolDescriptorAllocator* descriptorAllocator, 
        CopyCommandListProvider* commandListProvider,
        const HAL::Device& device, 
        const HAL::Heap& mainResourceExplicitHeap, 
        uint64_t explicitHeapOffset)
        :
        GPUResource(UploadStrategy::Automatic, stateTracker, resourceAllocator, descriptorAllocator, commandListProvider)
    {
        mBufferPtr = SegregatedPoolsResourceAllocator::BufferPtr{
            new HAL::Buffer{ device, properties, mainResourceExplicitHeap, explicitHeapOffset },
            [](HAL::Buffer* buffer) { delete buffer; }
        };

        if (mStateTracker) mStateTracker->StartTrakingResource(HALBuffer());
    }

    template <class Element>
    uint64_t Buffer::ElementCapacity(uint64_t elementAlignment) const
    {
        return mBufferPtr ?
            mBufferPtr->ElementCapacity<Element>(elementAlignment) :
            mUploadBuffers.front().first->ElementCapacity<Element>(elementAlignment);
    }

    template <class Element>
    const HAL::CBDescriptor* Buffer::GetOrCreateCBDescriptor(uint64_t elementAlignment)
    {
        assert_format(mUploadStrategy != GPUResource::UploadStrategy::DirectAccess,
            "DirectAccess and descriptor creation are incompatible");

        auto stride = MemoryUtils::Align(sizeof(Element), elementAlignment);

        if (mCurrentCBDescriptorStride != stride || !mCBDescriptor)
        {
            mCBDescriptor = mDescriptorAllocator->AllocateCBDescriptor(*HALBuffer(), stride);
            mCurrentCBDescriptorStride = stride;
        }
    }

    template <class Element>
    const HAL::UADescriptor* Buffer::GetOrCreateUADescriptor(uint64_t elementAlignment)
    {
        assert_format(mUploadStrategy != GPUResource::UploadStrategy::DirectAccess,
            "DirectAccess and descriptor creation are incompatible");

        auto stride = MemoryUtils::Align(sizeof(Element), elementAlignment);

        if (mCurrentUADescriptorStride != stride || !mUADescriptor)
        {
            mUADescriptor = mDescriptorAllocator->AllocateSRDescriptor(*HALBuffer(), stride);
            mCurrentUADescriptorStride = stride;
        }
    }

    template <class Element>
    const HAL::SRDescriptor* Buffer::GetOrCreateSRDescriptor(uint64_t elementAlignment)
    {
        assert_format(mUploadStrategy != GPUResource::UploadStrategy::DirectAccess,
            "DirectAccess and descriptor creation are incompatible");

        auto stride = MemoryUtils::Align(sizeof(Element), elementAlignment);

        if (mCurrentSRDescriptorStride != stride || !mSRDescriptor)
        {
            mSRDescriptor = mDescriptorAllocator->AllocateSRDescriptor(*HALBuffer(), stride);
            mCurrentSRDescriptorStride = stride;
        }
    }

}
