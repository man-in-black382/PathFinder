#include "ResourceStorage.hpp"
#include "RenderPass.hpp"

#include "../Foundation/StringUtils.hpp"

namespace PathFinder
{

    ResourceStorage::ResourceStorage(HAL::Device* device, const RenderSurface& defaultRenderSurface, uint8_t simultaneousFramesInFlight)
        : mDevice{ device },
        mDefaultRenderSurface{ defaultRenderSurface },
        mDescriptorStorage{ device },
        mSimultaneousFramesInFlight{ simultaneousFramesInFlight } {}

    void ResourceStorage::BeginFrame(uint64_t frameFenceValue)
    {
        for (auto& passBufferPair : mPerpassConstantBuffers)
        {
            auto& buffer = passBufferPair.second;
            buffer->PrepareMemoryForNewFrame(frameFenceValue);
        }
    }

    void ResourceStorage::EndFrame(uint64_t completedFrameFenceValue)
    {
        for (auto& passBufferPair : mPerpassConstantBuffers)
        {
            auto& buffer = passBufferPair.second;
            buffer->DiscardMemoryForCompletedFrames(completedFrameFenceValue);
        }
    }

    const HAL::RTDescriptor& ResourceStorage::GetRenderTargetDescriptor(Foundation::Name resourceName)
    {
        if (auto format = GetResourceShaderVisibleFormatForCurrentPass(resourceName))
        {
            if (auto descriptor = mDescriptorStorage.GetRTDescriptor(resourceName, *format))
            {
                return *descriptor;
            }
            else {
                throw std::invalid_argument("Resource was not scheduled to be used as render target");
            }
        }
        else {
            throw std::invalid_argument("Resource format for this pass is unknown");
        }
    }

    const HAL::RTDescriptor& ResourceStorage::GetCurrentBackBufferDescriptor()
    {
        return mBackBufferDescriptors[mCurrentBackBufferIndex];
    }

    const HAL::DSDescriptor& ResourceStorage::GetDepthStencilDescriptor(ResourceName resourceName)
    {
        if (auto descriptor = mDescriptorStorage.GetDSDescriptor(resourceName))
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
        for (auto& nameAllocationPair : mResourceAllocationActions)
        {
            auto& allocation = nameAllocationPair.second;
            allocation();
        }

        mResourceAllocationActions.clear();
    }

    void ResourceStorage::UseSwapChain(HAL::SwapChain& swapChain)
    {
        for (auto i = 0; i < swapChain.BackBuffers().size(); i++)
        {
            mBackBufferDescriptors.push_back(mDescriptorStorage.EmplaceRTDescriptorIfNeeded(BackBufferNames[i], *swapChain.BackBuffers()[i]));
        }
    }

    void ResourceStorage::RegisterStateForResource(ResourceName resourceName, HAL::ResourceState state)
    {
        mResourcePerPassStates[std::make_tuple(mCurrentPassName, resourceName)] = state;
        mResourceExpectedStates[resourceName] |= state;
    }

    void ResourceStorage::RegisterColorFormatForResource(ResourceName resourceName, HAL::ResourceFormat::Color format)
    {
        mResourceShaderVisibleFormatMap[std::make_tuple(mCurrentPassName, resourceName)] = format;
    }

    void ResourceStorage::MarkResourceNameAsScheduled(ResourceName name)
    {
        mScheduledResourceNames[mCurrentPassName].push_back(name);
    }

    HAL::BufferResource<uint8_t>* ResourceStorage::GetRootConstantBufferForCurrentPass() const
    {
        auto it = mPerpassConstantBuffers.find(mCurrentPassName);
        if (it == mPerpassConstantBuffers.end()) return nullptr;
        return it->second.get();
    }

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
        auto it = mResourcePerPassStates.find(std::make_tuple(mCurrentPassName, resourceName));
        if (it == mResourcePerPassStates.end()) return std::nullopt;
        return it->second;
    }

    std::optional<HAL::ResourceFormat::Color> ResourceStorage::GetResourceShaderVisibleFormatForCurrentPass(ResourceName resourceName) const
    {
        auto it = mResourceShaderVisibleFormatMap.find(std::make_tuple(mCurrentPassName, resourceName));
        if (it == mResourceShaderVisibleFormatMap.end()) return std::nullopt;
        return it->second;
    }

}
