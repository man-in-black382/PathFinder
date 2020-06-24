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
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <tuple>
#include <memory>
#include <optional>

#include <dtl/dtl.hpp>

namespace PathFinder
{

    class RenderPassGraph;

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
            const RenderPassGraph* passExecutionGraph
        );

        using DebugBufferIteratorFunc = std::function<void(PassName passName, const float* debugData)>;
        using SchedulingInfoConfigurator = std::function<void(PipelineResourceSchedulingInfo&)>;

        const HAL::RTDescriptor* GetRenderTargetDescriptor(Foundation::Name resourceName, Foundation::Name passName, uint64_t mipIndex = 0);
        const HAL::DSDescriptor* GetDepthStencilDescriptor(Foundation::Name resourceName, Foundation::Name passName);
        bool HasMemoryLayoutChange() const;
        
        void CreatePerPassData();

        void StartResourceScheduling();
        void EndResourceScheduling();
        
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

        void QueueTexturesAllocationIfNeeded(
            ResourceName resourceName,
            HAL::ResourceFormat::FormatVariant format,
            HAL::TextureKind kind,
            const Geometry::Dimensions& dimensions,
            const HAL::ClearValue& optimizedClearValue,
            uint16_t mipCount,
            const SchedulingInfoConfigurator& siConfigurator
        );

        template <class BufferDataT>
        void QueueBuffersAllocationIfNeeded(
            ResourceName resourceName,
            uint64_t capacity,
            uint64_t perElementAlignment,
            const SchedulingInfoConfigurator& siConfigurator
        );

        void QueueResourceUsage(ResourceName resourceName, const SchedulingInfoConfigurator& siConfigurator);

    private:
        using ResourceMap = std::unordered_map<ResourceName, PipelineResourceStorageResource>;
        using DiffEntryList = std::vector<PipelineResourceStorageResource::DiffEntry>;

        PipelineResourceStoragePass& CreatePerPassData(PassName name);
        PipelineResourceStorageResource& CreatePerResourceData(ResourceName name, const HAL::ResourceFormat& resourceFormat);

        void CreateDebugBuffers();
        bool TransferPreviousFrameResources();
        void FinalizeSchedulingInfo();

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

        std::unordered_map<PassName, PipelineResourceStoragePass> mPerPassData;
        std::vector<std::function<void()>> mAllocationActions;
        std::vector<std::pair<SchedulingInfoConfigurator, Foundation::Name>> mSchedulingInfoCreationConfiguators;
        std::vector<std::pair<SchedulingInfoConfigurator, Foundation::Name>> mSchedulingInfoUsageConfiguators;

        // Two sets of resources: current and previous frame
        std::pair<ResourceMap, ResourceMap> mPerResourceData;
        ResourceMap* mPreviousFrameResources = &mPerResourceData.first;
        ResourceMap* mCurrentFrameResources = &mPerResourceData.second;

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