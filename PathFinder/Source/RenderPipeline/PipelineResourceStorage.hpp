#pragma once

#include "PipelineResourceStorage.hpp"
#include "RenderSurfaceDescription.hpp"
#include "GlobalRootConstants.hpp"
#include "PerFrameRootConstants.hpp"
#include "PipelineResource.hpp"
#include "PipelineResourceSchedulingInfo.hpp"
#include "PipelineResourceMemoryAliaser.hpp"
#include "PipelineResourceStateOptimizer.hpp"

#include "../HardwareAbstractionLayer/DescriptorHeap.hpp"
#include "../HardwareAbstractionLayer/SwapChain.hpp"
#include "../Foundation/MemoryUtils.hpp"
#include "../Memory/GPUResourceProducer.hpp"
#include "../Memory/PoolDescriptorAllocator.hpp"

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
            //std::unique_ptr<HAL::RingBufferResource<uint8_t>> PassConstantBuffer;

            //// Debug buffers for each pass.
            //std::unique_ptr<HAL::Buffer<float>> PassDebugBuffer;

            //// Debug readback buffers for each pass.
            //std::unique_ptr<HAL::RingBufferResource<float>> PassDebugReadbackBuffer;

            // Resource names scheduled for each pass
            std::unordered_set<ResourceName> ScheduledResourceNames;

            // Resource transition and aliasing barriers for each pass
            HAL::ResourceBarrierCollection AliasingBarriers;

            // UAV barriers to be applied after each draw/dispatch in
            // a pass that makes unordered accesses to resources
            HAL::ResourceBarrierCollection UAVBarriers;
        };

        struct PerResourceObjects
        {
            std::unique_ptr<PipelineResourceSchedulingInfo> SchedulingInfo;
            std::unique_ptr<TexturePipelineResource> Texture;
            std::unique_ptr<BufferPipelineResource> Buffer;

            const Memory::GPUResource* GetGPUResource() const;
        };

        PipelineResourceStorage(
            HAL::Device* device,
            Memory::GPUResourceProducer* resourceProducer,
            Memory::PoolDescriptorAllocator* descriptorAllocator,
            const RenderSurfaceDescription& defaultRenderSurface,
            const RenderPassExecutionGraph* passExecutionGraph
        );

      /*  using DebugBufferIteratorFunc = std::function<void(
            PassName passName, const HAL::Buffer<float>* debugBuffer, const HAL::RingBufferResource<float>* debugReadbackBuffer
        )>;*/

        const HAL::RTDescriptor& GetRenderTargetDescriptor(Foundation::Name resourceName);
        const HAL::DSDescriptor& GetDepthStencilDescriptor(Foundation::Name resourceName);
        const HAL::RTDescriptor& GetCurrentBackBufferDescriptor() const;
        
        void SetCurrentBackBufferIndex(uint8_t index);
        void SetCurrentPassName(PassName passName);
        void AllocateScheduledResources();
        void CreateSwapChainBackBufferDescriptors(const HAL::SwapChain& swapChain);

     /*   GlobalRootConstants* GlobalRootConstantData();
        PerFrameRootConstants* PerFrameRootConstantData();*/
        template <class RootConstants> RootConstants* RootConstantDataForCurrentPass();
        /*  HAL::Buffer<uint8_t>* RootConstantBufferForCurrentPass();
          const HAL::Buffer<float>* DebugBufferForCurrentPass() const;
          const HAL::RingBufferResource<float>* DebugReadbackBufferForCurrentPass() const;
          const HAL::Buffer<GlobalRootConstants>& GlobalRootConstantsBuffer() const;
          const HAL::Buffer<PerFrameRootConstants>& PerFrameRootConstantsBuffer() const;*/
        const std::unordered_set<ResourceName>& ScheduledResourceNamesForCurrentPass();
        const TexturePipelineResource* GetPipelineTextureResource(ResourceName resourceName) const;
        const BufferPipelineResource* GetPipelineBufferResource(ResourceName resourceName) const;
        const Memory::GPUResource* GetGPUResource(ResourceName resourceName) const;
        const HAL::ResourceBarrierCollection& AliasingBarriersForCurrentPass();
        const HAL::ResourceBarrierCollection& UnorderedAccessBarriersForCurrentPass();
        const Foundation::Name CurrentPassName() const;

        //void IterateDebugBuffers(const DebugBufferIteratorFunc& func) const;

        bool IsResourceAllocationScheduled(ResourceName name) const;
        void RegisterResourceNameForCurrentPass(ResourceName name);
        PipelineResourceSchedulingInfo* GetResourceSchedulingInfo(ResourceName name);

        template <class BufferDataT>
        void AllocateRootConstantBufferIfNeeded();

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
        PerPassObjects& GetPerPassObjects(PassName name);
        PerResourceObjects& GetPerResourceObjects(ResourceName name);
        const PerPassObjects* GetPerPassObjects(PassName name) const;
        const PerResourceObjects* GetPerResourceObjects(ResourceName name) const;

        void CreateDebugBuffers(const RenderPassExecutionGraph* passExecutionGraph);
        void PrepareSchedulingInfoForOptimization();
        void CreateAliasingAndUAVBarriers();

        HAL::Device* mDevice;
        Memory::GPUResourceProducer* mResourceProducer;
        Memory::PoolDescriptorAllocator* mDescriptorAllocator;

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
        PassName mCurrentPassName;

        // Dedicated storage for back buffer descriptors.
        // No fancy management is required.
        std::vector<HAL::RTDescriptor> mBackBufferDescriptors;

        //// Constant buffer for global data that changes rarely
        //HAL::RingBufferResource<GlobalRootConstants> mGlobalRootConstantsBuffer;

        //// Constant buffer for data that changes every frame
        //HAL::RingBufferResource<PerFrameRootConstants> mPerFrameRootConstantsBuffer;

        std::unordered_map<ResourceName, PerResourceObjects> mPerResourceObjects;

        std::unordered_map<ResourceName, PerPassObjects> mPerPassObjects;

        // Transitions for resources scheduled for readback
        HAL::ResourceBarrierCollection mReadbackBarriers;

        uint8_t mCurrentBackBufferIndex = 0;
    };

}

#include "PipelineResourceStorage.inl"