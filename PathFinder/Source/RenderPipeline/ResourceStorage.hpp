#pragma once

#include "ResourceStorage.hpp"
#include "RenderSurface.hpp"
#include "ResourceDescriptorStorage.hpp"

#include "../HardwareAbstractionLayer/DescriptorHeap.hpp"
#include "../HardwareAbstractionLayer/SwapChain.hpp"
#include "../HardwareAbstractionLayer/RingBufferResource.hpp"
#include "../Foundation/MemoryUtils.hpp"

#include <vector>
#include <unordered_map>
#include <functional>
#include <tuple>

namespace PathFinder
{

    class RenderPass;

    class ResourceStorage
    {
    public:
        friend class ResourceScheduler;
        friend class ResourceProvider;
        friend class RootConstantsUpdater;

        using ResourceName = Foundation::Name;
        using PassName = Foundation::Name;

        ResourceStorage(HAL::Device* device, const RenderSurface& defaultRenderSurface, uint8_t simultaneousFramesInFlight);

        void BeginFrame(uint64_t frameFenceValue);
        void EndFrame(uint64_t completedFrameFenceValue);

        HAL::RTDescriptor GetRenderTargetDescriptor(Foundation::Name resourceName) const;
        HAL::DSDescriptor GetDepthStencilDescriptor(Foundation::Name resourceName) const;
        HAL::RTDescriptor GetCurrentBackBufferDescriptor() const;
        
        void SetCurrentBackBufferIndex(uint8_t index);
        void SetCurrentPassName(PassName passName);
        void SetCurrentStateForResource(ResourceName name, HAL::ResourceState state);
        void AllocateScheduledResources();
        void UseSwapChain(HAL::SwapChain& swapChain);

        template <class RootConstants> 
        RootConstants* GetRootConstantDataForCurrentPass() const;

        HAL::BufferResource<uint8_t>* GetRootConstantBufferForCurrentPass() const;
        HAL::Resource* GetResource(ResourceName resourceName) const;

        const std::vector<ResourceName>* GetScheduledResourceNamesForCurrentPass() const;
        std::optional<HAL::ResourceState> GetResourceCurrentState(ResourceName resourceName) const;
        std::optional<HAL::ResourceState> GetResourceStateForCurrentPass(ResourceName resourceName) const;
        std::optional<HAL::ResourceFormat::Color> GetResourceShaderVisibleFormatForCurrentPass(ResourceName resourceName) const;

    private:
        const std::vector<Foundation::Name> BackBufferNames{ "BackBuffer1", "BackBuffer2", "BackBuffer3" };

    private:
        using ScheduledResourceNames = std::unordered_map<PassName, std::vector<ResourceName>>;
        using ResourceMap = std::unordered_map<ResourceName, std::unique_ptr<HAL::Resource>>;
        using ResourceStateMap = std::unordered_map<ResourceName, HAL::ResourceState>;
        using ResourcePerPassStateMap = std::unordered_map<PassName, std::unordered_map<ResourceName, HAL::ResourceState>>;
        using ResourceFormatMap = std::unordered_map<PassName, std::unordered_map<ResourceName, HAL::ResourceFormat::Color>>;
        using ResourceAllocationActions = std::unordered_map<ResourceName, std::function<void()>>;
        using BackBufferDescriptors = std::vector<HAL::RTDescriptor>;
        using BackBufferResources = std::vector<HAL::ColorTexture*>;
        using RootConstantBufferMap = std::unordered_map<PassName, std::unique_ptr<HAL::RingBufferResource<uint8_t>>>;

        //template <class BufferT> using TextureAllocationCallback = std::function<void(const ResourceT&)>;
        template <class TextureT> using TextureAllocationCallback = std::function<void(const TextureT&)>;
        template <class TextureT> using TexturePostAllocationActionMap = std::unordered_map<ResourceName, std::vector<TextureAllocationCallback<TextureT>>>;

        using TexturePostAllocationActions = std::tuple<
            TexturePostAllocationActionMap<HAL::ColorTexture>,
            TexturePostAllocationActionMap<HAL::TypelessTexture>,
            TexturePostAllocationActionMap<HAL::DepthStencilTexture>
        >;

        template <class TextureT, class ...Args>
        void QueueTextureAllocationIfNeeded(ResourceName resourceName, const TextureAllocationCallback<TextureT>& callback, Args&&... args);

        template <class BufferDataT>
        void AllocateRootConstantBufferIfNeeded();

        void RegisterStateForResource(ResourceName resourceName, HAL::ResourceState state);
        void RegisterColorFormatForResource(ResourceName resourceName, HAL::ResourceFormat::Color format);
        void MarkResourceNameAsScheduled(ResourceName name);

        HAL::Device* mDevice;

        RenderSurface mDefaultRenderSurface;

        // This class's logic works with 'the current' render pass.
        // Saves the user from passing current pass name in every possible API.
        PassName mCurrentPassName;

        // Amount of frames to be scheduled until a CPU wait is required.
        uint8_t mSimultaneousFramesInFlight;

        // Names of all registered resources for every render pass
        ScheduledResourceNames mScheduledResourceNames;

        // Storage for resource pointers. Holds them in memory.
        ResourceMap mResources;

        // Stores states that are requested for each render pass
        ResourcePerPassStateMap mResourcePerPassStates;

        // Masks of all states of each resource. 
        // Each mask contains all states the resource will
        // go through in the frame.
        ResourceStateMap mResourceExpectedStates;

        // Marks current states of each resource.
        // Changes through the frame in each render pass.
        ResourceStateMap mResourceCurrentStates;

        // Holds per-pass color formats of each resource:
        // actual types for typeless textures for a typed shader access,
        // actual types of primitive (non-structured) buffers and so on
        ResourceFormatMap mResourceShaderVisibleFormatMap;

        // Lambdas that'll allocate texture resources after frame scheduling is done
        ResourceAllocationActions mResourceAllocationActions;

        // Callbacks to be called after texture resource is allocated
        TexturePostAllocationActions mTexturePostAllocationActions;

        // Manages descriptor heaps
        ResourceDescriptorStorage mDescriptorStorage;

        // Dedicated storage for back buffer descriptors.
        // No fancy management is required.
        BackBufferDescriptors mBackBufferDescriptors;

        // Constant buffers for each pass that require it.
        RootConstantBufferMap mPerpassConstantBuffers;

        uint8_t mCurrentBackBufferIndex = 0;
    };

}

#include "ResourceStorage.inl"