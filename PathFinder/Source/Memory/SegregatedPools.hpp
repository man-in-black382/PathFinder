#pragma once

#include "Pool.hpp"

#include <vector>

namespace Memory
{

    template <
        class BucketUserData, // User data to associate with each bucket
        class SlotUserData // User data to associate with each individual slot in a bucket
    >
    struct SegregatedPoolsBucket
    {
    public:
        BucketUserData UserData;

    private:
        friend SegregatedPools;

        Pool<SlotUserData> mSlots;
        uint64_t mSlotSize;
        uint64_t mBucketIndex;

    public:
        inline auto SlotSize() const { return mSlotSize; }
    };



    template <class BucketUserData, class SlotUserData>
    struct SegregatedPoolsAllocation
    {
        const SegregatedPoolsBucket<BucketUserData, SlotUserData>* Bucket;
        Pool<SlotUserData>::Slot Slot;
    };


    /// Maintains a list of buckets each represented by a Pool
    /// and segregated by Pool's element (slot) size
    template <class BucketUserData, class SlotUserData>
    class SegregatedPools
    {
    public:
        SegregatedPools(uint64_t minimumBucketSlotSize, uint64_t bucketGrowSlotCount);

        SegregatedPoolsAllocation Allocate(uint64_t allocationSize);
        void Deallocate(const SegregatedPoolsAllocation& bucketSlot);

    private:
        uint64_t CeilToClosestPowerOf2(uint64_t value);
        uint32_t CalculateBucketIndex(uint64_t allocationSize);

        std::vector<Bucket> mBuckets;

        uint64_t mMinimumBucketSlotSize = 4096;
        uint64_t mGrowSlotCount = 0;
    };

}

#include "SegregatedPools.inl"
