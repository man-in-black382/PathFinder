#pragma once

#include <cstdint>
#include <list>

namespace Memory
{

    template <class SlotUserData = void>
    class Pool
    {
    public:
        struct Slot
        {
            uint64_t MemoryOffset;
            SlotUserData UserData;
        };

        Pool(uint64_t slotSize, uint64_t onGrowSlotCount);

        Slot Allocate();
        void Deallocate(const Slot& slot);

    private:
        void Grow();

        uint64_t mGrowSlotCount = 0;
        uint64_t mAllocatedSize = 0;
        uint64_t mSlotSize = 0;
        std::list<Slot> mFreeSlots;

    public:
        inline auto SlotSize() const { return mSlotSize; }
    };

}

#include "Pool.inl"