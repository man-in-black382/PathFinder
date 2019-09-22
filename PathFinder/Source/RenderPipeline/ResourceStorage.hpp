#pragma once

#include "ResourceStorage.hpp"
#include "RenderSurface.hpp"
#include "ResourceDescriptorStorage.hpp"
#include "HashSpecializations.hpp"
#include "GlobalRootConstants.hpp"
#include "PerFrameRootConstants.hpp"

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

    class PipelineResourceAllocator
    {
    public:
        friend class ResourceStorage;
        friend class ResourceScheduler;

        using TextureRTDescriptorInserterPtr = decltype(&ResourceDescriptorStorage::EmplaceRTDescriptorIfNeeded);
        using TextureDSDescriptorInserterPtr = decltype(&ResourceDescriptorStorage::EmplaceDSDescriptorIfNeeded);
        using TextureSRDescriptorInserterPtr = decltype(&ResourceDescriptorStorage::EmplaceSRDescriptorIfNeeded);
        using TextureUADescriptorInserterPtr = decltype(&ResourceDescriptorStorage::EmplaceUADescriptorIfNeeded);

        struct PerPassEntities
        {
            HAL::ResourceState RequestedState;
            std::optional<HAL::ResourceFormat::Color> ShaderVisibleFormat;
            TextureRTDescriptorInserterPtr RTInserter = nullptr;
            TextureDSDescriptorInserterPtr DSInserter = nullptr;
            TextureSRDescriptorInserterPtr SRInserter = nullptr;
            TextureUADescriptorInserterPtr UAInserter = nullptr;
        };

        HAL::ResourceState GatherExpectedStates() const;
        std::optional<PerPassEntities> GetPerPassData(PassName passName) const;

    private:
        HAL::ResourceFormat::FormatVariant mFormat;
        std::function<void()> mAllocationAction;
        std::unordered_map<PassName, PerPassEntities> mPerPassData;
    };



    class PipelineResource
    {
    public:
        friend class ResourceStorage;
        friend class ResoruceScheduler;

        struct PerPassEntities
        {
            std::optional<HAL::ResourceFormat::Color> ShaderVisibleFormat;
            bool IsRTDescriptorRequested = false;
            bool IsDSDescriptorRequested = false;
            bool IsSRDescriptorRequested = false;
            bool IsUADescriptorRequested = false;
        };

        std::optional<PerPassEntities> GetPerPassData(PassName passName) const;
        std::optional<HAL::ResourceTransitionBarrier> GetStateTransition() const;

    private:
        std::unique_ptr<HAL::Resource> mResource;
        std::unordered_map<PassName, PerPassEntities> mPerPassData;
        std::optional<HAL::ResourceTransitionBarrier> mOneTimeStateTransition;

    public:
        const HAL::Resource* Resource() const { return mResource.get(); }
    };



    class ResourceStorage
    {
    public:
        using PassNameResourceName = NameNameTuple;

        friend class ResourceScheduler;
        friend class ResourceProvider;
        friend class RootConstantsUpdater;

        ResourceStorage(HAL::Device* device, const RenderSurface& defaultRenderSurface, uint8_t simultaneousFramesInFlight);

        void BeginFrame(uint64_t frameFenceValue);
        void EndFrame(uint64_t completedFrameFenceValue);

        const HAL::RTDescriptor& GetRenderTargetDescriptor(Foundation::Name resourceName);
        const HAL::DSDescriptor& GetDepthStencilDescriptor(Foundation::Name resourceName);
        const HAL::RTDescriptor& GetCurrentBackBufferDescriptor();
        
        void SetCurrentBackBufferIndex(uint8_t index);
        void SetCurrentPassName(PassName passName);
        void AllocateScheduledResources(const RenderPassExecutionGraph& executionGraph);
        void UseSwapChain(HAL::SwapChain& swapChain);

        const ResourceDescriptorStorage& DescriptorStorage() const;
        GlobalRootConstants* GlobalRootConstantData();
        PerFrameRootConstants* PerFrameRootConstantData();
        template <class RootConstants> RootConstants* RootConstantDataForCurrentPass() const;
        HAL::BufferResource<uint8_t>* RootConstantBufferForCurrentPass() const;
        const HAL::BufferResource<GlobalRootConstants>& GlobalRootConstantsBuffer() const;
        const HAL::BufferResource<PerFrameRootConstants>& PerFrameRootConstantsBuffer() const;
        const std::unordered_set<ResourceName>& ScheduledResourceNamesForCurrentPass();
        PipelineResource* GetPipelineResource(ResourceName resourceName);
        const HAL::ResourceBarrierCollection& OneTimeResourceBarriers() const;
        const HAL::ResourceBarrierCollection& ResourceBarriersForCurrentPass();

    private:
        const std::vector<Foundation::Name> BackBufferNames{ "BackBuffer1", "BackBuffer2", "BackBuffer3" };

    private:
        bool IsResourceAllocationScheduled(ResourceName name) const;

        void RegisterResourceNameForCurrentPass(ResourceName name);

        PipelineResourceAllocator* GetResourceAllocator(ResourceName name);

        void CreateDescriptors(ResourceName resourceName, const PipelineResourceAllocator& allocator, const HAL::TextureResource& texture);

        void OptimizeResourceStates(const RenderPassExecutionGraph& executionGraph);

        std::vector<std::pair<PassName, HAL::ResourceState>> CollapseStateSequences(const RenderPassExecutionGraph& executionGraph, const PipelineResourceAllocator& allocator);
        
        template <class BufferDataT> 
        void AllocateRootConstantBufferIfNeeded();

        PipelineResourceAllocator* QueueTextureAllocationIfNeeded(
            ResourceName resourceName,
            HAL::ResourceFormat::FormatVariant format,
            HAL::ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions,
            const HAL::ResourceFormat::ClearValue& optimizedClearValue
        );

        HAL::Device* mDevice;

        RenderSurface mDefaultRenderSurface;

        // This class's logic works with 'the current' render pass.
        // Saves the user from passing current pass name in every possible API.
        PassName mCurrentPassName;

        // Amount of frames to be scheduled until a CPU wait is required.
        // To be used by ring buffers.
        uint8_t mSimultaneousFramesInFlight;

        // Manages descriptor heaps
        ResourceDescriptorStorage mDescriptorStorage;

        // Dedicated storage for back buffer descriptors.
        // No fancy management is required.
        std::vector<HAL::RTDescriptor> mBackBufferDescriptors;

        // Constant buffers for each pass that require it.
        std::unordered_map<PassName, std::unique_ptr<HAL::RingBufferResource<uint8_t>>> mPerPassConstantBuffers;
        
        // Resource names scheduled for each pass
        std::unordered_map<PassName, std::unordered_set<ResourceName>> mPerPassResourceNames;

        // Allocations info for each resource
        std::unordered_map<ResourceName, PipelineResourceAllocator> mPipelineResourceAllocators;

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

#include "ResourceStorage.inl"