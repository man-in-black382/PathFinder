#include "SegregatedPoolsResourceAllocator.hpp"

#include "../Foundation/Assert.hpp"

namespace Memory
{

    SegregatedPoolsResourceAllocator::SegregatedPoolsResourceAllocator(const HAL::Device* device, uint8_t simultaneousFramesInFlight)
        : mDevice{ device }, 
        mRingFrameTracker{ simultaneousFramesInFlight },
        mSimultaneousFramesInFlight{ simultaneousFramesInFlight },
        mUploadPools{ mMinimumSlotSize, mOnGrowSlotCount },
        mReadbackPools{ mMinimumSlotSize, mOnGrowSlotCount },
        mDefaultUniversalOrBufferPools{ mMinimumSlotSize, mOnGrowSlotCount },
        mDefaultRTDSPools{ mMinimumSlotSize, mOnGrowSlotCount },
        mDefaultNonRTDSPools{ mMinimumSlotSize, mOnGrowSlotCount }
    {
        mPendingDeallocations.resize(simultaneousFramesInFlight);

        mRingFrameTracker.SetDeallocationCallback([this](const Ring::FrameTailAttributes& frameAttributes)
        {
            auto frameIndex = frameAttributes.Tail - frameAttributes.Size;
            ExecutePendingDeallocations(frameIndex);
        });
    }

    std::unique_ptr<HAL::Texture> SegregatedPoolsResourceAllocator::AllocateTexture(const HAL::Texture::Properties& properties)
    {
        HAL::ResourceFormat format = HAL::Texture::ConstructResourceFormat(mDevice, properties);
        auto [allocation, pools] = FindOrAllocateMostFittingFreeSlot(format.ResourceSizeInBytes(), format, std::nullopt);

        std::optional<HeapIterator> heapIt = allocation.Slot.UserData.HeapListIterator;
        assert_format(heapIt, "Heap reference is missing. It must be present.");

        auto offsetInHeap = AdjustMemoryOffsetToPointInsideHeap(allocation);

        auto deallocationCallback = [this, pools, allocation](HAL::Texture* texture)
        {
            mPendingDeallocations[mCurrentFrameIndex].emplace_back(Deallocation{ texture, allocation, pools });
        };

        return std::make_unique<HAL::Texture>(mDevice, *heapIt, offsetInHeap, properties, deallocationCallback);
    }

    void SegregatedPoolsResourceAllocator::BeginFrame(uint64_t frameNumber)
    {
        mCurrentFrameIndex = mRingFrameTracker.Allocate(1);
        mRingFrameTracker.FinishCurrentFrame(frameNumber);
    }

    void SegregatedPoolsResourceAllocator::EndFrame(uint64_t frameNumber)
    {
        mRingFrameTracker.ReleaseCompletedFrames(frameNumber);
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
        std::optional<HeapIterator> heapIt = allocation.Slot.UserData.HeapListIterator;

        auto totalHeapsSize = TotalHeapsSizeInBytes(*allocation.Bucket);

        bool outOfAllocatedMemory = allocation.Slot.MemoryOffset >= totalHeapsSize;
        bool existingHeapReferencePresent = heapIt != std::nullopt;

        assert_format(!(outOfAllocatedMemory && existingHeapReferencePresent),
            "Implementation error. Heap reference is present but memory offset indicates that a new heap is required.");

        // Out of memory means we need to add another heap
        if (outOfAllocatedMemory)
        {
            auto newHeapSize = mOnGrowSlotCount * allocation.Bucket->SlotSize();
            heaps.emplace_back(mDevice, newHeapSize, resourceFormat.ResourceAliasingGroup(), cpuHeapType);
        }

        // Heap definitely exists at this point but its reference is not recorded in the slot,
        // which means this slot has not ever been requested yet.
        if (!existingHeapReferencePresent)
        {
            allocation.Slot.UserData.HeapListIterator = std::prev(heaps.end());
        }

        return std::make_pair(std::move(allocation), pools);
    }

    uint64_t SegregatedPoolsResourceAllocator::AdjustMemoryOffsetToPointInsideHeap(const PoolsAllocation& allocation)
    {
        // One heap is created per OnGrowSlotCount slots in a bucket.
        // So we can calculate an offset local to a particular heap.
        uint64_t bytesPerHeap = mOnGrowSlotCount * allocation.Bucket->SlotSize();
        uint64_t allocationHeapIndex = allocation.Slot.MemoryOffset / bytesPerHeap;
        uint64_t localOffset = allocation.Slot.MemoryOffset - allocationHeapIndex * bytesPerHeap;

        return localOffset;
    }

    uint64_t SegregatedPoolsResourceAllocator::TotalHeapsSizeInBytes(const PoolsBucket& bucket)
    {
        return bucket.UserData.Heaps.size() * mOnGrowSlotCount * bucket.SlotSize();
    }

    void SegregatedPoolsResourceAllocator::ExecutePendingDeallocations(uint64_t frameIndex)
    {
        for (Deallocation& deallocation : mPendingDeallocations[frameIndex])
        {
            if (deallocation.Resource)
            {
                delete deallocation.Resource;
            }
            deallocation.PoolsThatProducedAllocation->Deallocate(deallocation.Allocation);
        }
        mPendingDeallocations[frameIndex].clear();
    }

}
