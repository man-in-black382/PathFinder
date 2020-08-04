#include "../Foundation/MemoryUtils.hpp"

namespace Memory
{
    template <class Element>
    SegregatedPoolsResourceAllocator::BufferPtr SegregatedPoolsResourceAllocator::AllocateBuffer(
        const HAL::BufferProperties<Element>& properties, std::optional<HAL::CPUAccessibleHeapType> heapType)
    {
        HAL::ResourceFormat format{ mDevice, properties };
        Allocation allocation = FindOrAllocateMostFittingFreeSlot(format.ResourceSizeInBytes(), format, heapType);
        PoolsAllocation& poolAllocation = allocation.PoolAllocation;

        auto offsetInHeap = AdjustMemoryOffsetToPointInsideHeap(allocation);

        // If CPU accessible buffer is requested
        if (heapType)
        {
            // We can search for existing one
            if (!poolAllocation.Slot.UserData.Buffer)
            {
                // We need CPU accessible buffers to match slot size so that they can be reused properly
                HAL::BufferProperties cpuAccessibleBufferProperties{ allocation.PoolsPtr->SlotSizeInBucket(poolAllocation.BucketIndex) };

                // Allocate buffer with slot size, not requested size, to make it more generic and suitable for later reuse in other allocations
                poolAllocation.Slot.UserData.Buffer = new HAL::Buffer{ *mDevice, cpuAccessibleBufferProperties, *allocation.HeapPtr, offsetInHeap };
            }

            auto deallocationCallback = [this, poolAllocation, poolsThatProducedAllocation = allocation.PoolsPtr](HAL::Buffer* buffer)
            {
                // Do not pass cpu accessible resource for deallocation. We can reuse it later.
                mPendingDeallocations[mCurrentFrameIndex].emplace_back(Deallocation{ buffer, poolAllocation, poolsThatProducedAllocation, true });
            };

            // Create unique_ptr with already existing buffer ptr that's being reused
            return BufferPtr{ poolAllocation.Slot.UserData.Buffer, deallocationCallback };
        }
        else
        {
            auto deallocationCallback = [this, poolAllocation, poolsThatProducedAllocation = allocation.PoolsPtr](HAL::Buffer* buffer)
            {
                mPendingDeallocations[mCurrentFrameIndex].emplace_back(Deallocation{ buffer, poolAllocation, poolsThatProducedAllocation, false });
            };

            HAL::Buffer* buffer = new HAL::Buffer{ *mDevice, properties, *allocation.HeapPtr, offsetInHeap };

            // The design decision is to recreate buffers in default memory due to different state requirements unlike upload/readback 
            return BufferPtr{ buffer, deallocationCallback };
        }
    }

}
