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
#include <functional>
#include <tuple>
#include <memory>

namespace PathFinder
{

    class RenderPass;

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

    private:
        HAL::ResourceState GatherExpectedStates() const;

        HAL::ResourceFormat::FormatVariant Format;
        std::function<void()> AllocationAction;
        std::unordered_map<PassName, PerPassEntities> PerPassData;
    };



    class PipelineResource
    {
    public:
        friend class ResourceStorage;
        friend class ResoruceScheduler;

        struct PerPassEntities
        {
            std::optional<HAL::ResourceState> OptimizedState;
            std::optional<HAL::ResourceFormat::Color> ShaderVisibleFormat;
        };

        HAL::ResourceState CurrentState;

    private:
        std::unique_ptr<HAL::Resource> mResource;
        std::unordered_map<PassName, PerPassEntities> mPerPassData;

    public:
        const HAL::Resource* Resource() const { return mResource.get(); }

        std::optional<PerPassEntities> GetPerPassData(PassName passName) const
        {
            auto it = mPerPassData.find(passName);
            if (it == mPerPassData.end()) return std::nullopt;
            return it->second;
        };
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
        void SetCurrentStateForResource(ResourceName name, HAL::ResourceState state);
        void AllocateScheduledResources(); 
        void UseSwapChain(HAL::SwapChain& swapChain);

        const ResourceDescriptorStorage& DescriptorStorage() const;
        GlobalRootConstants* GlobalRootConstantData();
        PerFrameRootConstants* PerFrameRootConstantData();
        template <class RootConstants> RootConstants* RootConstantDataForCurrentPass() const;
        HAL::BufferResource<uint8_t>* RootConstantBufferForCurrentPass() const;
        const HAL::BufferResource<GlobalRootConstants>& GlobalRootConstantsBuffer() const;
        const HAL::BufferResource<PerFrameRootConstants>& PerFrameRootConstantsBuffer() const;
        const std::vector<ResourceName>& ScheduledResourceNamesForCurrentPass();
        PipelineResource& GetPipelineResource(ResourceName resourceName);

    private:
        const std::vector<Foundation::Name> BackBufferNames{ "BackBuffer1", "BackBuffer2", "BackBuffer3" };

    private:
        bool IsResourceAllocationScheduled(ResourceName name) const;

        PipelineResourceAllocator* GetResourceAllocator(ResourceName name);

        PipelineResourceAllocator* QueueTextureAllocationIfNeeded(
            ResourceName resourceName,
            HAL::ResourceFormat::FormatVariant format,
            HAL::ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions,
            const HAL::Resource::ClearValue& optimizedClearValue
        );

        HAL::ResourceState GatherExpectedStates(const PipelineResourceAllocator& allocator);
        void CreateDescriptors(ResourceName resourceName, const PipelineResourceAllocator& allocator, const HAL::TextureResource& texture);

        template <class BufferDataT> void AllocateRootConstantBufferIfNeeded();

        HAL::Device* mDevice;

        RenderSurface mDefaultRenderSurface;

        // This class's logic works with 'the current' render pass.
        // Saves the user from passing current pass name in every possible API.
        PassName mCurrentPassName;

        // Amount of frames to be scheduled until a CPU wait is required.
        uint8_t mSimultaneousFramesInFlight;

        // Manages descriptor heaps
        ResourceDescriptorStorage mDescriptorStorage;

        // Dedicated storage for back buffer descriptors.
        // No fancy management is required.
        std::vector<HAL::RTDescriptor> mBackBufferDescriptors;

        // Constant buffers for each pass that require it.
        std::unordered_map<PassName, std::unique_ptr<HAL::RingBufferResource<uint8_t>>> mPerPassConstantBuffers;
        
        std::unordered_map<PassName, std::vector<ResourceName>> mPerPassResourceNames;

        std::unordered_map<ResourceName, PipelineResourceAllocator> mPipelineResourceAllocators;

        std::unordered_map<ResourceName, PipelineResource> mPipelineResources;

        HAL::RingBufferResource<GlobalRootConstants> mGlobalRootConstantsBuffer;

        HAL::RingBufferResource<PerFrameRootConstants> mPerFrameRootConstantsBuffer;

        uint8_t mCurrentBackBufferIndex = 0;
    };

}

#include "ResourceStorage.inl"