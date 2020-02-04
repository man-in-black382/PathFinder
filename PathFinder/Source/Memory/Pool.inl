namespace Memory
{

    template <class SlotUserData>
    Pool<SlotUserData>::Pool(uint64_t slotSize, uint64_t onGrowSlotCount)
        : mSlotSize{ slotSize }, mGrowSlotCount{ onGrowSlotCount } {}

    template <class SlotUserData>
    void Pool<SlotUserData>::Grow()
    {
        for (auto i = 0u; i < mGrowSlotCount; ++i)
        {
            mFreeSlots.emplace_back(SlotType{ mAllocatedSize });
            mAllocatedSize += mSlotSize;
        }
    }

    template <class SlotUserData>
    void Pool<SlotUserData>::Deallocate(const Pool<SlotUserData>::SlotType& slot)
    {
        mFreeSlots.push_back(slot);
    }

    template <class SlotUserData>
    typename Pool<SlotUserData>::SlotType Pool<SlotUserData>::Allocate()
    {
        if (mFreeSlots.empty())
        {
            Grow();
        }

        Slot<SlotUserData> slot = mFreeSlots.front();
        mFreeSlots.pop_front();
        return slot;
    }

}
