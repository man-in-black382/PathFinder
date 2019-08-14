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
        for (auto& passBufferPair : mPerPassConstantBuffers)
        {
            auto& buffer = passBufferPair.second;
            buffer->PrepareMemoryForNewFrame(frameFenceValue);
        }
    }

    void ResourceStorage::EndFrame(uint64_t completedFrameFenceValue)
    {
        for (auto& passBufferPair : mPerPassConstantBuffers)
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
        mPipelineResources[name].CurrentState = state;
    }

    void ResourceStorage::AllocateScheduledResources()
    {
       /* for (auto& nameAllocationPair : mResourceAllocationActions)
        {
            auto& allocation = nameAllocationPair.second;
            allocation();
        }

        mResourceAllocationActions.clear();*/
    }

    void ResourceStorage::UseSwapChain(HAL::SwapChain& swapChain)
    {
        for (auto i = 0; i < swapChain.BackBuffers().size(); i++)
        {
            mBackBufferDescriptors.push_back(mDescriptorStorage.EmplaceRTDescriptorIfNeeded(BackBufferNames[i], *swapChain.BackBuffers()[i]));
        }
    }

    bool ResourceStorage::IsResourceAllocationScheduled(ResourceName name) const
    {
        return mPipelineResourceAllocators.find(name) != mPipelineResourceAllocators.end();
    }

    PipelineResourceAllocator* ResourceStorage::GetResourceAllocator(ResourceName name)
    {
        return mPipelineResourceAllocators.find(name) != mPipelineResourceAllocators.end() ? &mPipelineResourceAllocators.at(name) : nullptr;
    }

    PipelineResourceAllocator* ResourceStorage::QueueTextureAllocationIfNeeded(
        ResourceName resourceName,
        HAL::ResourceFormat::FormatVariant format,
        HAL::ResourceFormat::TextureKind kind,
        const Geometry::Dimensions& dimensions,
        const HAL::Resource::ClearValue& optimizedClearValue)
    {
        auto it = mPipelineResourceAllocators.find(resourceName);

        if (it != mPipelineResourceAllocators.end())
        {
            return &it->second;
        }

        PassName passThatRequestedAllocation = mCurrentPassName;

        PipelineResourceAllocator& allocator = mPipelineResourceAllocators[resourceName];

        allocator.AllocationAction = [=, &allocator]()
        {
            HAL::ResourceState expectedStates = allocator.GatherExpectedStates();
            HAL::ResourceState initialState = allocator.PerPassData[passThatRequestedAllocation].RequestedState;
            
            auto texture = std::make_unique<HAL::TextureResource>(
                *mDevice, format, kind, dimensions, optimizedClearValue, initialState, expectedStates);

            PipelineResource newResource;
            newResource.CurrentState = initialState;

            // TransferShaderVisibleFormats();
            // OptimizeStates();
            CreateDescriptors(resourceName, allocator, *texture);

            newResource.Resource = std::move(texture);
            mPipelineResources[resourceName] = std::move(newResource);
        };
    }

    void ResourceStorage::CreateDescriptors(ResourceName resourceName, const PipelineResourceAllocator& allocator, const HAL::TextureResource& texture)
    {
        for (const auto& pair : allocator.PerPassData)
        {
            const PipelineResourceAllocator::PerPassEntities& perPassData = pair.second;

            if (perPassData.RTInserter) (mDescriptorStorage.*perPassData.RTInserter)(resourceName, texture, perPassData.ShaderVisibleFormat);
            if (perPassData.DSInserter) (mDescriptorStorage.*perPassData.DSInserter)(resourceName, texture);
            if (perPassData.SRInserter) (mDescriptorStorage.*perPassData.SRInserter)(resourceName, texture, perPassData.ShaderVisibleFormat);
            if (perPassData.UAInserter) (mDescriptorStorage.*perPassData.UAInserter)(resourceName, texture, perPassData.ShaderVisibleFormat);
        }
    }

    HAL::BufferResource<uint8_t>* ResourceStorage::GetRootConstantBufferForCurrentPass() const
    {
        auto it = mPerPassConstantBuffers.find(mCurrentPassName);
        if (it == mPerPassConstantBuffers.end()) return nullptr;
        return it->second.get();
    }

    HAL::Resource* ResourceStorage::GetResource(ResourceName resourceName) const
    {
        auto it = mPipelineResources.find(resourceName);
        if (it == mPipelineResources.end()) return nullptr;
        
        return it->second.Resource.get(); // ?: second.Buffer // TODO:
    }

    const std::vector<ResourceName>* ResourceStorage::GetScheduledResourceNamesForCurrentPass() const
    {
        if (mPerPassResourceNames.find(mCurrentPassName) == mPerPassResourceNames.end()) return nullptr;
        return &mPerPassResourceNames.at(mCurrentPassName);
    }

    std::optional<HAL::ResourceState> ResourceStorage::GetResourceCurrentState(ResourceName resourceName) const
    {
       /* PipelineResource *pipelineResource = GetPipelineResource(resourceName);
        if (!pipelineResource) return std::nullopt;
        return pipelineResource->CurrentState;*/

        return std::nullopt;
    }

    std::optional<HAL::ResourceState> ResourceStorage::GetResourceStateForCurrentPass(ResourceName resourceName) const
    {
        /*  PipelineResource *pipelineResource = GetPipelineResource(resourceName);
          if (!pipelineResource) return std::nullopt;

          auto perPassDataIt = pipelineResource->PerPassData.find(mCurrentPassName);
          if (perPassDataIt == pipelineResource->PerPassData.end()) return std::nullopt;

          return perPassDataIt->second.State;*/
        return std::nullopt;
    }

    std::optional<HAL::ResourceFormat::Color> ResourceStorage::GetResourceShaderVisibleFormatForCurrentPass(ResourceName resourceName) const
    {
        /*  PipelineResource *pipelineResource = GetPipelineResource(resourceName);
          if (!pipelineResource) return std::nullopt;

          auto perPassDataIt = pipelineResource->PerPassData.find(mCurrentPassName);
          if (perPassDataIt == pipelineResource->PerPassData.end()) return std::nullopt;

          return perPassDataIt->second.ColorFormat;*/
        return std::nullopt;
    }

    HAL::ResourceState PipelineResourceAllocator::GatherExpectedStates() const
    {
        HAL::ResourceState expectedStates = HAL::ResourceState::Common;

        for (const auto& pair : PerPassData)
        {
            const PerPassEntities& perPassData = pair.second;
            expectedStates |= perPassData.RequestedState;
        }

        return expectedStates;
    }

}
