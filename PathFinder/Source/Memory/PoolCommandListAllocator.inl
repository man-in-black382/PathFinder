namespace Memory
{

    template <>
    PoolCommandListAllocator::CopyCommandListPtr PoolCommandListAllocator::AllocateCommandList()
    {
        return AllocateCopyCommandList();
    }

    template <>
    PoolCommandListAllocator::ComputeCommandListPtr PoolCommandListAllocator::AllocateCommandList()
    {
        return AllocateComputeCommandList();
    }

    template <>
    PoolCommandListAllocator::GraphicsCommandListPtr PoolCommandListAllocator::AllocateCommandList()
    {
        return AllocateGraphicsCommandList();
    }

    template <class CommandListT, class DeleterT>
    std::unique_ptr<CommandListT, DeleterT> PoolCommandListAllocator::AllocateCommandList(Pool<void>& pool, std::vector<CommandListT*>& cmdLists)
    {
        Pool<void>::SlotType slot = pool.Allocate();
        auto index = slot.MemoryOffset;

        if (index >= cmdLists.size())
        {
            cmdLists.emplace_back(new CommandListT{ *mDevice });
        }

        auto deleter = [this, slot, &pool, cmdList = cmdLists[index]](CommandListT* cmdList)
        {
            mPendingDeallocations[mCurrentFrameIndex].emplace_back(Deallocation{ &pool, slot, cmdList });
        };

        return { cmdLists[index], deleter };
    }

}
