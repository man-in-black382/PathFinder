#include "../Foundation/MemoryUtils.hpp"

namespace Memory
{

    template <class T>
    std::unique_ptr<HAL::BufferCPUAccessor<T>> SegregatedPoolsResourceAllocator::AllocateBuffer(uint64_t capacity, uint64_t perElementAlignment, HAL::CPUAccessibleHeapType heapType)
    {
        HAL::ResourceFormat format = HAL::Buffer::ConstructResourceFormat(mDevice, Foundation::MemoryUtils::Align(capacity * sizeof(T), perElementAlignment));

        auto [allocation, pools] = FindOrAllocateMostFittingFreeSlot(format.ResourceSizeInBytes(), format, std::nullopt);

        // Buffer does not exist. Need to create one.
        if (!allocation.Slot.UserData.Buffer)
        {
            allocation.Slot.UserData.Buffer = new HAL::Buffer{
                mDevice, &(*allocation.Slot.UserData.HeapListIterator), allocation.Slot.MemoryOffset, }
        }

        HAL::Buffer* buffer = allocation.Slot.UserData.Buffer;

        auto deallocationCallback = [this, pools, allocation](HAL::BufferCPUAccessor* accessor)
        {
            pools->Deallocate(allocation);
        });

        // Recycle existing buffer if it exists
        

        

        HeapIterator heapIt = *allocation.Slot.UserData.HeapListIterator;

        return std::make_unique<HAL::Texture>(mDevice, properties, 
    }

    template <class T>
    std::unique_ptr<HAL::Buffer> SegregatedPoolsResourceAllocator::AllocateBuffer(uint64_t capacity, uint64_t perElementAlignment, HAL::ResourceState initialState, HAL::ResourceState expectedStates)
    {

    }

}
