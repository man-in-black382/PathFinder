#include "../Foundation/MemoryUtils.hpp"

namespace Memory
{
    template <class Element>
    SegregatedPoolsResourceAllocator::BufferPtr SegregatedPoolsResourceAllocator::AllocateBuffer(
        const HAL::Buffer::Properties<Element>& properties, std::optional<HAL::CPUAccessibleHeapType> heapType)
    {
        HAL::ResourceFormat format = HAL::Buffer::ConstructResourceFormat(mDevice, properties);
        auto [allocation, pools] = FindOrAllocateMostFittingFreeSlot(format.ResourceSizeInBytes(), format, heapType);

        std::optional<HeapIterator> heapIt = allocation.Slot.UserData.HeapListIterator;
        assert_format(heapIt, "Heap reference is missing. It must be present.");

        auto offsetInHeap = AdjustMemoryOffsetToPointInsideHeap(allocation);

        // If CPU accessible buffer is requested
        if (heapType)
        {
            // We can search for existing one
            if (!allocation.Slot.UserData.Buffer)
            {
                // We need CPU accessible buffers to match slot size so that they can be reused properly
                HAL::Buffer::Properties cpuAccessibleBufferProperties{ allocation.Bucket->SlotSize() };

                // Allocate buffer with slot size, not requested size, to make it more generic and suitable for later reuse in other allocations
                allocation.Slot.UserData.Buffer = new HAL::Buffer{ *mDevice, cpuAccessibleBufferProperties, *(*heapIt), offsetInHeap };
            }

            auto deallocationCallback = [this, pools, allocation](HAL::Buffer* buffer)
            {
                // Do not pass cpu accessible resource for deallocation. We can reuse it later.
                mPendingDeallocations[mCurrentFrameIndex].emplace_back(Deallocation{ nullptr, allocation, pools });
            };

            // Create unique_ptr with already existing buffer ptr that's being reused
            return BufferPtr{ allocation.Slot.UserData.Buffer, deallocationCallback };
        }
        else
        {
            auto deallocationCallback = [this, pools, allocation](HAL::Buffer* buffer)
            {
                mPendingDeallocations[mCurrentFrameIndex].emplace_back(Deallocation{ buffer, allocation, pools });
            };

            HAL::Buffer* buffer = new HAL::Buffer{ *mDevice, properties, *(*heapIt), offsetInHeap };

            // The design decision is to recreate buffers in default memory due to different state requirements unlike upload/readback 
            return BufferPtr{ buffer, deallocationCallback };
        }
    }

}
