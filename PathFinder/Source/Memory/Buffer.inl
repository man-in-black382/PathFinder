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
        HAL::CopyCommandListBase* commandList)
        :
        GPUResource(uploadStrategy, stateTracker, resourceAllocator, descriptorAllocator, commandList)
    {
        mBufferPtr = uploadStrategy == GPUResource::UploadStrategy::Automatic ? // Skip allocating default memory buffer when direct access
            resourceAllocator->AllocateBuffer(properties) : nullptr; // through persistent mapping is requested

        if (StateTracker() && mBufferPtr) StateTracker()->StartTrakingResource(HALBuffer());
    }

    template <class Element>
    Buffer::Buffer(
        const HAL::Buffer::Properties<Element>& properties, 
        ResourceStateTracker* stateTracker, 
        SegregatedPoolsResourceAllocator* resourceAllocator, 
        PoolDescriptorAllocator* descriptorAllocator, 
        HAL::CopyCommandListBase* commandList, 
        const HAL::Device& device, 
        const HAL::Heap& mainResourceExplicitHeap, 
        uint64_t explicitHeapOffset)
        :
        GPUResource(UploadStrategy::Automatic, stateTracker, resourceAllocator, descriptorAllocator, commandList)
    {
        mBufferPtr = SegregatedPoolsResourceAllocator::BufferPtr{
            new HAL::Buffer{ device, properties, mainResourceExplicitHeap, explicitHeapOffset },
            [](HAL::Buffer* buffer) { delete buffer; }
        };
    }


    template <class Element>
    const HAL::CBDescriptor* Buffer::GetOrCreateCBDescriptor(uint64_t elementAlignment)
    {
        assert_format(CurrentUploadStrategy() != GPUResource::UploadStrategy::DirectAccess,
            "DirectAccess and descriptor creation are incompatible");

        auto stride = MemoryUtils::Align(sizeof(Element), elementAlignment);

        if (mCurrentCBDescriptorStride != stride || !mCBDescriptor)
        {
            mCBDescriptor = DescriptorAllocator()->AllocateCBDescriptor(HALBuffer(), stride);
            mCurrentCBDescriptorStride = stride;
        }
    }

    template <class Element>
    const HAL::UADescriptor* Buffer::GetOrCreateUADescriptor(uint64_t elementAlignment)
    {
        assert_format(CurrentUploadStrategy() != GPUResource::UploadStrategy::DirectAccess,
            "DirectAccess and descriptor creation are incompatible");

        auto stride = MemoryUtils::Align(sizeof(Element), elementAlignment);

        if (mCurrentUADescriptorStride != stride || !mUADescriptor)
        {
            mUADescriptor = DescriptorAllocator()->AllocateSRDescriptor(HALBuffer(), stride);
            mCurrentUADescriptorStride = stride;
        }
    }

    template <class Element>
    const HAL::SRDescriptor* Buffer::GetOrCreateSRDescriptor(uint64_t elementAlignment)
    {
        assert_format(CurrentUploadStrategy() != GPUResource::UploadStrategy::DirectAccess,
            "DirectAccess and descriptor creation are incompatible");

        auto stride = MemoryUtils::Align(sizeof(Element), elementAlignment);

        if (mCurrentSRDescriptorStride != stride || !mSRDescriptor)
        {
            mSRDescriptor = DescriptorAllocator()->AllocateSRDescriptor(HALBuffer(), stride);
            mCurrentSRDescriptorStride = stride;
        }
    }

}
