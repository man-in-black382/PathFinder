#include "../Foundation/MemoryUtils.hpp"

namespace Memory
{
    template <class Element>
    std::unique_ptr<HAL::Buffer> SegregatedPoolsResourceAllocator::AllocateBuffer(
        const HAL::Buffer::Properties<Element> properties, std::optional<HAL::CPUAccessibleHeapType> heapType)
    {
        HAL::ResourceFormat format = HAL::Buffer::ConstructResourceFormat(mDevice, properties);
        auto [allocation, pools] = FindOrAllocateMostFittingFreeSlot(format.ResourceSizeInBytes(), format, heapType);

        HeapIterator heapIt = *allocation.Slot.UserData.HeapListIterator;
        assert_format(heapIt, "Heap reference is missing. It must be present.");

        auto offsetInHeap = AdjustMemoryOffsetToPointInsideHeap(allocation);

        // If CPU accessible buffer is requested
        if (heapType)
        {
            // We can search for existing one
            if (!allocation.Slot.UserData.Buffer)
            {
                // Allocate buffer with slot size, not requested size, to make it more generic and suitable for later reuse in other allocations
                allocation.Slot.UserData.Buffer = new HAL::Buffer{ mDevice, properties, *heapIt, offsetInHeap };
            }

            auto deallocationCallback = [this, pools, allocation](HAL::Buffer* buffer)
            {
                // Do not pass cpu accessible resource for deallocation. We can reuse it later.
                mPendingDeallocations[mCurrentFrameIndex].emplace_back(nullptr, allocation, pools);
            };

            // Create unique_ptr with already existing buffer ptr that's being reused
            return std::unique_ptr<HAL::Buffer>(allocation.Slot.UserData.Buffer, deallocationCallback);
        }
        else
        {
            auto deallocationCallback = [this, pools, allocation](HAL::Buffer* buffer)
            {
                mPendingDeallocations[mCurrentFrameIndex].emplace_back(buffer, allocation, pools);
            };

            // The design decision is to recreate buffers in default memory due to different state requirements unlike upload/readback 
            return std::make_unique<HAL::Buffer>(mDevice, properties, *heapIt, offsetInHeap, deallocationCallback);
        }
    }

}
