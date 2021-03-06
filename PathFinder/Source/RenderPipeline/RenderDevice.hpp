#pragma once

#include "RenderSurfaceDescription.hpp"
#include "PipelineResourceStorage.hpp"
#include "PipelineStateManager.hpp"
#include "RenderPassMetadata.hpp"
#include "GPUProfiler.hpp"
#include "PipelineSettings.hpp"

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
#include <forward_list>

#include "DrawablePrimitive.hpp"

namespace PathFinder
{

    class RenderDevice
    {
    public:
        using GraphicsCommandListPtr = Memory::PoolCommandListAllocator::GraphicsCommandListPtr;
        using ComputeCommandListPtr = Memory::PoolCommandListAllocator::ComputeCommandListPtr;
        using CommandListPtrVariant = std::variant<GraphicsCommandListPtr, ComputeCommandListPtr>;
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
            CommandListPtrVariant PreWorkCommandList = GraphicsCommandListPtr{ nullptr };

            // A command list render commands are recorded into
            CommandListPtrVariant WorkCommandList = GraphicsCommandListPtr{ nullptr };

            // An optional command list that may be required to execute Begin barriers 
            // or back buffer transition at the end of the frame
            CommandListPtrVariant PostWorkCommandList = GraphicsCommandListPtr{ nullptr };
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
            const RenderSurfaceDescription& defaultRenderSurface,
            const PipelineSettings* settings
        );

        PassCommandLists& CommandListsForPass(const RenderPassGraph::Node& node);
        PassHelpers& PassHelpersForPass(const RenderPassGraph::Node& node);

        HAL::ComputeCommandListBase* GetComputeCommandListBase(CommandListPtrVariant& variant) const;

        void SetBackBuffer(Memory::Texture* backBuffer);
        const Memory::Texture* BackBuffer() const;

        void AllocateUploadCommandList();
        void AllocateRTASBuildsCommandList();
        void PrepareForGraphExecution();

        void ExecuteRenderGraph();
        void GatherMeasurements();

        template <class Lambda>
        void RecordWorkerCommandList(const RenderPassGraph::Node& passNode, const Lambda& action);

    private:
        // Helper data structure that manages fences and holds command lists. 
        // Converted into API calls after render pass work and rerouted transitions are determined and placed.
        class FrameBlueprint
        {
        public:
            struct Signal
            {
                HAL::Fence* Fence = nullptr;
                uint64_t FenceValue = 0;
                std::string SignalName;
            };

            struct Wait
            {
                std::vector<const Signal*> SignalsToWait;
                std::vector<std::string> EventNamesToWait;
            };

            struct RenderPassEvent
            {
                PassCommandLists CommandLists;
                std::optional<Wait> WaitEvent;
                std::optional<Signal> SignalEvent;
                uint64_t EstimatedBatchIndex = 0;
            };

            struct ReroutedTransitionsEvent
            {
                CommandListPtrVariant CommandList;
                Signal SignalEvent;
                Wait WaitEvent;
            };

            using Event = std::variant<RenderPassEvent, ReroutedTransitionsEvent>;
            using EventList = std::list<Event>;
            using EventIt = EventList::iterator;
            using BlueprintTraverser = std::function<void(uint64_t queueIndex, Event& frameEvent)>;

            FrameBlueprint(const RenderPassGraph* graph, uint64_t bvhBuildQueueIndex, HAL::Fence* bvhFence, const std::vector<HAL::Fence*>& queueFences);

            void Build();
            void PropagateFenceUpdates();

            ReroutedTransitionsEvent& InsertReroutedTransitionsEvent(
                std::optional<uint64_t> afterDependencyLevel,
                uint64_t waitingDependencyLevel, 
                uint64_t queueIndex, 
                const std::vector<uint64_t>& queuesToSyncWith);

            void Traverse(const BlueprintTraverser& traverser);

            const RenderPassEvent& GetRenderPassEvent(const RenderPassGraph::Node& node) const;
            RenderPassEvent& GetRenderPassEvent(const RenderPassGraph::Node& node);

        private:
            // Unique ptr to circumvent std::vector chosing copy ctor of std::list instead of the move one.
            std::vector<std::unique_ptr<EventList>> mEventsPerQueue;
            std::vector<uint64_t> mCurrentBatchIndices;
            std::vector<std::vector<EventIt>> mRenderPassEventRefs;
            std::vector<std::vector<EventIt>> mReroutedTransitionEventRefs;

            const RenderPassGraph* mPassGraph = nullptr;
            uint64_t mBVHBuildQueueIndex;
            Signal mBVHBuildSignal;
            std::vector<HAL::Fence*> mQueueFences;
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
            using DependencyLevelIndex = uint64_t;
            using ResourceUser = std::variant<const RenderPassGraph::Node*, DependencyLevelIndex>;
            
            // Previous usage can indicate usage in a single render pass in a dependency level
            // or a whole dependency level when we're dealing with rerouted transitions
            ResourceUser User;
            uint64_t EstimatedCommandListBatchIndex = 0;
        };

        struct ResourceReadbackInfo
        {
            std::vector<Memory::CopyRequestManager::CopyCommand> CopyCommands;
            HAL::ResourceBarrierCollection ToCopyStateTransitions;
        };

        void RecordNonWorkerCommandLists();
        void TraverseAndExecuteFrameBlueprint();
        void UploadPassConstants();

        void GatherResourceTransitionKnowledge(const RenderPassGraph::DependencyLevel& dependencyLevel);
        void AllocateAndRecordPreWorkCommandList(const RenderPassGraph::Node& node, const HAL::ResourceBarrierCollection& barriers, const std::string& cmdListName);
        void AllocateAndRecordReroutedTransitionsCommandList(std::optional<uint64_t> reroutingDependencyLevelIndex, uint64_t currentDependencyLevelIndex, const HAL::ResourceBarrierCollection& barriers);
        void CollectNodeStandardTransitions(const RenderPassGraph::Node* node, uint64_t currentCommandListBatchIndex, HAL::ResourceBarrierCollection& collection);
        void CollectNodeTransitionsToReroute(std::optional<uint64_t>& reroutingDependencyLevelIndex, HAL::ResourceBarrierCollection& collection, const RenderPassGraph::DependencyLevel& currentDL);
        void CollectNodeUAVAndAliasingBarriers(const RenderPassGraph::Node& node, HAL::ResourceBarrierCollection& collection);
        void RecordResourceTransitions(const RenderPassGraph::DependencyLevel& dependencyLevel);
        void RecordPostWorkCommandLists();
        void ExecuteUploadCommands();
        void ExecuteBVHBuildCommands();

        bool IsStateTransitionSupportedOnQueue(uint64_t queueIndex, HAL::ResourceState beforeState, HAL::ResourceState afterState) const;
        bool IsStateTransitionSupportedOnQueue(uint64_t queueIndex, HAL::ResourceState afterState) const;
        HAL::CommandQueue& GetCommandQueue(uint64_t queueIndex);
        uint64_t FindMostCompetentQueueIndex(const robin_hood::unordered_flat_set<RenderPassGraph::Node::QueueIndex>& queueIndices) const;
        uint64_t FindQueueSupportingTransition(HAL::ResourceState beforeStates, HAL::ResourceState afterStates) const;
        CommandListPtrVariant AllocateCommandListForQueue(uint64_t queueIndex) const;
        bool IsNullCommandList(CommandListPtrVariant& variant) const;
        HAL::Fence& FenceForQueueIndex(uint64_t index);

        template <class CommandQueueT, class CommandListT>
        void ExecuteCommandListBatch(std::vector<CommandListPtrVariant>& batch, HAL::CommandQueue& queue);

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
        const PipelineSettings* mPipelinesSettings;

        Memory::Texture* mBackBuffer = nullptr;
        Memory::PoolCommandListAllocator::GraphicsCommandListPtr mPreRenderUploadsCommandList;
        Memory::PoolCommandListAllocator::ComputeCommandListPtr mRTASBuildsCommandList;
        std::vector<PassHelpers> mPassHelpers;
        HAL::GraphicsCommandQueue mGraphicsQueue;
        HAL::ComputeCommandQueue mComputeQueue;

        HAL::Fence mGraphicsQueueFence;
        HAL::Fence mComputeQueueFence;
        HAL::Fence mBVHFence;

        uint64_t mQueueCount = 2;
        uint64_t mBVHBuildsQueueIndex = 1;

        FrameBlueprint mFrameBlueprint;

        // Keep track of nodes where transitions previously occurred (where resource was used last) to insert Begin part of split barriers there
        robin_hood::unordered_flat_map<RenderPassGraph::SubresourceName, SubresourcePreviousUsageInfo> mSubresourcesPreviousUsageInfo;

        // Keep list of separate barriers gathered for dependency level so we could cull them, if conditions are met, when command list batches are determined
        std::vector<std::vector<SubresourceTransitionInfo>> mDependencyLevelStandardTransitions;

        // Keep list of transitions in the current dependency level that need to be rerouted 
        std::vector<SubresourceTransitionInfo> mDependencyLevelTransitionsToReroute;

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
        std::vector<PipelineMeasurement> mPassMeasurements;
        PipelineMeasurement mFrameMeasurement;

    public:
        inline HAL::GraphicsCommandQueue& GraphicsCommandQueue() { return mGraphicsQueue; }
        inline HAL::ComputeCommandQueue& ComputeCommandQueue() { return mComputeQueue; }
        inline HAL::GraphicsCommandList* PreRenderUploadsCommandList() { return mPreRenderUploadsCommandList.get(); }
        inline HAL::ComputeCommandList* RTASBuildsCommandList() { return mRTASBuildsCommandList.get(); }
        inline const RenderSurfaceDescription& DefaultRenderSurfaceDesc() { return mDefaultRenderSurface; }
        inline const auto& RenderPassMeasurements() const { return mPassMeasurements; }
        inline const PipelineMeasurement& FrameMeasurement() const { return mFrameMeasurement; }
    };

}

#include "RenderDevice.inl"