#pragma once

#include "RenderSurfaceDescription.hpp"
#include "PipelineResourceStorage.hpp"
#include "PipelineStateManager.hpp"
#include "RenderPassMetadata.hpp"
#include "GPUProfiler.hpp"

#include <Foundation/Name.hpp>
#include <Utility/EventTracker.hpp>
#include <Geometry/Dimensions.hpp>

#include <HardwareAbstractionLayer/ShaderRegister.hpp>
#include <HardwareAbstractionLayer/Resource.hpp>
#include <HardwareAbstractionLayer/CommandQueue.hpp>
#include <HardwareAbstractionLayer/RayTracingAccelerationStructure.hpp>
#include <HardwareAbstractionLayer/Viewport.hpp>
#include <HardwareAbstractionLayer/RenderTarget.hpp>

#include <Memory/PoolCommandListAllocator.hpp>
#include <Memory/PoolDescriptorAllocator.hpp>
#include <Memory/GPUResource.hpp>
#include <Memory/ResourceStateTracker.hpp>
#include <Memory/CopyRequestManager.hpp>

#include <robinhood/robin_hood.h>

#include "DrawablePrimitive.hpp"

namespace PathFinder
{

    class RenderDevice
    {
    public:
        using GraphicsCommandListPtr = Memory::PoolCommandListAllocator::GraphicsCommandListPtr;
        using ComputeCommandListPtr = Memory::PoolCommandListAllocator::ComputeCommandListPtr;
        using CommandListPtrVariant = std::variant<GraphicsCommandListPtr, ComputeCommandListPtr>;
        using HALCommandListPtrVariant = std::variant<HAL::GraphicsCommandList*, HAL::ComputeCommandList*>;
        using FenceAndValue = std::pair<const HAL::Fence*, uint64_t>;
        
        struct PipelineMeasurement
        {
            std::string Name;
            float DurationSeconds;
            GPUProfiler::EventID ProfilerEventID;
        };

        struct PassCommandLists
        {
            // A command list to execute transition barriers before render pass work.
            // Separated from work so we could perform transitions in separate thread.
            CommandListPtrVariant TransitionsCommandList = GraphicsCommandListPtr{ nullptr };

            // A command list render commands are recorded into
            CommandListPtrVariant WorkCommandList = GraphicsCommandListPtr{ nullptr };

            // An optional command list that may be required to execute Begin barriers 
            // or back buffer transition at the end of the frame
            CommandListPtrVariant PostWorkCommandList = GraphicsCommandListPtr{ nullptr };

            // Index of a batch these command lists belong to.
            uint64_t CommandListBatchIndex = 0;
        };

        struct PassHelpers
        {
            // Keep a set of UAV barriers to be inserted between draws/dispatches.
            // UAV barriers to be inserted between render passes (if needed) are handled separately.
            HAL::ResourceBarrierCollection UAVBarriers;

            // Storage for pass constant buffer and its information
            PipelineResourceStoragePass* ResourceStoragePassData = nullptr;

            // Helpers for correct bindings and sanity checks
            const HAL::RayDispatchInfo* LastAppliedRTStateDispatchInfo = nullptr;
            std::optional<HAL::Viewport> LastAppliedViewport;
            std::optional<Geometry::Rect2D> LastAppliedScissor;
            uint64_t ExecutedRenderCommandsCount = 0;
            const HAL::RootSignature* LastSetRootSignature = nullptr;
            HAL::GPUAddress LastBoundRootConstantBufferAddress = 0;
            std::optional<PipelineStateManager::PipelineStateVariant> LastSetPipelineState;
        };

        RenderDevice(
            const HAL::Device& device,
            Memory::PoolDescriptorAllocator* descriptorAllocator,
            Memory::PoolCommandListAllocator* commandListAllocator,
            Memory::ResourceStateTracker* resourceStateTracker,
            Memory::CopyRequestManager* copyRequestManager,
            PipelineResourceStorage* resourceStorage,
            PipelineStateManager* pipelineStateManager,
            GPUProfiler* gpuProfiler,
            const RenderPassGraph* renderPassGraph,
            const RenderSurfaceDescription& defaultRenderSurface
        );

        PassCommandLists& CommandListsForNode(const RenderPassGraph::Node& node);
        PassHelpers& PassHelpersForNode(const RenderPassGraph::Node& node);

        HAL::ComputeCommandListBase* GetComputeCommandListBase(CommandListPtrVariant& variant) const;
        HAL::ComputeCommandListBase* GetComputeCommandListBase(HALCommandListPtrVariant& variant) const;
        HALCommandListPtrVariant GetHALCommandListVariant(CommandListPtrVariant& variant) const;

        void SetBackBuffer(Memory::Texture* backBuffer);
        const Memory::Texture* BackBuffer() const;

        void AllocateUploadCommandList();
        void AllocateRTASBuildsCommandList();
        void AllocateWorkerCommandLists();

        void ExecuteRenderGraph();
        void GatherMeasurements();

        template <class Lambda>
        void RecordWorkerCommandList(const RenderPassGraph::Node& passNode, const Lambda& action);

    private:
        struct CommandListBatch
        {
            bool IsEmpty = true;
            std::vector<HALCommandListPtrVariant> CommandLists;
            std::vector<FenceAndValue> FencesToWait;
            FenceAndValue FenceToSignal;

            // Debug info
            std::vector<std::optional<std::string>> CommandListNames;
            std::vector<std::string> EventNamesToWait;
            std::string SignalName;
        };

        struct SubresourceTransitionInfo
        {
            RenderPassGraph::SubresourceName SubresourceName;
            // Optional, because we also track "omitted" transitions to maintain split barrier correctness
            std::optional<HAL::ResourceTransitionBarrier> TransitionBarrier;
            const HAL::Resource* Resource = nullptr;
        };

        struct SubresourcePreviousUsageInfo
        {
            const RenderPassGraph::Node* Node = nullptr;
            uint64_t CommandListBatchIndex = 0;
        };

        struct ResourceReadbackInfo
        {
            std::vector<Memory::CopyRequestManager::CopyCommand> CopyCommands;
            HAL::ResourceBarrierCollection ToCopyStateTransitions;
        };

        void BatchCommandLists();
        void ExetuteCommandLists();
        void UploadPassConstants();

        void GatherResourceTransitionKnowledge(const RenderPassGraph::DependencyLevel& dependencyLevel);
        void CollectNodeTransitions(const RenderPassGraph::Node* node, uint64_t currentCommandListBatchIndex, HAL::ResourceBarrierCollection& collection);
        void CreateBatchesWithTransitionRerouting(const RenderPassGraph::DependencyLevel& dependencyLevel);
        void CreateBatchesWithoutTransitionRerouting(const RenderPassGraph::DependencyLevel& dependencyLevel);
        void RecordPostWorkCommandLists();
        void InsertCommandListsIntoCorrespondingBatches();
        void ExecuteUploadCommands();
        void ExecuteBVHBuildCommands();

        bool IsStateTransitionSupportedOnQueue(uint64_t queueIndex, HAL::ResourceState beforeState, HAL::ResourceState afterState) const;
        bool IsStateTransitionSupportedOnQueue(uint64_t queueIndex, HAL::ResourceState afterState) const;
        HAL::CommandQueue& GetCommandQueue(uint64_t queueIndex);
        uint64_t FindMostCompetentQueueIndex(const robin_hood::unordered_flat_set<RenderPassGraph::Node::QueueIndex>& queueIndices) const;
        uint64_t FindQueueSupportingTransition(HAL::ResourceState beforeStates, HAL::ResourceState afterStates) const;
        CommandListPtrVariant AllocateCommandListForQueue(uint64_t queueIndex) const;
        bool IsNullCommandList(HALCommandListPtrVariant& variant) const;
        HAL::Fence& FenceForQueueIndex(uint64_t index);

        template <class CommandQueueT, class CommandListT>
        void ExecuteCommandListBatch(CommandListBatch& batch, HAL::CommandQueue& queue);

        Memory::PoolDescriptorAllocator* mDescriptorAllocator;
        Memory::PoolCommandListAllocator* mCommandListAllocator;
        Memory::ResourceStateTracker* mResourceStateTracker;
        Memory::CopyRequestManager* mCopyRequestManager;
        PipelineResourceStorage* mResourceStorage;
        PipelineStateManager* mPipelineStateManager;
        GPUProfiler* mGPUProfiler;
        const RenderPassGraph* mRenderPassGraph;
        RenderSurfaceDescription mDefaultRenderSurface;
        EventTracker mEventTracker;

        Memory::Texture* mBackBuffer = nullptr;
        Memory::PoolCommandListAllocator::GraphicsCommandListPtr mPreRenderUploadsCommandList;
        Memory::PoolCommandListAllocator::ComputeCommandListPtr mRTASBuildsCommandList;
        std::vector<PassCommandLists> mPassCommandLists;
        std::vector<CommandListPtrVariant> mReroutedTransitionsCommandLists;
        std::vector<std::vector<CommandListBatch>> mCommandListBatches;
        std::vector<PassHelpers> mPassHelpers;
        HAL::GraphicsCommandQueue mGraphicsQueue;
        HAL::ComputeCommandQueue mComputeQueue;

        HAL::Fence mGraphicsQueueFence;
        HAL::Fence mComputeQueueFence;
        HAL::Fence mBVHFence;
        uint64_t mQueueCount = 2;
        uint64_t mBVHBuildsQueueIndex = 1;

        // Keep track of nodes where transitions previously occurred (where resource was used last) to insert Begin part of split barriers there
        robin_hood::unordered_flat_map<RenderPassGraph::SubresourceName, SubresourcePreviousUsageInfo> mSubresourcesPreviousUsageInfo;

        // Keep list of separate barriers gathered for dependency level so we could cull them, if conditions are met, when command list batches are determined
        std::vector<std::vector<SubresourceTransitionInfo>> mDependencyLevelTransitionBarriers;

        // UAV barriers to be applied between passes (not between draw/dispatch calls) when UAV->UAV usage is detected
        std::vector<HAL::ResourceBarrierCollection> mDependencyLevelInterpassUAVBarriers;

        // Keep track of queues inside a graph dependency layer that require transition rerouting
        robin_hood::unordered_flat_set<RenderPassGraph::Node::QueueIndex> mDependencyLevelQueuesThatRequireTransitionRerouting;

        // Collect begin barriers for passes that may issue them to be applied in batches after all nodes are processed
        std::vector<HAL::ResourceBarrierCollection> mPerNodeBeginBarriers;

        // Collect aliasing barriers for passes
        std::vector<HAL::ResourceBarrierCollection> mPerNodeAliasingBarriers;

        // Collect readback requests to be executed after passes that require them
        std::vector<ResourceReadbackInfo> mPerNodeReadbackInfo;

        // An hierarchy of various measured GPU events
        std::vector<PipelineMeasurement> mMeasurements;

    public:
        inline HAL::GraphicsCommandQueue& GraphicsCommandQueue() { return mGraphicsQueue; }
        inline HAL::ComputeCommandQueue& ComputeCommandQueue() { return mComputeQueue; }
        inline HAL::GraphicsCommandList* PreRenderUploadsCommandList() { return mPreRenderUploadsCommandList.get(); }
        inline HAL::ComputeCommandList* RTASBuildsCommandList() { return mRTASBuildsCommandList.get(); }
        inline const RenderSurfaceDescription& DefaultRenderSurfaceDesc() { return mDefaultRenderSurface; }
        inline const auto& Measurements() const { return mMeasurements; }
    };

}

#include "RenderDevice.inl"