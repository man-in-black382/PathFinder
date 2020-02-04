#include <cmath>
#include "../Foundation/Assert.hpp"

namespace Memory
{


    template <class BucketUserData, class SlotUserData>
    SegregatedPoolsBucket<BucketUserData, SlotUserData>::SegregatedPoolsBucket(uint64_t slotSize, uint64_t onGrowSlotCount)
        : mSlots{ slotSize, onGrowSlotCount } {}



    template <class BucketUserData, class SlotUserData>
    SegregatedPools<BucketUserData, SlotUserData>::SegregatedPools(uint64_t minimumBucketSlotSize, uint64_t bucketGrowSlotCount)
        : mMinimumBucketSlotSize{ minimumBucketSlotSize }, mGrowSlotCount{ bucketGrowSlotCount } {}

    template <class BucketUserData, class SlotUserData>
    uint32_t SegregatedPools<BucketUserData, SlotUserData>::CalculateBucketIndex(uint64_t allocationSize)
    {
        auto valueLog = std::log2f((float)allocationSize);
        return std::ceil(valueLog);
    }

    template <class BucketUserData, class SlotUserData>
    uint64_t SegregatedPools<BucketUserData, SlotUserData>::CeilToClosestPowerOf2(uint64_t value)
    {
        auto valueLog = std::log2f((float)value);
        auto newPowerOf2 = std::ceil(value);
        return std::powf(2.0f, newPowerOf2);
    }

    template <class BucketUserData, class SlotUserData>
    typename SegregatedPools<BucketUserData, SlotUserData>::Allocation
        SegregatedPools<BucketUserData, SlotUserData>::Allocate(uint64_t allocationSize)
    {
        assert_format(allocationSize >= mMinimumBucketSlotSize,
            "Requested allocation is smaller than the minimum provided to allocator at construction");

        auto bucketIndex = CalculateBucketIndex(allocationSize);

        // Create missing pools if existing pools do not satisfy allocation memory requirement
        if (bucketIndex >= mBuckets.size())
        {
            auto numberOfBucketsToAdd = (bucketIndex + 1) - mBuckets.size();
          
            for (auto i = 0; i < numberOfBucketsToAdd; ++i)
            {
                uint64_t slotSize = std::powf(2.0f, bucketIndex);
                auto& bucket = mBuckets.emplace_back(slotSize, mGrowSlotCount);
                bucket.mBucketIndex = mBuckets.size() - 1;
                bucket.mSlotSize = slotSize;
            }
        }

        return { &mBuckets[bucketIndex], mBuckets[bucketIndex].mSlots.Allocate() };
    }

    template <class BucketUserData, class SlotUserData>
    void SegregatedPools<BucketUserData, SlotUserData>::Deallocate(const SegregatedPools<BucketUserData, SlotUserData>::Allocation& allocation)
    {
        mBuckets[allocation.Bucket->mBucketIndex].mSlots.Deallocate(allocation.Slot);
    }

}
