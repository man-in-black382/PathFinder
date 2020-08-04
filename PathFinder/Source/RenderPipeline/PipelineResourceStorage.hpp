#pragma once

#include "RenderSurfaceDescription.hpp"
#include "GlobalRootConstants.hpp"
#include "PerFrameRootConstants.hpp"
#include "PipelineResourceSchedulingInfo.hpp"
#include "PipelineResourceMemoryAliaser.hpp"
#include "PipelineResourceStoragePass.hpp"
#include "PipelineResourceStorageResource.hpp"

#include "../HardwareAbstractionLayer/DescriptorHeap.hpp"
#include "../HardwareAbstractionLayer/SwapChain.hpp"
#include "../Foundation/MemoryUtils.hpp"
#include "../Memory/GPUResourceProducer.hpp"
#include "../Memory/PoolDescriptorAllocator.hpp"
#include "../Memory/ResourceStateTracker.hpp"

#include <vector>
#include <functional>
#include <tuple>
#include <memory>
#include <optional>

#include <robinhood/robin_hood.h>
#include <dtl/dtl.hpp>

namespace PathFinder
{

    class RenderPassGraph;

    class PipelineResourceStorage
    {
    public:
        using ResourceName = Foundation::Name;
        using PassName = Foundation::Name;

        PipelineResourceStorage(
            HAL::Device* device,
            Memory::GPUResourceProducer* resourceProducer,
            Memory::PoolDescriptorAllocator* descriptorAllocator,
            Memory::ResourceStateTracker* stateTracker,
            const RenderSurfaceDescription& defaultRenderSurface,
            const RenderPassGraph* passExecutionGraph
        );

        using DebugBufferIteratorFunc = std::function<void(PassName passName, const float* debugData)>;
        using SchedulingInfoConfigurator = std::function<void(PipelineResourceSchedulingInfo&)>;

        const HAL::RTDescriptor* GetRenderTargetDescriptor(Foundation::Name resourceName, Foundation::Name passName, uint64_t mipIndex = 0);
        const HAL::DSDescriptor* GetDepthStencilDescriptor(Foundation::Name resourceName, Foundation::Name passName);

        void BeginFrame();
        void EndFrame();

        bool HasMemoryLayoutChange() const;
        
        PipelineResourceStoragePass& CreatePerPassData(PassName name);

        void StartResourceScheduling();
        void EndResourceScheduling();
        void AllocateScheduledResources();
        
        template <class Constants> 
        void UpdateGlobalRootConstants(const Constants& constants);

        template <class Constants>
        void UpdateFrameRootConstants(const Constants& constants);

        template <class Constants>
        void UpdatePassRootConstants(const Constants& constants, const RenderPassGraph::Node& passNode);

        const Memory::Buffer* GlobalRootConstantsBuffer() const;
        const Memory::Buffer* PerFrameRootConstantsBuffer() const;

        PipelineResourceStoragePass* GetPerPassData(PassName name);
        PipelineResourceStorageResource* GetPerResourceData(ResourceName name);
        const PipelineResourceStoragePass* GetPerPassData(PassName name) const;
        const PipelineResourceStorageResource* GetPerResourceData(ResourceName name) const;

        void IterateDebugBuffers(const DebugBufferIteratorFunc& func) const;

        void QueueTextureAllocationIfNeeded(
            ResourceName resourceName,
            HAL::FormatVariant format,
            HAL::TextureKind kind,
            const Geometry::Dimensions& dimensions,
            const HAL::ClearValue& optimizedClearValue,
            uint16_t mipCount,
            const SchedulingInfoConfigurator& siConfigurator
        );

        template <class BufferDataT>
        void QueueBufferAllocationIfNeeded(
            ResourceName resourceName,
            uint64_t capacity,
            uint64_t perElementAlignment,
            const SchedulingInfoConfigurator& siConfigurator
        );

        void QueueResourceUsage(ResourceName resourceName, std::optional<ResourceName> aliasName, const SchedulingInfoConfigurator& siConfigurator);

    private:
        using ResourceMap = robin_hood::unordered_flat_map<ResourceName, uint64_t>;
        using ResourceAliasMap = robin_hood::unordered_flat_map<ResourceName, ResourceName>;
        using ResourceList = std::vector<PipelineResourceStorageResource>;
        using DiffEntryList = std::vector<PipelineResourceStorageResource::DiffEntry>;

        struct SchedulingRequest
        {
            SchedulingInfoConfigurator Configurator;
            Foundation::Name ResourceName;
        };

        PipelineResourceStorageResource& CreatePerResourceData(ResourceName name, const HAL::ResourceFormat& resourceFormat);

        bool TransferPreviousFrameResources();

        HAL::Device* mDevice;
        Memory::GPUResourceProducer* mResourceProducer;
        Memory::PoolDescriptorAllocator* mDescriptorAllocator;
        Memory::ResourceStateTracker* mResourceStateTracker;
        const RenderPassGraph* mPassExecutionGraph;

        std::unique_ptr<HAL::Heap> mRTDSHeap;
        std::unique_ptr<HAL::Heap> mNonRTDSHeap;
        std::unique_ptr<HAL::Heap> mBufferHeap;
        std::unique_ptr<HAL::Heap> mUniversalHeap;

        RenderSurfaceDescription mDefaultRenderSurface;

        PipelineResourceMemoryAliaser mRTDSMemoryAliaser;
        PipelineResourceMemoryAliaser mNonRTDSMemoryAliaser;
        PipelineResourceMemoryAliaser mBufferMemoryAliaser;
        PipelineResourceMemoryAliaser mUniversalMemoryAliaser;

        // Constant buffer for global data that changes rarely
        Memory::GPUResourceProducer::BufferPtr mGlobalRootConstantsBuffer;

        // Constant buffer for data that changes every frame
        Memory::GPUResourceProducer::BufferPtr mPerFrameRootConstantsBuffer;

        robin_hood::unordered_node_map<PassName, PipelineResourceStoragePass> mPerPassData;

        std::vector<std::function<void()>> mAllocationActions;
        std::vector<SchedulingRequest> mSchedulingCreationRequests;
        std::vector<SchedulingRequest> mSchedulingUsageRequests;

        // Two sets of resources: current and previous frame
        std::pair<ResourceList, ResourceList> mResourceLists;
        std::pair<ResourceMap, ResourceMap> mResourceMaps;
        ResourceList* mPreviousFrameResources = &mResourceLists.first;
        ResourceList* mCurrentFrameResources = &mResourceLists.second;
        ResourceMap* mPreviousFrameResourceMap = &mResourceMaps.first;
        ResourceMap* mCurrentFrameResourceMap = &mResourceMaps.second;
        ResourceAliasMap mAliasMap;

        // Resource diff entries to determine resource allocation needs
        std::pair<DiffEntryList, DiffEntryList> mDiffEntries;
        DiffEntryList* mPreviousFrameDiffEntries = &mDiffEntries.first;
        DiffEntryList* mCurrentFrameDiffEntries = &mDiffEntries.second;

        // Transitions for resources scheduled for readback
        HAL::ResourceBarrierCollection mReadbackBarriers;

        bool mMemoryLayoutChanged = false;
    };

}

#include "PipelineResourceStorage.inl"