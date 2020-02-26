#pragma once

#include "PipelineResourceStorage.hpp"
#include "RenderSurfaceDescription.hpp"
#include "GlobalRootConstants.hpp"
#include "PerFrameRootConstants.hpp"
#include "PipelineResourceSchedulingInfo.hpp"
#include "PipelineResourceMemoryAliaser.hpp"
#include "PipelineResourceStateOptimizer.hpp"

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

namespace PathFinder
{

    class RenderPassExecutionGraph;

    using ResourceName = Foundation::Name;
    using PassName = Foundation::Name;

    class PipelineResourceStorage
    {
    public:
        struct PerPassObjects
        {
            // Constant buffers for each pass that require it.
            Memory::GPUResourceProducer::BufferPtr PassConstantBuffer;

            // Debug buffer for each pass.
            Memory::GPUResourceProducer::BufferPtr PassDebugBuffer;

            // Resource names scheduled for each pass
            std::unordered_set<ResourceName> ScheduledResourceNames;

            // Resource aliasing barriers for the pass
            HAL::ResourceBarrierCollection AliasingBarriers;

            // UAV barriers to be applied after each draw/dispatch in
            // a pass that makes unordered accesses to resources
            HAL::ResourceBarrierCollection UAVBarriers;

            // State transition barriers for the pass
            HAL::ResourceBarrierCollection TransitionBarriers;
        };

        struct PerResourceObjects
        {
            std::optional<PipelineResourceSchedulingInfo> SchedulingInfo;
            Memory::GPUResourceProducer::TexturePtr Texture;
            Memory::GPUResourceProducer::BufferPtr Buffer;

            const Memory::GPUResource* GetGPUResource() const;
            Memory::GPUResource* GetGPUResource();
        };

        PipelineResourceStorage(
            HAL::Device* device,
            Memory::GPUResourceProducer* resourceProducer,
            Memory::PoolDescriptorAllocator* descriptorAllocator,
            Memory::ResourceStateTracker* stateTracker,
            const RenderSurfaceDescription& defaultRenderSurface,
            const RenderPassExecutionGraph* passExecutionGraph
        );

        using DebugBufferIteratorFunc = std::function<void(PassName passName, const float* debugData)>;

        const HAL::RTDescriptor& GetRenderTargetDescriptor(Foundation::Name resourceName);
        const HAL::DSDescriptor& GetDepthStencilDescriptor(Foundation::Name resourceName);
        const HAL::RTDescriptor& GetCurrentBackBufferDescriptor() const;
        
        void SetCurrentBackBufferIndex(uint8_t index);
        void SetCurrentRenderPassGraphNode(const RenderPassExecutionGraph::Node& node);
        void AllocateScheduledResources();
        void CreateSwapChainBackBufferDescriptors(const HAL::SwapChain& swapChain);
        void TransitionResourcesToCurrentPassStates();
        void RequestCurrentPassDebugReadback();
        
        template <class Constants> 
        void UpdateGlobalRootConstants(const Constants& constants);

        template <class Constants>
        void UpdateFrameRootConstants(const Constants& constants);

        template <class Constants>
        void UpdateCurrentPassRootConstants(const Constants& constants);

        const Memory::Buffer* GlobalRootConstantsBuffer() const;
        const Memory::Buffer* PerFrameRootConstantsBuffer() const;
        const Memory::Buffer* RootConstantsBufferForCurrentPass() const;
        const Memory::Buffer* DebugBufferForCurrentPass() const;
        const std::unordered_set<ResourceName>& ScheduledResourceNamesForCurrentPass();
        const Memory::Texture* GetTextureResource(ResourceName resourceName) const;
        const Memory::Buffer* GetBufferResource(ResourceName resourceName) const;
        const Memory::GPUResource* GetGPUResource(ResourceName resourceName) const;
        const HAL::ResourceBarrierCollection& AliasingBarriersForCurrentPass();
        const HAL::ResourceBarrierCollection& TransitionBarriersForCurrentPass();
        const HAL::ResourceBarrierCollection& UnorderedAccessBarriersForCurrentPass();
        const RenderPassExecutionGraph::Node& CurrentPassGraphNode() const;

        PerPassObjects& GetPerPassObjects(PassName name);
        PerResourceObjects& GetPerResourceObjects(ResourceName name);
        const PerPassObjects* GetPerPassObjects(PassName name) const;
        const PerResourceObjects* GetPerResourceObjects(ResourceName name) const;

        void IterateDebugBuffers(const DebugBufferIteratorFunc& func) const;

        bool IsResourceAllocationScheduled(ResourceName name) const;
        void RegisterResourceNameForCurrentPass(ResourceName name);
        PipelineResourceSchedulingInfo* GetResourceSchedulingInfo(ResourceName name);

        PipelineResourceSchedulingInfo* QueueTextureAllocationIfNeeded(
            ResourceName resourceName,
            HAL::ResourceFormat::FormatVariant format,
            HAL::TextureKind kind,
            const Geometry::Dimensions& dimensions,
            const HAL::ClearValue& optimizedClearValue,
            uint16_t mipCount
        );

        template <class BufferDataT>
        PipelineResourceSchedulingInfo* QueueBufferAllocationIfNeeded(
            ResourceName resourceName,
            uint64_t capacity,
            uint64_t perElementAlignment
        );

    private:
        void CreateDebugBuffers();
        void PrepareSchedulingInfoForOptimization();
        void CreateAliasingAndUAVBarriers();

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

        // This class's logic works with 'the current' render pass.
        // Saves the user from passing current pass name in every possible API.
        RenderPassExecutionGraph::Node mCurrentRenderPassGraphNode;

        // Dedicated storage for back buffer descriptors.
        // No fancy management is required.
        std::vector<Memory::PoolDescriptorAllocator::RTDescriptorPtr> mBackBufferDescriptors;

        // Constant buffer for global data that changes rarely
        Memory::GPUResourceProducer::BufferPtr mGlobalRootConstantsBuffer;

        // Constant buffer for data that changes every frame
        Memory::GPUResourceProducer::BufferPtr mPerFrameRootConstantsBuffer;

        std::unordered_map<ResourceName, PerResourceObjects> mPerResourceObjects;

        std::unordered_map<ResourceName, PerPassObjects> mPerPassObjects;
        PerPassObjects* mCurrentPassObjects = nullptr;

        // Transitions for resources scheduled for readback
        HAL::ResourceBarrierCollection mReadbackBarriers;

        uint8_t mCurrentBackBufferIndex = 0;
    };

}

#include "PipelineResourceStorage.inl"