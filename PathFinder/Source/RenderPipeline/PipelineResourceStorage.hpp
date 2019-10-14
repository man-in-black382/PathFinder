#pragma once

#include "PipelineResourceStorage.hpp"
#include "RenderSurfaceDescription.hpp"
#include "ResourceDescriptorStorage.hpp"
#include "HashSpecializations.hpp"
#include "GlobalRootConstants.hpp"
#include "PerFrameRootConstants.hpp"
#include "PipelineResource.hpp"
#include "PipelineResourceAllocation.hpp"

#include "../HardwareAbstractionLayer/DescriptorHeap.hpp"
#include "../HardwareAbstractionLayer/SwapChain.hpp"
#include "../HardwareAbstractionLayer/RingBufferResource.hpp"
#include "../Foundation/MemoryUtils.hpp"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <tuple>
#include <memory>

namespace PathFinder
{

    class RenderPass;
    class RenderPassExecutionGraph;

    using ResourceName = Foundation::Name;
    using PassName = Foundation::Name;

    class PipelineResourceStorage
    {
    public:
        using PassNameResourceName = NameNameTuple;

        PipelineResourceStorage(
            HAL::Device* device, ResourceDescriptorStorage* descriptorStorage, 
            const RenderSurfaceDescription& defaultRenderSurface, uint8_t simultaneousFramesInFlight
        );

        void BeginFrame(uint64_t frameFenceValue);
        void EndFrame(uint64_t completedFrameFenceValue);

        const HAL::RTDescriptor& GetRenderTargetDescriptor(Foundation::Name resourceName) const;
        const HAL::DSDescriptor& GetDepthStencilDescriptor(Foundation::Name resourceName) const;
        const HAL::RTDescriptor& GetCurrentBackBufferDescriptor() const;
        
        void SetCurrentBackBufferIndex(uint8_t index);
        void SetCurrentPassName(PassName passName);
        void AllocateScheduledResources(const RenderPassExecutionGraph& executionGraph);
        void CreateSwapChainBackBufferDescriptors(const HAL::SwapChain& swapChain);

        GlobalRootConstants* GlobalRootConstantData();
        PerFrameRootConstants* PerFrameRootConstantData();
        template <class RootConstants> RootConstants* RootConstantDataForCurrentPass() const;
        HAL::BufferResource<uint8_t>* RootConstantBufferForCurrentPass() const;
        const HAL::BufferResource<GlobalRootConstants>& GlobalRootConstantsBuffer() const;
        const HAL::BufferResource<PerFrameRootConstants>& PerFrameRootConstantsBuffer() const;
        const std::unordered_set<ResourceName>& ScheduledResourceNamesForCurrentPass();
        const PipelineResource* GetPipelineResource(ResourceName resourceName) const;
        const HAL::ResourceBarrierCollection& OneTimeResourceBarriers() const;
        const HAL::ResourceBarrierCollection& ResourceBarriersForCurrentPass();
        const Foundation::Name CurrentPassName() const;
        const ResourceDescriptorStorage* DescriptorStorage() const;

        bool IsResourceAllocationScheduled(ResourceName name) const;
        void RegisterResourceNameForCurrentPass(ResourceName name);
        PipelineResourceAllocation* GetResourceAllocator(ResourceName name);

        template <class BufferDataT>
        void AllocateRootConstantBufferIfNeeded();

        PipelineResourceAllocation* QueueTextureAllocationIfNeeded(
            ResourceName resourceName,
            HAL::ResourceFormat::FormatVariant format,
            HAL::ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions,
            const HAL::ResourceFormat::ClearValue& optimizedClearValue
        );

    private:
        void CreateDescriptors(ResourceName resourceName, PipelineResource& resource, const PipelineResourceAllocation& allocator, const HAL::TextureResource& texture);
        void CreateStateTransitionBarriers(const RenderPassExecutionGraph& executionGraph);
        std::vector<std::pair<PassName, HAL::ResourceState>> CollapseStateSequences(const RenderPassExecutionGraph& executionGraph, const PipelineResourceAllocation& allocator);

        HAL::Device* mDevice;

        RenderSurfaceDescription mDefaultRenderSurface;

        // This class's logic works with 'the current' render pass.
        // Saves the user from passing current pass name in every possible API.
        PassName mCurrentPassName;

        // Amount of frames to be scheduled until a CPU wait is required.
        // To be used by ring buffers.
        uint8_t mSimultaneousFramesInFlight;

        // Manages descriptor heaps
        ResourceDescriptorStorage* mDescriptorStorage;

        // Dedicated storage for back buffer descriptors.
        // No fancy management is required.
        std::vector<HAL::RTDescriptor> mBackBufferDescriptors;

        // Constant buffers for each pass that require it.
        std::unordered_map<PassName, std::unique_ptr<HAL::RingBufferResource<uint8_t>>> mPerPassConstantBuffers;
        
        // Resource names scheduled for each pass
        std::unordered_map<PassName, std::unordered_set<ResourceName>> mPerPassResourceNames;

        // Allocations info for each resource
        std::unordered_map<ResourceName, PipelineResourceAllocation> mPipelineResourceAllocations;

        // Allocated pipeline resources
        std::unordered_map<ResourceName, PipelineResource> mPipelineResources;

        // Resource barriers for each pass
        std::unordered_map<PassName, HAL::ResourceBarrierCollection> mPerPassResourceBarriers;

        // Constant buffer for global data that changes rarely
        HAL::RingBufferResource<GlobalRootConstants> mGlobalRootConstantsBuffer;

        // Constant buffer for data that changes every frame
        HAL::RingBufferResource<PerFrameRootConstants> mPerFrameRootConstantsBuffer;

        // Resource barriers to be performed once as initial state setup
        HAL::ResourceBarrierCollection mOneTimeResourceBarriers;

        // 
        uint8_t mCurrentBackBufferIndex = 0;
    };

}

#include "PipelineResourceStorage.inl"