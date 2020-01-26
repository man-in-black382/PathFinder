#include "SegregatedPoolsResourceAllocator.hpp"

namespace Memory
{

    SegregatedPoolsResourceAllocator::SegregatedPoolsResourceAllocator(const HAL::Device* device)
        : mDevice{ device },
        mUploadPools{ mMinimumSlotSize, mOnGrowSlotCount },
        mReadbackPools{ mMinimumSlotSize, mOnGrowSlotCount },
        mDefaultUniversalOrBufferPools{ mMinimumSlotSize, mOnGrowSlotCount },
        mDefaultRTDSPools{ mMinimumSlotSize, mOnGrowSlotCount },
        mDefaultNonRTDSPools{ mMinimumSlotSize, mOnGrowSlotCount }
    {

    }

    std::unique_ptr<HAL::Texture> SegregatedPoolsResourceAllocator::AllocateTexture(const HAL::Texture::Properties& properties)
    {
        HAL::ResourceFormat format = HAL::Texture::ConstructResourceFormat(
            mDevice, properties.Format, properties.Kind, properties.Dimensions, properties.MipCount, properties.OptimizedClearValue);
        
        format.SetExpectedStates(properties.ExpectedStateMask | properties.InitialStateMask);

        auto [allocation, pools] = FindOrAllocateMostFittingFreeSlot(format.ResourceSizeInBytes(), format, std::nullopt);

        HeapIterator heapIt = *allocation.Slot.UserData.HeapListIterator;

        return std::make_unique<HAL::Texture>(mDevice, properties, [this, pools, allocation](HAL::Texture* texture)
        {
            // Put texture in a deletion queue
            // .. put in queue ..

            // Return slot to its pool to be potentially reused for other allocations
            pools->Deallocate(allocation);
        });
    }

    std::pair<SegregatedPoolsResourceAllocator::PoolsAllocation, SegregatedPoolsResourceAllocator::Pools*> 
        SegregatedPoolsResourceAllocator::FindOrAllocateMostFittingFreeSlot(
        uint64_t alloctionSizeInBytes, const HAL::ResourceFormat& resourceFormat, std::optional<HAL::CPUAccessibleHeapType> cpuHeapType)
    {
        Pools* pools = nullptr;

        if (cpuHeapType)
        {
            switch (*cpuHeapType)
            {
            case HAL::CPUAccessibleHeapType::Upload:
                pools = &mUploadPools;
                break;

            case HAL::CPUAccessibleHeapType::Readback:
                pools = &mReadbackPools;
                break;
            }
        }
        else
        {
            switch (resourceFormat.ResourceAliasingGroup())
            {
            case HAL::HeapAliasingGroup::Universal:
            case HAL::HeapAliasingGroup::Buffers:
                pools = &mDefaultUniversalOrBufferPools;
                break;

            case HAL::HeapAliasingGroup::RTDSTextures:
                pools = &mDefaultRTDSPools;
                break;

            case HAL::HeapAliasingGroup::NonRTDSTextures:
                pools = &mDefaultNonRTDSPools;
                break;
            }
        }

        SegregatedPoolsAllocation allocation = pools->Allocate(alloctionSizeInBytes);
        HeapList& heaps = allocation.Bucket->UserData.Heaps;
        HeapIterator heapIt = allocation.Slot.UserData.HeapListIterator;

        bool outOfAllocatedMemory = allocation.Slot.MemoryOffset >= allocation.Bucket->UserData.TotalHeapsSize;
        bool existingHeapReferencePresent = heapIt != std::nullopt;

        assert_format(!(outOfAllocatedMemory && existingHeapReferencePresent),
            "Implementation error. Heap reference is present but memory offset indicates that a new heap is required.");

        // Out of memory means we need to add another heap
        if (outOfAllocatedMemory)
        {
            auto newHeapSize = mOnGrowSlotCount * allocation.Bucket->SlotSize();

            // Track the sum of heap sizes associated with the bucket
            allocation.Bucket->UserData.TotalHeapsSize += newHeapSize; 
            heaps.emplace_front(mDevice, newHeapSize, resourceFormat.ResourceAliasingGroup(), cpuHeapType);
        }

        // Heap definitely exists at this point but its reference is not recorded in the slot,
        // which means this slot has not ever been requested yet.
        // All new heaps not referenced by slots are located at the beginning of the list.
        if (!existingHeapReferencePresent)
        {
            allocation.Slot.UserData.HeapListIterator = heaps.begin();
        }

        return std::make_pair(std::move(allocation), pools);
    }

    uint64_t SegregatedPoolsResourceAllocator::AdjustMemoryOffsetToPointInsideHeap(const Pool<SlotUserData>::Slot& slot)
    {

    }

}
