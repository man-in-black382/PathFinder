#pragma once

#include "RenderSurfaceDescription.hpp"
#include "GlobalRootConstants.hpp"
#include "PerFrameRootConstants.hpp"
#include "PipelineResourceSchedulingInfo.hpp"
#include "PipelineResourceMemoryAliaser.hpp"
#include "PipelineResourceStateOptimizer.hpp"
#include "PipelineResourceStoragePass.hpp"
#include "PipelineResourceStorageResource.hpp"
#include "PipelineResourceSchedulingRequest.hpp"

#include "../HardwareAbstractionLayer/DescriptorHeap.hpp"
#include "../HardwareAbstractionLayer/SwapChain.hpp"
#include "../Foundation/MemoryUtils.hpp"
#include "../Memory/GPUResourceProducer.hpp"
#include "../Memory/PoolDescriptorAllocator.hpp"
#include "../Memory/ResourceStateTracker.hpp"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <tuple>
#include <memory>
#include <optional>

#include <dtl/dtl.hpp>

namespace PathFinder
{

    class RenderPassExecutionGraph;

    using ResourceName = Foundation::Name;
    using PassName = Foundation::Name;

    class PipelineResourceStorage
    {
    public:
        PipelineResourceStorage(
            HAL::Device* device,
            Memory::GPUResourceProducer* resourceProducer,
            Memory::PoolDescriptorAllocator* descriptorAllocator,
            Memory::ResourceStateTracker* stateTracker,
            const RenderSurfaceDescription& defaultRenderSurface,
            const RenderPassExecutionGraph* passExecutionGraph
        );

        using DebugBufferIteratorFunc = std::function<void(PassName passName, const float* debugData)>;
        using DelayedSchedulingAction = std::function<void()>;

        const HAL::RTDescriptor* GetRenderTargetDescriptor(Foundation::Name resourceName, uint64_t resourceIndex = 0, uint64_t mipIndex = 0);
        const HAL::DSDescriptor* GetDepthStencilDescriptor(Foundation::Name resourceName, uint64_t resourceIndex = 0);
        
        void SetCurrentRenderPassGraphNode(const RenderPassExecutionGraph::Node& node);
        void CommitRenderPasses();
        void StartResourceScheduling();
        void EndResourceScheduling();
        void RequestResourceTransitionsToCurrentPassStates();
        void RequestCurrentPassDebugReadback();
        void AllowCurrentPassConstantBufferSingleOffsetAdvancement();
        
        template <class Constants> 
        void UpdateGlobalRootConstants(const Constants& constants);

        template <class Constants>
        void UpdateFrameRootConstants(const Constants& constants);

        template <class Constants>
        void UpdateCurrentPassRootConstants(const Constants& constants);

        const Memory::Buffer* GlobalRootConstantsBuffer() const;
        const Memory::Buffer* PerFrameRootConstantsBuffer() const;
        const Memory::Buffer* DebugBufferForCurrentPass() const;
        HAL::GPUAddress RootConstantsBufferAddressForCurrentPass() const;
        const HAL::ResourceBarrierCollection& AliasingBarriersForCurrentPass();
        const HAL::ResourceBarrierCollection& UnorderedAccessBarriersForCurrentPass();
        const RenderPassExecutionGraph::Node& CurrentPassGraphNode() const;

        void AddResourceCreationAction(const DelayedSchedulingAction& action, ResourceName resourceName, PassName passName);
        void AddResourceUsageAction(const DelayedSchedulingAction& action);

        PipelineResourceStoragePass* GetPerPassData(PassName name);
        PipelineResourceStorageResource* GetPerResourceData(ResourceName name);
        const PipelineResourceStoragePass* GetPerPassData(PassName name) const;
        const PipelineResourceStorageResource* GetPerResourceData(ResourceName name) const;

        void IterateDebugBuffers(const DebugBufferIteratorFunc& func) const;

        PipelineResourceStorageResource& QueueTexturesAllocationIfNeeded(
            ResourceName resourceName,
            HAL::ResourceFormat::FormatVariant format,
            HAL::TextureKind kind,
            const Geometry::Dimensions& dimensions,
            const HAL::ClearValue& optimizedClearValue,
            uint16_t mipCount,
            uint64_t textureCount
        );

        template <class BufferDataT>
        PipelineResourceStorageResource& QueueBuffersAllocationIfNeeded(
            ResourceName resourceName,
            uint64_t capacity,
            uint64_t perElementAlignment,
            uint64_t buffersCount
        );

    private:
        using ResourceMap = std::unordered_map<ResourceName, PipelineResourceStorageResource>;
        using DiffEntryList = std::vector<PipelineResourceStorageResource::DiffEntry>;

        PipelineResourceStoragePass& CreatePerPassData(PassName name);
        PipelineResourceStorageResource& CreatePerResourceData(ResourceName name, const HAL::ResourceFormat& resourceFormat, uint64_t resourceCount);

        void CreateDebugBuffers();
        bool TransferPreviousFrameResources();
        void CreateAliasingBarriers();
        void CreateUAVBarriers();
        void FinalizeSchedulingInfo();

        HAL::Device* mDevice;
        Memory::GPUResourceProducer* mResourceProducer;
        Memory::PoolDescriptorAllocator* mDescriptorAllocator;
        Memory::ResourceStateTracker* mResourceStateTracker;
        const RenderPassExecutionGraph* mPassExecutionGraph;

        std::unique_ptr<HAL::Heap> mRTDSHeap;
        std::unique_ptr<HAL::Heap> mNonRTDSHeap;
        std::unique_ptr<HAL::Heap> mBufferHeap;
        std::unique_ptr<HAL::Heap> mUniversalHeap;

        RenderSurfaceDescription mDefaultRenderSurface;

        PipelineResourceStateOptimizer mStateOptimizer;

        PipelineResourceMemoryAliaser mRTDSMemoryAliaser;
        PipelineResourceMemoryAliaser mNonRTDSMemoryAliaser;
        PipelineResourceMemoryAliaser mBufferMemoryAliaser;
        PipelineResourceMemoryAliaser mUniversalMemoryAliaser;

        // This class' logic works with 'the current' render pass.
        // Saves the user from passing current pass name in every possible API.
        RenderPassExecutionGraph::Node mCurrentRenderPassGraphNode;

        // Constant buffer for global data that changes rarely
        Memory::GPUResourceProducer::BufferPtr mGlobalRootConstantsBuffer;

        // Constant buffer for data that changes every frame
        Memory::GPUResourceProducer::BufferPtr mPerFrameRootConstantsBuffer;

        std::unordered_map<PassName, PipelineResourceStoragePass> mPerPassData;
        PipelineResourceStoragePass* mCurrentPassData = nullptr;

        // Two sets of resources: current and previous frame
        std::pair<ResourceMap, ResourceMap> mPerResourceData;
        ResourceMap* mPreviousFrameResources = &mPerResourceData.first;
        ResourceMap* mCurrentFrameResources = &mPerResourceData.second;

        // Resource diff entries to determine resource allocation needs
        std::pair<DiffEntryList, DiffEntryList> mDiffEntries;
        DiffEntryList* mPreviousFrameDiffEntries = &mDiffEntries.first;
        DiffEntryList* mCurrentFrameDiffEntries = &mDiffEntries.second;

        // Prepared callbacks to be called after all passes scheduled their resources
        std::vector<DelayedSchedulingAction> mResourceCreationRequests;
        std::vector<DelayedSchedulingAction> mResourceUsageRequests;

        // Keeps track of resource allocation requests to detect duplicates
        std::unordered_map<Foundation::Name, Foundation::Name> mResourceCreationRequestTracker;

        // Transitions for resources scheduled for readback
        HAL::ResourceBarrierCollection mReadbackBarriers;
    };

}

#include "PipelineResourceStorage.inl"