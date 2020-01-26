#pragma once

#include "SegregatedPools.hpp"

#include "../HardwareAbstractionLayer/Device.hpp"
#include "../HardwareAbstractionLayer/Heap.hpp"
#include "../HardwareAbstractionLayer/Buffer.hpp"
#include "../HardwareAbstractionLayer/Texture.hpp"
#include "../HardwareAbstractionLayer/BufferCPUAccessor.hpp"

#include <memory>

namespace Memory
{

    class SegregatedPoolsResourceAllocator
    {
    public:
        SegregatedPoolsResourceAllocator(const HAL::Device* device);

        template <class T> std::unique_ptr<HAL::Buffer> AllocateBuffer(uint64_t capacity, uint64_t perElementAlignment, HAL::ResourceState initialState, HAL::ResourceState expectedStates);
        template <class T> std::unique_ptr<HAL::BufferCPUAccessor<T>> AllocateBuffer(uint64_t capacity, uint64_t perElementAlignment, HAL::CPUAccessibleHeapType heapType);
        
        std::unique_ptr<HAL::Texture> AllocateTexture(const HAL::Texture::Properties& properties);

    private:
        using HeapList = std::list<HAL::Heap>;
        using HeapIterator = HeapList::iterator;

        struct SlotUserData
        {
            std::optional<HeapIterator> HeapListIterator = std::nullopt;

            // We only store buffers inside pool slots
            // to keep them alive and reuse on new buffer allocation requests.
            // Textures are recreated on each allocation request and not stored here.
            // Manually managed.
            HAL::Buffer* Buffer = nullptr;
        };

        struct BucketUserData
        {
            // List of heaps for one bucket 
            HeapList Heaps;
            
            // Tracks total size of heaps in a bucket
            uint64_t TotalHeapsSize = 0;
        };

        // We store actual heaps inside segregated pool buckets 
        // and resource and a reference to one of those heaps the 
        // resource is allocated in inside pool slot
        using Pools = SegregatedPools<BucketUserData, SlotUserData>;
        using PoolsAllocation = SegregatedPoolsAllocation<BucketUserData, SlotUserData>;

        std::pair<PoolsAllocation, Pools*> FindOrAllocateMostFittingFreeSlot(
            uint64_t alloctionSizeInBytes, const HAL::ResourceFormat& resourceFormat, std::optional<HAL::CPUAccessibleHeapType> cpuHeapType);

        uint64_t AdjustMemoryOffsetToPointInsideHeap(const Pool<SlotUserData>::Slot& slot);

        const HAL::Device* mDevice = nullptr;

        // Minimum allocation size
        uint64_t mMinimumSlotSize = 4096; 

        // Amount of slots to allocate at once when out of allocated memory
        uint32_t mOnGrowSlotCount = 10; 

        // Buffer only upload heaps
        Pools mUploadPools;

        // Buffer only readback heaps
        Pools mReadbackPools;

        // Used for universal heaps when supported by hardware.
        // Used only for default memory buffer heaps otherwise.
        Pools mDefaultUniversalOrBufferPools;

        // RT & DS texture only, default memory heaps. Unused when universal heaps are supported by HW.
        Pools mDefaultRTDSPools;

        // Other texture type, default memory heaps. Unused when universal heaps are supported by HW.
        Pools mDefaultNonRTDSPools;
    };

}

#include "SegregatedPoolsResourceAllocator.inl"