#pragma once

#include <cstdint>
#include <list>

namespace Memory
{

    template <class SlotUserData = void>
    class Pool
    {
    public:
        template <class UserDataT>
        struct Slot
        {
            uint64_t MemoryOffset;
            UserDataT UserData;
        };

        template <>
        struct Slot<void>
        {
            uint64_t MemoryOffset;
        };

        using SlotType = Slot<SlotUserData>;

        Pool(uint64_t slotSize, uint64_t onGrowSlotCount);

        SlotType Allocate();
        void Deallocate(const SlotType& slot);

    private:
        void Grow();

        uint64_t mGrowSlotCount = 0;
        uint64_t mAllocatedSize = 0;
        uint64_t mSlotSize = 0;
        std::list<SlotType> mFreeSlots;

    public:
        inline auto SlotSize() const { return mSlotSize; }
    };

}

#include "Pool.inl"