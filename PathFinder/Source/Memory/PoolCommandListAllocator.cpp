#include "PoolCommandListAllocator.hpp"

namespace Memory
{

    PoolCommandListAllocator::PoolCommandListAllocator(const HAL::Device* device, uint8_t simultaneousFramesInFlight)
        : mDevice{ device }, mRingFrameTracker{ simultaneousFramesInFlight }, mGraphicsPool{ 1, 1 }, mComputePool{ 1, 1 }, mCopyPool{ 1, 1 }
    {
        mRingFrameTracker.SetDeallocationCallback([this](const Ring::FrameTailAttributes& frameAttributes)
        {
            auto frameIndex = frameAttributes.Tail - frameAttributes.Size;
            ExecutePendingDealloactions(frameIndex);
        });

        mPendingDeallocations.resize(simultaneousFramesInFlight);
    }

    PoolCommandListAllocator::~PoolCommandListAllocator()
    {
        for (HAL::GraphicsCommandList* cmdList : mGraphicCommandLists) delete cmdList;
        for (HAL::ComputeCommandList* cmdList : mComputeCommandLists) delete cmdList;
        for (HAL::CopyCommandList* cmdList : mCopyCommandLists) delete cmdList;
        for (HAL::GraphicsCommandAllocator* allocator : mGraphicCommandAllocators) delete allocator;
        for (HAL::ComputeCommandAllocator* allocator : mComputeCommandAllocators) delete allocator;
        for (HAL::CopyCommandAllocator* allocator : mCopyCommandAllocators) delete allocator;
    }

    void PoolCommandListAllocator::BeginFrame(uint64_t frameNumber)
    {
        mCurrentFrameIndex = mRingFrameTracker.Allocate(1);
        mRingFrameTracker.FinishCurrentFrame(frameNumber);
    }

    void PoolCommandListAllocator::EndFrame(uint64_t frameNumber)
    {
        mRingFrameTracker.ReleaseCompletedFrames(frameNumber);
    }
  
    PoolCommandListAllocator::GraphicsCommandListPtr PoolCommandListAllocator::AllocateGraphicsCommandList()
    {
        return AllocateCommandList<
            HAL::GraphicsCommandList, 
            HAL::GraphicsCommandAllocator, 
            std::function<void(HAL::GraphicsCommandList*)>>
            (mGraphicsPool, mGraphicCommandLists, mGraphicCommandAllocators);
    }

    PoolCommandListAllocator::ComputeCommandListPtr PoolCommandListAllocator::AllocateComputeCommandList()
    {
        return AllocateCommandList<
            HAL::ComputeCommandList,
            HAL::ComputeCommandAllocator,
            std::function<void(HAL::ComputeCommandList*)>>
            (mComputePool, mComputeCommandLists, mComputeCommandAllocators);
    }

    PoolCommandListAllocator::CopyCommandListPtr PoolCommandListAllocator::AllocateCopyCommandList()
    {
        return AllocateCommandList<
            HAL::CopyCommandList,
            HAL::CopyCommandAllocator,
            std::function<void(HAL::CopyCommandList*)>>
            (mCopyPool, mCopyCommandLists, mCopyCommandAllocators);
    }

    template <class CommandListT, class CommandAllocatorT, class DeleterT>
    std::unique_ptr<CommandListT, DeleterT> PoolCommandListAllocator::AllocateCommandList(Pool<void>& pool, std::vector<CommandListT*>& cmdLists, std::vector<CommandAllocatorT*>& cmdAllocators)
    {
        Pool<void>::SlotType slot = pool.Allocate();
        auto index = slot.MemoryOffset;

        if (index >= cmdLists.size())
        {
            CommandAllocatorT* allocator = new CommandAllocatorT{ *mDevice };
            cmdAllocators.emplace_back(allocator);
            cmdLists.emplace_back(new CommandListT{ *mDevice, allocator });
        }

        auto deleter = [this, slot, &pool, cmdList = cmdLists[index], cmdAllocator = cmdAllocators[index]](CommandListT* cmdList)
        {
            mPendingDeallocations[mCurrentFrameIndex].emplace_back(Deallocation{ &pool, slot, cmdList });
        };

        return { cmdLists[index], deleter };
    }

    void PoolCommandListAllocator::ExecutePendingDealloactions(uint64_t frameIndex)
    {
        for (Deallocation& deallocation : mPendingDeallocations[frameIndex])
        {
            deallocation.PoolThatProducedAllocation->Deallocate(deallocation.Slot);
            deallocation.CommandList->Reset();
        }

        mPendingDeallocations[frameIndex].clear();
    }

}
