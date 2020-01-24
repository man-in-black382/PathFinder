namespace Memory
{

    template <class SlotUserData>
    Pool<SlotUserData>::Pool(uint64_t slotSize, uint64_t onGrowSlotCount)
        : mSlotSize{ slotSize }, mGrowSlotCount{ initialSlotCount } {}

    template <class SlotUserData>
    void Pool<SlotUserData>::Grow()
    {
        for (auto i = 0u; i < mGrowSlotCount; ++i)
        {
            mFreeSlots.emplace_back(Slot{ mAllocatedSize });
            mAllocatedSize += mSlotSize;
        }
    }

    template <class SlotUserData>
    void Pool<SlotUserData>::Deallocate(const Slot& slot)
    {
        mFreeSlots.push_back(slot);
    }

    template <class SlotUserData>
    Pool<SlotUserData>::Slot Pool<SlotUserData>::Allocate()
    {
        if (mFreeSlots.empty())
        {
            Grow();
        }

        Slot slot = mFreeSlots.front();
        mFreeSlots.pop_front();
        return slot;
    }

}
