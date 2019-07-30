#pragma once

#include "IResourceProvider.hpp"
#include "IResourceScheduler.hpp"
#include "RenderSurface.hpp"

#include "../HardwareAbstractionLayer/DescriptorHeap.hpp"
#include "../HardwareAbstractionLayer/SwapChain.hpp"
#include "ResourceDescriptorStorage.hpp"

#include <vector>
#include <unordered_map>
#include <functional>
#include <tuple>

namespace PathFinder
{

    class ResourceManager : public IResourceScheduler
    {
    public:
        using ResourceName = Foundation::Name;
        using PassName = Foundation::Name;

        ResourceManager(HAL::Device* device, const RenderSurface& defaultRenderSurface);

        virtual void WillRenderToRenderTarget(
            Foundation::Name resourceName,
            HAL::ResourceFormat::Color dataFormat,
            HAL::ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions) override;

        virtual void WillRenderToRenderTarget(
            Foundation::Name resourceName,
            HAL::ResourceFormat::TypelessColor dataFormat,
            HAL::ResourceFormat::Color shaderVisisbleFormat,
            HAL::ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions) override;

        virtual void WillRenderToDepthStencil(
            Foundation::Name resourceName,
            HAL::ResourceFormat::DepthStencil dataFormat,
            const Geometry::Dimensions& dimensions) override;

        virtual void WillRenderToRenderTarget(Foundation::Name resourceName) override;
        virtual void WillRenderToDepthStencil(Foundation::Name resourceName) override;

        HAL::RTDescriptor GetRenderTarget(Foundation::Name resourceName);
        HAL::DSDescriptor GetDepthStencil(Foundation::Name resourceName);
        HAL::RTDescriptor GetBackBuffer();

        void SetCurrentBackBufferIndex(uint8_t index);
        void SetCurrentPassName(Foundation::Name passName);
        void SetCurrentStateForResource(ResourceName name, HAL::ResourceState state);
        void AllocateScheduledResources();
        void UseSwapChain(HAL::SwapChain& swapChain);

        HAL::Resource* GetResource(ResourceName resourceName);
        const std::vector<ResourceName>& GetScheduledResourceNamesForCurrentPass() const;
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
        using ResourceAllocationMap = std::unordered_map<ResourceName, std::function<void()>>;
        using BackBufferDescriptors = std::vector<HAL::RTDescriptor>;
        using BackBufferResources = std::vector<HAL::ColorTextureResource*>;

        template <class ResourceT> using TextureAllocationCallback = std::function<void(const ResourceT&)>;
        template <class ResourceT> using TextureAllocationCallbackMap = std::unordered_map<ResourceName, std::vector<TextureAllocationCallback<ResourceT>>>;

        using TextureAllocationCallbacks = std::tuple<
            TextureAllocationCallbackMap<HAL::ColorTextureResource>,
            TextureAllocationCallbackMap<HAL::TypelessTextureResource>,
            TextureAllocationCallbackMap<HAL::DepthStencilTextureResource>
        >;

        template <class TextureT, class ...Args>
        void QueueTextureAllocationIfNeeded(ResourceName resourceName, const TextureAllocationCallback<TextureT>& callback, Args&&... args);

        void RegisterStateForResource(ResourceName resourceName, HAL::ResourceState state);
        void MarkResourceNameAsScheduled(ResourceName name);

        HAL::Device* mDevice;

        RenderSurface mDefaultRenderSurface;
        PassName mCurrentPassName;

        ScheduledResourceNames mScheduledResourceNames;
        ResourceMap mResources;
        ResourcePerPassStateMap mResourcePerPassStates;
        ResourceStateMap mResourceExpectedStates;
        ResourceStateMap mResourceCurrentStates;
        ResourceAllocationMap mResourceDelayedAllocations;
        ResourceFormatMap mResourceShaderVisibleFormatMap;
        TextureAllocationCallbacks mTextureAllocationCallbacks;
        ResourceDescriptorStorage mDescriptorStorage;
        BackBufferDescriptors mBackBufferDescriptors;

        uint8_t mCurrentBackBufferIndex = 0;
    };

}
