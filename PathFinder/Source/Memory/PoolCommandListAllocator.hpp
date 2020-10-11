#pragma once

#include <HardwareAbstractionLayer/Device.hpp>
#include <HardwareAbstractionLayer/CommandList.hpp>

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
        template <class CommandListT>
        using CommandListPtr = std::unique_ptr<CommandListT, std::function<void(CommandListT*)>>;

        using GraphicsCommandListPtr = CommandListPtr<HAL::GraphicsCommandList>;
        using ComputeCommandListPtr = CommandListPtr<HAL::ComputeCommandList>;
        using CopyCommandListPtr = CommandListPtr<HAL::CopyCommandList>;

        PoolCommandListAllocator(const HAL::Device* device, uint8_t simultaneousFramesInFlight);
        ~PoolCommandListAllocator();

        void BeginFrame(uint64_t frameNumber);
        void EndFrame(uint64_t frameNumber);

        GraphicsCommandListPtr AllocateGraphicsCommandList(uint64_t threadIndex = 0);
        ComputeCommandListPtr AllocateComputeCommandList(uint64_t threadIndex = 0);
        CopyCommandListPtr AllocateCopyCommandList(uint64_t threadIndex = 0);

        template <class CommandListT> CommandListPtr<CommandListT> AllocateCommandList(uint64_t threadIndex = 0) = delete;
        template <> GraphicsCommandListPtr AllocateCommandList(uint64_t threadIndex);
        template <> ComputeCommandListPtr AllocateCommandList(uint64_t threadIndex);
        template <> CopyCommandListPtr AllocateCommandList(uint64_t threadIndex);

    private:
        template <class CommandListT, class CommandAllocatorT>
        struct CommandListPackage
        {
            CommandListPackage(const HAL::Device& device) 
                : CommandAllocator{ std::make_unique<CommandAllocatorT>(device) }, CommandListPool{ 1,1 } {}

            std::unique_ptr<CommandAllocatorT> CommandAllocator;
            Pool<void> CommandListPool;
            std::vector<CommandListT*> CommandLists;
        };

        using GraphicsCommandListPackage = CommandListPackage<HAL::GraphicsCommandList, HAL::GraphicsCommandAllocator>;
        using ComputeCommandListPackage = CommandListPackage<HAL::ComputeCommandList, HAL::ComputeCommandAllocator>;
        using CopyCommandListPackage = CommandListPackage<HAL::CopyCommandList, HAL::CopyCommandAllocator>;

        enum class CommandListPackageType
        {
            Graphics, Compute, Copy
        };

        struct Deallocation
        {
            Pool<void>::SlotType Slot;
            HAL::CommandList* CommandList;
            uint64_t ThreadIndex = 0;
            uint64_t PackageIndex = 0;
            CommandListPackageType PackageType;
        };

        struct ThreadObjects
        {
            std::vector<GraphicsCommandListPackage> GraphicsCommandListPackages;
            std::vector<ComputeCommandListPackage> ComputeCommandListPackages;
            std::vector<CopyCommandListPackage> CopyCommandListPackages;
        };

        void PreallocateThreadObjectsIfNeeded(uint64_t threadIndex);
        void ExecutePendingDeallocations(uint64_t frameIndex);

        template <class CommandListT, class CommandAllocatorT, class DeleterT>
        std::unique_ptr<CommandListT, DeleterT> AllocateCommandList(
            std::vector<CommandListPackage<CommandListT, CommandAllocatorT>>& packages,
            uint64_t threadIndex,
            CommandListPackageType packageType);

        const HAL::Device* mDevice = nullptr;
        Ring mRingFrameTracker;
        uint8_t mCurrentFrameIndex = 0;
        uint8_t mSimultaneousFramesInFlight = 1;

        std::vector<std::vector<Deallocation>> mPendingDeallocations;
        std::vector<std::unique_ptr<ThreadObjects>> mPerThreadObjects;
    };

}

#include "PoolCommandListAllocator.inl"