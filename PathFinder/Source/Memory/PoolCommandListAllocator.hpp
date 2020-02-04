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
        using GraphicsCommandListPtr = std::unique_ptr<HAL::GraphicsCommandList, std::function<void(HAL::GraphicsCommandList*)>>;
        using ComputeCommandListPtr = std::unique_ptr<HAL::ComputeCommandList, std::function<void(HAL::ComputeCommandList*)>>;
        using CopyCommandListPtr = std::unique_ptr<HAL::CopyCommandList, std::function<void(HAL::CopyCommandList*)>>;

        PoolCommandListAllocator(const HAL::Device* device, uint8_t simultaneousFramesInFlight);
        ~PoolCommandListAllocator();

        void BeginFrame(uint64_t frameNumber);
        void EndFrame(uint64_t frameNumber);

        GraphicsCommandListPtr AllocateGraphicsCommandList();
        ComputeCommandListPtr AllocateComputeCommandList();
        CopyCommandListPtr AllocateCopyCommandList();

    private:
        struct Deallocation
        {
            Pool<void>* PoolThatProducedAllocation;
            Pool<void>::SlotType Slot;
            HAL::CommandList* CommandList;
        };

        void ExecutePendingDealloactions(uint64_t frameIndex);

        template <class CommandListT, class CommandAllocatorT, class DeleterT>
        std::unique_ptr<CommandListT, DeleterT> AllocateCommandList(Pool<void>& pool, std::vector<CommandListT*>& cmdLists, std::vector<CommandAllocatorT*>& cmdAllocators);

        const HAL::Device* mDevice = nullptr;
        Ring mRingFrameTracker;
        uint8_t mCurrentFrameIndex = 0;

        Pool<void> mGraphicsPool;
        Pool<void> mComputePool;
        Pool<void> mCopyPool;

        std::vector<HAL::GraphicsCommandList*> mGraphicCommandLists;
        std::vector<HAL::ComputeCommandList*> mComputeCommandLists;
        std::vector<HAL::CopyCommandList*> mCopyCommandLists;

        std::vector<HAL::GraphicsCommandAllocator*> mGraphicCommandAllocators;
        std::vector<HAL::ComputeCommandAllocator*> mComputeCommandAllocators;
        std::vector<HAL::CopyCommandAllocator*> mCopyCommandAllocators;

        std::vector<std::vector<Deallocation>> mPendingDeallocations;
    };

}

