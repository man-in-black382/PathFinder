#include "PoolCommandListAllocator.hpp"

namespace Memory
{

    PoolCommandListAllocator::PoolCommandListAllocator(const HAL::Device* device, uint8_t simultaneousFramesInFlight)
        : mDevice{ device }, mRingFrameTracker{ simultaneousFramesInFlight }, mGraphicsPool{ 1, 1 }, mComputePool{ 1, 1 }, mCopyPool{ 1, 1 }
    {
        mRingFrameTracker.SetDeallocationCallback([this](const Ring::FrameTailAttributes& frameAttributes)
        {
            auto frameIndex = frameAttributes.Tail - frameAttributes.Size;
            ExecutePendingDeallocations(frameIndex);
        });

        mPendingDeallocations.resize(simultaneousFramesInFlight);
    }

    PoolCommandListAllocator::~PoolCommandListAllocator()
    {
        for (HAL::GraphicsCommandList* cmdList : mGraphicCommandLists) delete cmdList;
        for (HAL::ComputeCommandList* cmdList : mComputeCommandLists) delete cmdList;
        for (HAL::CopyCommandList* cmdList : mCopyCommandLists) delete cmdList;
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
        return AllocateCommandList<HAL::GraphicsCommandList, std::function<void(HAL::GraphicsCommandList*)>>(mGraphicsPool, mGraphicCommandLists);
    }

    PoolCommandListAllocator::ComputeCommandListPtr PoolCommandListAllocator::AllocateComputeCommandList()
    {
        return AllocateCommandList<HAL::ComputeCommandList, std::function<void(HAL::ComputeCommandList*)>>(mComputePool, mComputeCommandLists);
    }

    PoolCommandListAllocator::CopyCommandListPtr PoolCommandListAllocator::AllocateCopyCommandList()
    {
        return AllocateCommandList<HAL::CopyCommandList, std::function<void(HAL::CopyCommandList*)>>(mCopyPool, mCopyCommandLists);
    }

    void PoolCommandListAllocator::ExecutePendingDeallocations(uint64_t frameIndex)
    {
        for (Deallocation& deallocation : mPendingDeallocations[frameIndex])
        {
            deallocation.CommandList->Reset();
            deallocation.PoolThatProducedAllocation->Deallocate(deallocation.Slot);
        }

        mPendingDeallocations[frameIndex].clear();
    }

}
