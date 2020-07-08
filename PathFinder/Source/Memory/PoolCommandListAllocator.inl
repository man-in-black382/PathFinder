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
            Pool<void>& packagePool,
            std::vector<CommandListPackage<CommandListT, CommandAllocatorT>>& packages,
            std::optional<uint64_t>& currentPackageIndex,
            uint64_t threadIndex,
            CommandListPackageType packageType)
    {
        // If frame changed we need to request new package of command allocator and command lists
        // by either taking existing one from the pool or creating a new one if none are available
        if (!currentPackageIndex || *currentPackageIndex != mCurrentFrameIndex)
        {
            Pool<void>::SlotType packageSlot = packagePool.Allocate();
            auto packageIndex = packageSlot.MemoryOffset;

            if (packageIndex >= packages.size())
            {
                packages.emplace_back(*mDevice);
            }

            currentPackageIndex = packageIndex;
        }

        // Get command list from a pool associated with the package
        CommandListPackage<CommandListT, CommandAllocatorT>& package = packages[*currentPackageIndex];
        Pool<void>::SlotType commandListSlot = package.CommandListPool.Allocate();
        auto cmdListIndex = commandListSlot.MemoryOffset;

        if (cmdListIndex >= package.CommandLists.size())
        {
            package.CommandLists.emplace_back(new CommandListT{ *mDevice, package.CommandAllocator.get() });
        }

        CommandListT* cmdList = package.CommandLists[cmdListIndex];
        // Return command lists in closed state
        cmdList->Close();

        auto deleter = [this, commandListSlot, threadIndex, packageType](CommandListT* cmdList)
        {
            mPendingDeallocations[mCurrentFrameIndex].emplace_back(Deallocation{ commandListSlot, cmdList, threadIndex, packageType });
        };

        return { cmdList, deleter };
    }

}
