namespace Memory
{

    template <>
    PoolCommandListAllocator::CopyCommandListPtr PoolCommandListAllocator::AllocateCommandList(uint64_t threadIndex)
    {
        return AllocateCopyCommandList();
    }

    template <>
    PoolCommandListAllocator::ComputeCommandListPtr PoolCommandListAllocator::AllocateCommandList(uint64_t threadIndex)
    {
        return AllocateComputeCommandList();
    }

    template <>
    PoolCommandListAllocator::GraphicsCommandListPtr PoolCommandListAllocator::AllocateCommandList(uint64_t threadIndex)
    {
        return AllocateGraphicsCommandList();
    }

    template <class CommandListT, class CommandAllocatorT, class DeleterT>
    std::unique_ptr<CommandListT, DeleterT>
        PoolCommandListAllocator::AllocateCommandList(
            std::vector<CommandListPackage<CommandListT, CommandAllocatorT>>& packages,
            uint64_t threadIndex,
            CommandListPackageType packageType)
    {
        // If frame changed we need to request new package of command allocator and command lists
        // by either taking existing one from the pool or creating a new one if none are available
        uint64_t packageIndex = mCurrentFrameIndex;

        if (packageIndex >= packages.size())
        {
            if (packageIndex >= packages.size())
            {
                packages.emplace_back(*mDevice);
                packages.back().CommandAllocator->SetDebugName(StringFormat("Command Allocator. Thread %d. Frame Index %d.", threadIndex, packageIndex));
            }
        }

        // Get command list from a pool associated with the package
        CommandListPackage<CommandListT, CommandAllocatorT>& package = packages[packageIndex];
        Pool<void>::SlotType commandListSlot = package.CommandListPool.Allocate();
        auto cmdListIndex = commandListSlot.MemoryOffset;

        if (cmdListIndex >= package.CommandLists.size())
        {
            package.CommandLists.emplace_back(new CommandListT{ *mDevice, package.CommandAllocator.get() });
        }

        CommandListT* cmdList = package.CommandLists[cmdListIndex];
        // Return command lists in closed state
        cmdList->Close();

        Deallocation deallocation{ commandListSlot, cmdList, threadIndex, packageIndex, packageType };

        auto deleter = [this, deallocation](CommandListT* cmdList)
        {
            mPendingDeallocations[mCurrentFrameIndex].push_back(deallocation);
        };

        return { cmdList, deleter };
    }

}
