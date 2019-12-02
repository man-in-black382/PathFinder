#pragma once

#include "PipelineResourceStorage.hpp"
#include "RenderSurfaceDescription.hpp"
#include "ResourceDescriptorStorage.hpp"
#include "HashSpecializations.hpp"
#include "GlobalRootConstants.hpp"
#include "PerFrameRootConstants.hpp"
#include "PipelineResource.hpp"
#include "PipelineResourceAllocation.hpp"
#include "PipelineResourceMemoryAliaser.hpp"
#include "PipelineResourceStateOptimizer.hpp"

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
        struct PerPassObjects
        {
            // Constant buffers for each pass that require it.
            std::unique_ptr<HAL::RingBufferResource<uint8_t>> PassConstantBuffer;

            // Resource names scheduled for each pass
            std::unordered_set<ResourceName> ScheduledResourceNames;

            // Resource transition and aliasing barriers for each pass
            HAL::ResourceBarrierCollection TransitionAndAliasingBarriers;

            // UAV barriers to be applied after each draw/dispatch in
            // a pass that makes unordered accesses to resources
            HAL::ResourceBarrierCollection UAVBarriers;
        };

        struct PerResourceObjects
        {
            std::unique_ptr<PipelineResourceAllocation> Allocation;
            std::unique_ptr<TexturePipelineResource> Texture;
            std::unique_ptr<BufferPipelineResource> Buffer;

            const HAL::Resource* GetResource() const;
        };

        PipelineResourceStorage(
            HAL::Device* device, ResourceDescriptorStorage* descriptorStorage, 
            const RenderSurfaceDescription& defaultRenderSurface, uint8_t simultaneousFramesInFlight,
            const RenderPassExecutionGraph* passExecutionGraph
        );

        void BeginFrame(uint64_t frameFenceValue);
        void EndFrame(uint64_t completedFrameFenceValue);

        const HAL::RTDescriptor& GetRenderTargetDescriptor(Foundation::Name resourceName) const;
        const HAL::DSDescriptor& GetDepthStencilDescriptor(Foundation::Name resourceName) const;
        const HAL::UADescriptor& GetUnorderedAccessDescriptor(Foundation::Name resourceName) const;
        const HAL::RTDescriptor& GetCurrentBackBufferDescriptor() const;
        
        void SetCurrentBackBufferIndex(uint8_t index);
        void SetCurrentPassName(PassName passName);
        void AllocateScheduledResources();
        void CreateSwapChainBackBufferDescriptors(const HAL::SwapChain& swapChain);

        GlobalRootConstants* GlobalRootConstantData();
        PerFrameRootConstants* PerFrameRootConstantData();
        template <class RootConstants> RootConstants* RootConstantDataForCurrentPass();
        HAL::BufferResource<uint8_t>* RootConstantBufferForCurrentPass();
        const HAL::BufferResource<GlobalRootConstants>& GlobalRootConstantsBuffer() const;
        const HAL::BufferResource<PerFrameRootConstants>& PerFrameRootConstantsBuffer() const;
        const std::unordered_set<ResourceName>& ScheduledResourceNamesForCurrentPass();
        const TexturePipelineResource* GetPipelineTextureResource(ResourceName resourceName) const;
        const BufferPipelineResource* GetPipelineBufferResource(ResourceName resourceName) const;
        const HAL::Resource* GetResource(ResourceName resourceName) const;
        const HAL::ResourceBarrierCollection& TransitionAndAliasingBarriersForCurrentPass();
        const HAL::ResourceBarrierCollection& UnorderedAccessBarriersForCurrentPass();
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
            const HAL::ResourceFormat::ClearValue& optimizedClearValue,
            uint16_t mipCount
        );

        template <class BufferDataT>
        PipelineResourceAllocation* QueueBufferAllocationIfNeeded(
            ResourceName resourceName,
            uint64_t capacity,
            uint64_t perElementAlignment
        );

    private:
        PerPassObjects& GetPerPassObjects(PassName name);
        PerResourceObjects& GetPerResourceObjects(ResourceName name);
        const PerPassObjects* GetPerPassObjects(PassName name) const;
        const PerResourceObjects* GetPerResourceObjects(ResourceName name) const;

        void CreateDescriptors(TexturePipelineResource& resource, const PipelineResourceAllocation& allocator);
        void CreateDescriptors(BufferPipelineResource& resource, const PipelineResourceAllocation& allocator, uint64_t explicitStride);

        void PrepareAllocationsForOptimization();
        void CreateResourceBarriers();

        HAL::Device* mDevice;

        std::unique_ptr<HAL::Heap> mRTDSHeap;
        std::unique_ptr<HAL::Heap> mNonRTDSHeap;
        std::unique_ptr<HAL::Heap> mBufferHeap;

        RenderSurfaceDescription mDefaultRenderSurface;

        PipelineResourceStateOptimizer mStateOptimizer;

        PipelineResourceMemoryAliaser mRTDSMemoryAliaser;
        PipelineResourceMemoryAliaser mNonRTDSMemoryAliaser;
        PipelineResourceMemoryAliaser mBufferMemoryAliaser;

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

        // Constant buffer for global data that changes rarely
        HAL::RingBufferResource<GlobalRootConstants> mGlobalRootConstantsBuffer;

        // Constant buffer for data that changes every frame
        HAL::RingBufferResource<PerFrameRootConstants> mPerFrameRootConstantsBuffer;

        std::unordered_map<ResourceName, PerResourceObjects> mPerResourceObjects;

        std::unordered_map<ResourceName, PerPassObjects> mPerPassObjects;

        uint8_t mCurrentBackBufferIndex = 0;
    };

}

#include "PipelineResourceStorage.inl"