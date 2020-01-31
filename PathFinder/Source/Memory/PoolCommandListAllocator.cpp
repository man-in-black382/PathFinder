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
  
    std::unique_ptr<HAL::GraphicsCommandList> PoolCommandListAllocator::AllocateGraphicsCommandList()
    {
        return AllocateCommandList(mGraphicsPool, mGraphicCommandLists, mGraphicCommandAllocators);
    }

    std::unique_ptr<HAL::ComputeCommandList> PoolCommandListAllocator::AllocateComputeCommandList()
    {
        return AllocateCommandList(mComputePool, mComputeCommandLists, mComputeCommandAllocators);
    }

    std::unique_ptr<HAL::CopyCommandList> PoolCommandListAllocator::AllocateCopyCommandList()
    {
        return AllocateCommandList(mCopyPool, mCopyCommandLists, mComputeCommandAllocators);
    }

    template <class CommandListT, class CommandAllocatorT>
    std::unique_ptr<CommandListT> PoolCommandListAllocator::AllocateCommandList(Pool<>& pool, std::vector<CommandListT>& cmdLists, std::vector<CommandAllocatorT>& cmdAllocators)
    {
        Pool<void>::Slot slot = pool.Allocate();
        auto index = slot.MemoryOffset;

        if (index >= cmdLists.size())
        {
            CommandAllocatorT& allocator = cmdAllocators.emplace_back(*mDevice);
            cmdLists.emplace_back(*mDevice, allocator);
        }

        auto deallocationCallback = [this, slot, cmdList = &cmdLists[index], cmdAllocator = &cmdAllocators[index]](CommandListT* cmdList)
        {
            mPendingDeallocations[mCurrentFrameIndex].emplace_back(&pool, slot, cmdList, cmdAllocator);
        };

        return std::make_unique<CommandListT>(&cmdLists[index], deallocationCallback);
    }

    void PoolCommandListAllocator::ExecutePendingDealloactions(uint64_t frameIndex)
    {
        for (Deallocation& deallocation : mPendingDeallocations[frameIndex])
        {
            deallocation.PoolThatProducedAllocation->Deallocate(deallocation.Slot);
            deallocation.CommandList->Reset(*deallocation.CommandAllocator);
        }

        mPendingDeallocations[frameIndex].clear();
    }

}
