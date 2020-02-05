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

}
