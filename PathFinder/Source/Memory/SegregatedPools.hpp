#pragma once

#include "Pool.hpp"

#include <vector>

namespace Memory
{

    template <class BucketUserData = void, class SlotUserData = void>
    class SegregatedPools;

    template <
        class BucketUserData, // User data to associate with each bucket
        class SlotUserData // User data to associate with each individual slot in a bucket
    >
    struct SegregatedPoolsBucket
    {
    public:
        BucketUserData UserData;

    private:
        friend class SegregatedPools<BucketUserData, SlotUserData>;

        Pool<SlotUserData> mSlots;
        uint64_t mSlotSize;
        uint64_t mBucketIndex;

    public:
        inline auto SlotSize() const { return mSlotSize; }
    };



    template <class BucketUserData, class SlotUserData>
    struct SegregatedPoolsAllocation
    {
        SegregatedPoolsBucket<BucketUserData, SlotUserData>* Bucket;
        typename Pool<SlotUserData>::Slot Slot;
    };


    /// Maintains a list of buckets each represented by a Pool
    /// and segregated by Pool's element (slot) size
    template <class BucketUserData, class SlotUserData>
    class SegregatedPools
    {
    public:
        using Allocation = SegregatedPoolsAllocation<BucketUserData, SlotUserData>;
        using Bucket = SegregatedPoolsBucket<BucketUserData, SlotUserData>;

        SegregatedPools(uint64_t minimumBucketSlotSize, uint64_t bucketGrowSlotCount);

        Allocation Allocate(uint64_t allocationSize);
        void Deallocate(const Allocation& allocation);

    private:
        uint64_t CeilToClosestPowerOf2(uint64_t value);
        uint32_t CalculateBucketIndex(uint64_t allocationSize);

        std::vector<Bucket> mBuckets;

        uint64_t mMinimumBucketSlotSize = 4096;
        uint64_t mGrowSlotCount = 0;
    };

}

#include "SegregatedPools.inl"
