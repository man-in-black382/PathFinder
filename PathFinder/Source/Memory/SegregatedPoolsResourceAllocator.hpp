#pragma once

#include "SegregatedPools.hpp"
#include "Ring.hpp"

#include "../HardwareAbstractionLayer/Device.hpp"
#include "../HardwareAbstractionLayer/Heap.hpp"
#include "../HardwareAbstractionLayer/Buffer.hpp"
#include "../HardwareAbstractionLayer/Texture.hpp"

#include <memory>
#include <vector>

namespace Memory
{

    class SegregatedPoolsResourceAllocator
    {
    public:
        using BufferPtr = std::unique_ptr<HAL::Buffer, std::function<void(HAL::Buffer*)>>;
        using TexturePtr = std::unique_ptr<HAL::Texture, std::function<void(HAL::Texture*)>>;

        SegregatedPoolsResourceAllocator(const HAL::Device* device, uint8_t simultaneousFramesInFlight);

        template <class Element>
        BufferPtr AllocateBuffer(const HAL::Buffer::Properties<Element>& properties, std::optional<HAL::CPUAccessibleHeapType> heapType = std::nullopt);
        TexturePtr AllocateTexture(const HAL::Texture::Properties& properties);

        void BeginFrame(uint64_t frameNumber);
        void EndFrame(uint64_t frameNumber);

    private:
        using HeapList = std::vector<HAL::Heap>;
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
            // List of heaps associated with a bucket
            HeapList* HeapListPtr = nullptr;
        };

        // We store actual heaps inside segregated pool buckets 
        // and resource and a reference to one of those heaps the 
        // resource is allocated in inside pool slot
        using Pools = SegregatedPools<BucketUserData, SlotUserData>;
        using PoolsAllocation = SegregatedPoolsAllocation<BucketUserData, SlotUserData>;
        using PoolsBucket = SegregatedPoolsBucket<BucketUserData, SlotUserData>;

        struct Deallocation
        {
            HAL::Resource* Resource = nullptr;
            PoolsAllocation Allocation;
            Pools* PoolsThatProducedAllocation;
        };

        std::pair<PoolsAllocation, Pools*> FindOrAllocateMostFittingFreeSlot(
            uint64_t alloctionSizeInBytes, const HAL::ResourceFormat& resourceFormat, std::optional<HAL::CPUAccessibleHeapType> cpuHeapType);

        uint64_t AdjustMemoryOffsetToPointInsideHeap(const PoolsAllocation& allocation);
        uint64_t TotalHeapsSizeInBytes(const PoolsBucket& bucket);
        void ExecutePendingDeallocations(uint64_t frameIndex);

        const HAL::Device* mDevice = nullptr;

        Ring mRingFrameTracker;

        uint8_t mSimultaneousFramesInFlight;
        uint64_t mCurrentFrameIndex = 0;

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

        std::vector<HeapList> mHeapLists;

        std::vector<std::vector<Deallocation>> mPendingDeallocations;
    };

}

#include "SegregatedPoolsResourceAllocator.inl"