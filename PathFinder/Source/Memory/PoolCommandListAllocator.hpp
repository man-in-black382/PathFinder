#pragma once

#include "../HardwareAbstractionLayer/Device.hpp"
#include "../HardwareAbstractionLayer/CommandList.hpp"

#include "Ring.hpp"
#include "Pool.hpp"

#include <tuple>
#include <vector>
#include <memory>

namespace Memory
{
   
    class PoolCommandListAllocator
    {
    public:
        PoolCommandListAllocator(const HAL::Device* device, uint8_t simultaneousFramesInFlight);

        void BeginFrame(uint64_t frameNumber);
        void EndFrame(uint64_t frameNumber);

        std::unique_ptr<HAL::GraphicsCommandList> AllocateGraphicsCommandList();
        std::unique_ptr<HAL::ComputeCommandList> AllocateComputeCommandList();
        std::unique_ptr<HAL::CopyCommandList> AllocateCopyCommandList();

    private:
        struct Deallocation
        {
            Pool<>* PoolThatProducedAllocation;
            Pool<>::Slot Slot;
            HAL::CommandList* CommandList;
            HAL::CommandAllocator* CommandAllocator;
        };

        void ExecutePendingDealloactions(uint64_t frameIndex);

        template <class CommandListT, class CommandAllocatorT>
        std::unique_ptr<CommandListT> AllocateCommandList(Pool<>& pool, std::vector<CommandListT>& cmdLists, std::vector<CommandAllocatorT>& cmdAllocators);

        const HAL::Device* mDevice = nullptr;
        Ring mRingFrameTracker;
        uint8_t mCurrentFrameIndex = 0;

        Pool<> mGraphicsPool;
        Pool<> mComputePool;
        Pool<> mCopyPool;

        std::vector<HAL::GraphicsCommandList> mGraphicCommandLists;
        std::vector<HAL::ComputeCommandList> mComputeCommandLists;
        std::vector<HAL::CopyCommandList> mCopyCommandLists;

        std::vector<HAL::GraphicsCommandAllocator> mGraphicCommandAllocators;
        std::vector<HAL::ComputeCommandAllocator> mComputeCommandAllocators;
        std::vector<HAL::CopyCommandAllocator> mCopyCommandAllocators;

        std::vector<std::vector<Deallocation>> mPendingDeallocations;
    };

}

