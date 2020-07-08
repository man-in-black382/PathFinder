#include "PoolCommandListAllocator.hpp"

namespace Memory
{

    PoolCommandListAllocator::PoolCommandListAllocator(const HAL::Device* device, uint8_t simultaneousFramesInFlight)
        : mDevice{ device }, mRingFrameTracker{ simultaneousFramesInFlight }, mSimultaneousFramesInFlight{simultaneousFramesInFlight}
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
       /* for (HAL::GraphicsCommandList* cmdList : mGraphicCommandLists) delete cmdList;
        for (HAL::ComputeCommandList* cmdList : mComputeCommandLists) delete cmdList;
        for (HAL::CopyCommandList* cmdList : mCopyCommandLists) delete cmdList;*/
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
  
    PoolCommandListAllocator::GraphicsCommandListPtr PoolCommandListAllocator::AllocateGraphicsCommandList(uint64_t threadIndex)
    {
        PreallocateThreadObjectsIfNeeded(threadIndex);
        return AllocateCommandList<HAL::GraphicsCommandList, HAL::GraphicsCommandAllocator, std::function<void(HAL::GraphicsCommandList*)>>(
            mPerThreadObjects[threadIndex]->GraphicsCommandListPackagePool,
            mPerThreadObjects[threadIndex]->GraphicsCommandListPackages,
            mPerThreadObjects[threadIndex]->CurrentGraphicsPackageIndex,
            threadIndex,
            CommandListPackageType::Graphics);
    }

    PoolCommandListAllocator::ComputeCommandListPtr PoolCommandListAllocator::AllocateComputeCommandList(uint64_t threadIndex)
    {
        PreallocateThreadObjectsIfNeeded(threadIndex);
        return AllocateCommandList<HAL::ComputeCommandList, HAL::ComputeCommandAllocator, std::function<void(HAL::ComputeCommandList*)>>(
            mPerThreadObjects[threadIndex]->ComputeCommandListPackagePool,
            mPerThreadObjects[threadIndex]->ComputeCommandListPackages,
            mPerThreadObjects[threadIndex]->CurrentComputePackageIndex,
            threadIndex,
            CommandListPackageType::Compute);
    }

    PoolCommandListAllocator::CopyCommandListPtr PoolCommandListAllocator::AllocateCopyCommandList(uint64_t threadIndex)
    {
        PreallocateThreadObjectsIfNeeded(threadIndex);
        return AllocateCommandList<HAL::CopyCommandList, HAL::CopyCommandAllocator, std::function<void(HAL::CopyCommandList*)>>(
            mPerThreadObjects[threadIndex]->CopyCommandListPackagePool,
            mPerThreadObjects[threadIndex]->CopyCommandListPackages,
            mPerThreadObjects[threadIndex]->CurrentCopyPackageIndex,
            threadIndex,
            CommandListPackageType::Copy);
    }

    void PoolCommandListAllocator::PreallocateThreadObjectsIfNeeded(uint64_t threadIndex)
    {
        int64_t newThreadsCount = threadIndex + 1 - (int64_t)mPerThreadObjects.size();

        for (auto i = 0; i < newThreadsCount; ++i)
        {
            mPerThreadObjects.emplace_back(std::make_unique<ThreadObjects>());
        }
    }

    void PoolCommandListAllocator::ExecutePendingDeallocations(uint64_t frameIndex)
    {
        for (std::unique_ptr<ThreadObjects>& threadObjects : mPerThreadObjects)
        {
            if (!threadObjects->GraphicsCommandListPackages.empty())
                threadObjects->GraphicsCommandListPackages[frameIndex].CommandAllocator->Reset();

            if (!threadObjects->ComputeCommandListPackages.empty())
                threadObjects->ComputeCommandListPackages[frameIndex].CommandAllocator->Reset();

            if (!threadObjects->CopyCommandListPackages.empty())
                threadObjects->CopyCommandListPackages[frameIndex].CommandAllocator->Reset();
        }

        for (Deallocation& deallocation : mPendingDeallocations[frameIndex])
        {
            switch (deallocation.PackageType)
            {
            case CommandListPackageType::Graphics: mPerThreadObjects[deallocation.ThreadIndex]->GraphicsCommandListPackagePool.Deallocate(deallocation.Slot); break;
            case CommandListPackageType::Compute: mPerThreadObjects[deallocation.ThreadIndex]->ComputeCommandListPackagePool.Deallocate(deallocation.Slot); break;
            case CommandListPackageType::Copy: mPerThreadObjects[deallocation.ThreadIndex]->CopyCommandListPackagePool.Deallocate(deallocation.Slot); break;
            default: break;
            }
        }

        mPendingDeallocations[frameIndex].clear();
    }

}
