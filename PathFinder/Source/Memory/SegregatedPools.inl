#include <cmath>
#include "../Foundation/Assert.hpp"

namespace Memory
{

    template <class BucketUserData, class SlotUserData>
    SegregatedPools<BucketUserData, SlotUserData>::SegregatedPools(uint64_t minimumBucketSlotSize, uint64_t bucketGrowSlotCount)
        : mMinimumBucketSlotSize{ minimumBucketSlotSize }, mGrowSlotCount{ bucketGrowSlotCount } {}

    template <class BucketUserData, class SlotUserData>
    uint32_t SegregatedPools<BucketUserData, SlotUserData>::CalculateBucketIndex(uint64_t allocationSize)
    {
        auto valueLog = std::log2((float)allocationSize);
        return std::ceil(valueLog);
    }

    template <class BucketUserData, class SlotUserData>
    uint64_t SegregatedPools<BucketUserData, SlotUserData>::CeilToClosestPowerOf2(uint64_t value)
    {
        auto valueLog = std::log2((float)value);
        auto newPowerOf2 = (uint32_t)std::ceil(value);
        return std::pow(2.0, newPowerOf2);
    }

    template <class BucketUserData, class SlotUserData>
    uint64_t SegregatedPools<BucketUserData, SlotUserData>::BucketSingleSlotSize(uint32_t bucketIndex) const
    {
        return mBuckets[bucketIndex].SlotSize();
    }

    template <class BucketUserData, class SlotUserData>
    SegregatedPools<BucketUserData, SlotUserData>::Allocation
        SegregatedPools<BucketUserData, SlotUserData>::Allocate(uint64_t allocationSize)
    {
        assert_format(allocationSize >= mMinimumBucketSlotSize,
            "Requested allocation is smaller than the minimum provided to allocator at construction");

        auto bucketIndex = CalculateBucketIndex(allocationSize);
        auto allocationSize = std::pow(2.0, bucketIndex);

        // Create missing pools if existing pools do not satisfy allocation memory requirement
        if (bucketIndex >= mBuckets.size())
        {
            auto numberOfBucketsToAdd = (bucketIndex + 1) - mBuckets.size();
          
            for (auto i = 0; i < numberOfBucketsToAdd; ++i)
            {
                auto bucket = mBuckets.emplace_back();
                bucket.mBucketIndex = mBuckets.size() - 1;
                bucket.mSlotSize = std::pow(2.0, bucketIndex);
            }
        }

        return { &mBuckets[bucketIndex], mBuckets[bucketIndex].Allocate() };
    }

    template <class BucketUserData, class SlotUserData>
    void SegregatedPools<BucketUserData, SlotUserData>::Deallocate(const SegregatedPools<BucketUserData, SlotUserData>::Allocation& allocation)
    {
        mBuckets[allocation.Bucket->mBucketIndex].Deallocate(allocation.Slot);
    }

}
