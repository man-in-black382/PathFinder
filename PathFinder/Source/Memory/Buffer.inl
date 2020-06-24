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
        GPUResource(uploadStrategy, stateTracker, resourceAllocator, descriptorAllocator, commandListProvider),
        mRequstedStride{ Foundation::MemoryUtils::Align(sizeof(Element), properties.ElementAlighnment) }
    {
        if (uploadStrategy == GPUResource::UploadStrategy::Automatic)
        {
            mBufferPtr = resourceAllocator->AllocateBuffer(properties);
            if (mStateTracker) mStateTracker->StartTrakingResource(mBufferPtr.get());
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
        GPUResource(UploadStrategy::Automatic, stateTracker, resourceAllocator, descriptorAllocator, commandListProvider),
        mRequstedStride{ Foundation::MemoryUtils::Align(sizeof(Element), properties.ElementAlighnment) }
    {
        mBufferPtr = SegregatedPoolsResourceAllocator::BufferPtr{
            new HAL::Buffer{ device, properties, mainResourceExplicitHeap, explicitHeapOffset },
            [](HAL::Buffer* buffer) { delete buffer; }
        };

        if (mStateTracker) mStateTracker->StartTrakingResource(mBufferPtr.get());
    }

    template <class Element>
    uint64_t Buffer::Capacity(uint64_t elementAlignment) const
    {
        return mBufferPtr ?
            mBufferPtr->ElementCapacity<Element>(elementAlignment) :
            mUploadBuffers.front().first->ElementCapacity<Element>(elementAlignment);
    }

}
