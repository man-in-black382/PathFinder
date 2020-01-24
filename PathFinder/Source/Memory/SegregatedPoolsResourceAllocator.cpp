#include "SegregatedPoolsResourceAllocator.hpp"

namespace Memory
{

    SegregatedPoolsResourceAllocator::SegregatedPoolsResourceAllocator(const HAL::Device* device)
        : mDevice{ device }, mPools{ mMinimumSlotSize, mOnGrowSlotCount }
    {

    }

    std::unique_ptr<HAL::Texture> SegregatedPoolsResourceAllocator::AllocateTexture(const HAL::Texture::Properties& properties)
    {
        HAL::ResourceFormat format = HAL::Texture::ConstructResourceFormat(
            mDevice, properties.Format, properties.Kind, properties.Dimensions, properties.MipCount, properties.OptimizedClearValue);
        
        format.SetExpectedStates(properties.ExpectedStateMask | properties.InitialStateMask);

        Pools::BucketSlot bucketSlot = mPools.Allocate(format.ResourceSizeInBytes());

        return std::make_unique<HAL::Texture>(mDevice, properties, [this, bucketSlot](HAL::Texture* texture) 
        {
            // Put texture in a deletion queue
            // .. put in queue ..

            mPools.Deallocate(bucketSlot);
        });
    }

    SegregatedPoolsResourceAllocator::PoolsAllocation SegregatedPoolsResourceAllocator::Allocate(
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

        auto allocation = pools->Allocate(alloctionSizeInBytes);
        HeapList& heaps = allocation.Bucket->UserData;
        auto heapIt = allocation.Slot.UserData.HeapListIterator;

        bool outOfAllocatedMemory = allocation.Slot.MemoryOffset >= allocation.Bucket->UserData.TotalHeapsSize;
        bool existingHeapReferencePresent = heapIt != std::nullopt;

        assert(!(outOfAllocatedMemory && existingHeapReferencePresent));

        bool needsNewHeap = 
    }

}
