#include "ResourceManager.hpp"

namespace PathFinder
{

    ResourceManager::ResourceManager(HAL::Device* device, const RenderSurface& defaultRenderSurface)
        : mDevice{ device },
        mDefaultRenderSurface{ defaultRenderSurface },
        mDescriptorStorage{ device } {}

    //-------------------------------- Scheduling --------------------------------//

    void ResourceManager::WillRenderToRenderTarget(
        Foundation::Name resourceName, HAL::ResourceFormat::Color dataFormat,
        HAL::ResourceFormat::TextureKind kind, const Geometry::Dimensions& dimensions)
    {
        MarkResourceNameAsScheduled(resourceName);
        RegisterStateForResource(resourceName, HAL::ResourceState::RenderTarget);

        QueueTextureAllocationIfNeeded<HAL::ColorTextureResource>(
            resourceName, 
            [this, resourceName](const HAL::ColorTextureResource& resource) 
        { 
            mDescriptorStorage.EmplaceRTDescriptorIfNeeded(resourceName, resource);
        }, 
            *mDevice, 
            dataFormat, 
            kind, 
            dimensions);
    }

    void ResourceManager::WillRenderToRenderTarget(
        Foundation::Name resourceName,
        HAL::ResourceFormat::TypelessColor dataFormat,
        HAL::ResourceFormat::Color shaderVisisbleFormat,
        HAL::ResourceFormat::TextureKind kind, 
        const Geometry::Dimensions& dimensions)
    {
        MarkResourceNameAsScheduled(resourceName);
        RegisterStateForResource(resourceName, HAL::ResourceState::RenderTarget);

        QueueTextureAllocationIfNeeded<HAL::TypelessTextureResource>(
            resourceName, 
            [this, resourceName, shaderVisisbleFormat](const HAL::TypelessTextureResource& resource)
        {
            mDescriptorStorage.EmplaceRTDescriptorIfNeeded(resourceName, resource, shaderVisisbleFormat);
        }, 
            *mDevice, 
            dataFormat, 
            kind, 
            dimensions);
    }

    void ResourceManager::WillRenderToRenderTarget(Foundation::Name resourceName)
    {
        MarkResourceNameAsScheduled(resourceName);
        RegisterStateForResource(resourceName, HAL::ResourceState::RenderTarget);

        QueueTextureAllocationIfNeeded<HAL::ColorTextureResource>(
            resourceName, 
            [this, resourceName](const HAL::ColorTextureResource& resource)
        {
            mDescriptorStorage.EmplaceRTDescriptorIfNeeded(resourceName, resource);
        }, 
            *mDevice, 
            mDefaultRenderSurface.RenderTargetFormat(),
            HAL::ResourceFormat::TextureKind::Texture2D, 
            mDefaultRenderSurface.Dimensions());
    }

    void ResourceManager::WillRenderToDepthStencil(
        Foundation::Name resourceName,
        HAL::ResourceFormat::DepthStencil dataFormat,
        const Geometry::Dimensions& dimensions)
    {
        MarkResourceNameAsScheduled(resourceName);
        RegisterStateForResource(resourceName, HAL::ResourceState::DepthWrite);

        QueueTextureAllocationIfNeeded<HAL::DepthStencilTextureResource>(
            resourceName,
            [this, resourceName](const HAL::DepthStencilTextureResource& resource)
        {
            mDescriptorStorage.EmplaceDSDescriptorIfNeeded(resourceName, resource);
        }, 
            *mDevice, 
            dataFormat, 
            dimensions);
    }

    void ResourceManager::WillRenderToDepthStencil(Foundation::Name resourceName)
    {
        MarkResourceNameAsScheduled(resourceName);
        RegisterStateForResource(resourceName, HAL::ResourceState::DepthWrite);

        QueueTextureAllocationIfNeeded<HAL::DepthStencilTextureResource>(
            resourceName,
            [this, resourceName](const HAL::DepthStencilTextureResource& resource)
        {
            mDescriptorStorage.EmplaceDSDescriptorIfNeeded(resourceName, resource);
        },
            *mDevice, 
            mDefaultRenderSurface.DepthStencilFormat(), 
            mDefaultRenderSurface.Dimensions());
    }

    //-------------------------------- Providing --------------------------------//

    HAL::RTDescriptor ResourceManager::GetRenderTarget(Foundation::Name resourceName) const
    {
        if (auto format = GetResourceShaderVisibleFormatForCurrentPass(resourceName); 
            auto descriptor = mDescriptorStorage.TryGetRTDescriptor(resourceName, format.value()))
        {
            return *descriptor;
        }

        throw std::invalid_argument("Resource was not scheduled to be used as render target");
    }

    HAL::RTDescriptor ResourceManager::GetBackBuffer() const
    {
        return mBackBufferDescriptors[mCurrentBackBufferIndex];
    }

    HAL::DSDescriptor ResourceManager::GetDepthStencil(ResourceName resourceName) const
    {
        if (auto descriptor = mDescriptorStorage.TryGetDSDescriptor(resourceName))
        {
            return *descriptor;
        }

        throw std::invalid_argument("Resource was not scheduled to be used as depth-stencil");
    }

    void ResourceManager::SetCurrentBackBufferIndex(uint8_t index)
    {
        mCurrentBackBufferIndex = index;
    }

    void ResourceManager::SetCurrentPassName(Foundation::Name passName)
    {
        mCurrentPassName = passName;
    }

    void ResourceManager::SetCurrentStateForResource(ResourceName name, HAL::ResourceState state)
    {
        mResourceCurrentStates[name] = state;
    }

    void ResourceManager::AllocateScheduledResources()
    {
        for (auto& nameAllocationPair : mResourceDelayedAllocations)
        {
            auto& allocation = nameAllocationPair.second;
            allocation();
        }

        mResourceDelayedAllocations.clear();
    }

    void ResourceManager::UseSwapChain(HAL::SwapChain& swapChain)
    {
        for (auto i = 0; i < swapChain.BackBuffers().size(); i++)
        {
            mBackBufferDescriptors.push_back(mDescriptorStorage.EmplaceRTDescriptorIfNeeded(BackBufferNames[i], *swapChain.BackBuffers()[i]));
        }
    }

    //-------------------------------- Resource Management --------------------------------//

    void ResourceManager::RegisterStateForResource(ResourceName resourceName, HAL::ResourceState state)
    {
        mResourcePerPassStates[mCurrentPassName][resourceName] = state;
        mResourceExpectedStates[resourceName] |= state;
    }

    void ResourceManager::MarkResourceNameAsScheduled(ResourceName name)
    {
        mScheduledResourceNames[mCurrentPassName].push_back(name);
    }

    template <class TextureT, class ...Args>
    void ResourceManager::QueueTextureAllocationIfNeeded(ResourceName resourceName, const TextureAllocationCallback<TextureT>& callback, Args&&... args)
    {
        auto resourceIt = mResources.find(resourceName);
        bool isAlreadyAllocated = resourceIt != mResources.end();
        if (isAlreadyAllocated) return;

        // Save callback until resource is created
        TextureAllocationCallbackMap<TextureT>& callbacks = std::get<TextureAllocationCallbackMap<TextureT>>(mTextureAllocationCallbacks);
        callbacks[resourceName].push_back(callback);

        bool isWaitingForAllocation = mResourceDelayedAllocations.find(resourceName) != mResourceDelayedAllocations.end();
        if (isWaitingForAllocation) return;

        Foundation::Name passName = mCurrentPassName;

        mResourceDelayedAllocations[resourceName] = [this, passName, resourceName, args...]()
        {
            HAL::ResourceState initialState = mResourcePerPassStates[passName][resourceName];
            HAL::ResourceState expectedStates = mResourceExpectedStates[resourceName];

            auto resource = std::make_unique<TextureT>(args..., initialState, expectedStates);

            // Call all callbacks associated with this resource name
            TextureAllocationCallbackMap<TextureT>& callbackMap = std::get<TextureAllocationCallbackMap<TextureT>>(mTextureAllocationCallbacks);
            std::vector<TextureAllocationCallback<TextureT>>& callbackList = callbackMap[resourceName];

            for (TextureAllocationCallback<TextureT>& callback : callbackList)
            {
                callback(*resource);
            }

            // Store the actual resource
            mResources.emplace(resourceName, std::move(resource));
            mResourceCurrentStates[resourceName] = initialState;
        };
    }

    //-------------------------------- Getters --------------------------------//

    HAL::Resource* ResourceManager::GetResource(ResourceName resourceName)
    {
        auto it = mResources.find(resourceName);
        if (it == mResources.end()) return nullptr;
        return it->second.get();
    }

    const std::vector<ResourceManager::ResourceName>* ResourceManager::GetScheduledResourceNamesForCurrentPass() const
    {
        if (mScheduledResourceNames.find(mCurrentPassName) == mScheduledResourceNames.end()) return nullptr;
        return &mScheduledResourceNames.at(mCurrentPassName);
    }

    std::optional<HAL::ResourceState> ResourceManager::GetResourceCurrentState(ResourceName resourceName) const
    {
        auto stateIt = mResourceCurrentStates.find(resourceName);
        if (stateIt == mResourceCurrentStates.end()) return std::nullopt;
        return stateIt->second;
    }

    std::optional<HAL::ResourceState> ResourceManager::GetResourceStateForCurrentPass(ResourceName resourceName) const
    {
        auto mapIt = mResourcePerPassStates.find(mCurrentPassName);

        if (mapIt == mResourcePerPassStates.end()) return std::nullopt;

        auto& map = mapIt->second;
        auto stateIt = map.find(resourceName);

        if (stateIt == map.end()) return std::nullopt;

        return stateIt->second;
    }

    std::optional<HAL::ResourceFormat::Color> ResourceManager::GetResourceShaderVisibleFormatForCurrentPass(ResourceName resourceName) const
    {
        auto mapIt = mResourceShaderVisibleFormatMap.find(mCurrentPassName);

        if (mapIt == mResourceShaderVisibleFormatMap.end()) return std::nullopt;

        auto& map = mapIt->second;
        auto formatIt = map.find(resourceName);

        if (formatIt == map.end()) return std::nullopt;

        return formatIt->second;
    }

}
