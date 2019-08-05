#include "ResourceStorage.hpp"
#include "RenderPass.hpp"

namespace PathFinder
{

    ResourceStorage::ResourceStorage(HAL::Device* device, const RenderSurface& defaultRenderSurface, uint8_t simultaneousFramesInFlight)
        : mDevice{ device },
        mDefaultRenderSurface{ defaultRenderSurface },
        mDescriptorStorage{ device },
        mSimultaneousFramesInFlight{ simultaneousFramesInFlight } {}

    HAL::RTDescriptor ResourceStorage::GetRenderTarget(Foundation::Name resourceName) const
    {
        if (auto format = GetResourceShaderVisibleFormatForCurrentPass(resourceName); 
            auto descriptor = mDescriptorStorage.TryGetRTDescriptor(resourceName, format.value()))
        {
            return *descriptor;
        }

        throw std::invalid_argument("Resource was not scheduled to be used as render target");
    }

    HAL::RTDescriptor ResourceStorage::GetBackBuffer() const
    {
        return mBackBufferDescriptors[mCurrentBackBufferIndex];
    }

    HAL::DSDescriptor ResourceStorage::GetDepthStencil(ResourceName resourceName) const
    {
        if (auto descriptor = mDescriptorStorage.TryGetDSDescriptor(resourceName))
        {
            return *descriptor;
        }

        throw std::invalid_argument("Resource was not scheduled to be used as depth-stencil");
    }

    void ResourceStorage::SetCurrentBackBufferIndex(uint8_t index)
    {
        mCurrentBackBufferIndex = index;
    }

    void ResourceStorage::SetCurrentPassName(PassName passName)
    {
        mCurrentPassName = passName;
    }

    void ResourceStorage::SetCurrentStateForResource(ResourceName name, HAL::ResourceState state)
    {
        mResourceCurrentStates[name] = state;
    }

    void ResourceStorage::AllocateScheduledResources()
    {
        for (auto& nameAllocationPair : mResourceDelayedAllocations)
        {
            auto& allocation = nameAllocationPair.second;
            allocation();
        }

        mResourceDelayedAllocations.clear();
    }

    void ResourceStorage::UseSwapChain(HAL::SwapChain& swapChain)
    {
        for (auto i = 0; i < swapChain.BackBuffers().size(); i++)
        {
            mBackBufferDescriptors.push_back(mDescriptorStorage.EmplaceRTDescriptorIfNeeded(BackBufferNames[i], *swapChain.BackBuffers()[i]));
        }
    }

    //-------------------------------- Resource Management --------------------------------//

    void ResourceStorage::RegisterStateForResource(ResourceName resourceName, HAL::ResourceState state)
    {
        mResourcePerPassStates[mCurrentPassName][resourceName] = state;
        mResourceExpectedStates[resourceName] |= state;
    }

    void ResourceStorage::MarkResourceNameAsScheduled(ResourceName name)
    {
        mScheduledResourceNames[mCurrentPassName].push_back(name);
    }

    //-------------------------------- Getters --------------------------------//

    HAL::Resource* ResourceStorage::GetResource(ResourceName resourceName) const
    {
        auto it = mResources.find(resourceName);
        if (it == mResources.end()) return nullptr;
        return it->second.get();
    }

    const std::vector<ResourceStorage::ResourceName>* ResourceStorage::GetScheduledResourceNamesForCurrentPass() const
    {
        if (mScheduledResourceNames.find(mCurrentPassName) == mScheduledResourceNames.end()) return nullptr;
        return &mScheduledResourceNames.at(mCurrentPassName);
    }

    std::optional<HAL::ResourceState> ResourceStorage::GetResourceCurrentState(ResourceName resourceName) const
    {
        auto stateIt = mResourceCurrentStates.find(resourceName);
        if (stateIt == mResourceCurrentStates.end()) return std::nullopt;
        return stateIt->second;
    }

    std::optional<HAL::ResourceState> ResourceStorage::GetResourceStateForCurrentPass(ResourceName resourceName) const
    {
        auto mapIt = mResourcePerPassStates.find(mCurrentPassName);

        if (mapIt == mResourcePerPassStates.end()) return std::nullopt;

        auto& map = mapIt->second;
        auto stateIt = map.find(resourceName);

        if (stateIt == map.end()) return std::nullopt;

        return stateIt->second;
    }

    std::optional<HAL::ResourceFormat::Color> ResourceStorage::GetResourceShaderVisibleFormatForCurrentPass(ResourceName resourceName) const
    {
        auto mapIt = mResourceShaderVisibleFormatMap.find(mCurrentPassName);

        if (mapIt == mResourceShaderVisibleFormatMap.end()) return std::nullopt;

        auto& map = mapIt->second;
        auto formatIt = map.find(resourceName);

        if (formatIt == map.end()) return std::nullopt;

        return formatIt->second;
    }

}
