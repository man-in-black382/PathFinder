#include "ResourceStorage.hpp"
#include "RenderPass.hpp"

#include "../Foundation/StringUtils.hpp"
#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    ResourceStorage::ResourceStorage(HAL::Device* device, const RenderSurface& defaultRenderSurface, uint8_t simultaneousFramesInFlight)
        : mDevice{ device },
        mDefaultRenderSurface{ defaultRenderSurface },
        mDescriptorStorage{ device },
        mSimultaneousFramesInFlight{ simultaneousFramesInFlight },

        mGlobalRootConstantsBuffer{
            *device, 1, simultaneousFramesInFlight, 256,
            HAL::ResourceState::ConstantBuffer, HAL::ResourceState::ConstantBuffer,
            HAL::CPUAccessibleHeapType::Upload },

        mPerFrameRootConstantsBuffer{
            *device, 1, simultaneousFramesInFlight, 256,
            HAL::ResourceState::ConstantBuffer, HAL::ResourceState::ConstantBuffer,
            HAL::CPUAccessibleHeapType::Upload }
    {}

    void ResourceStorage::BeginFrame(uint64_t frameFenceValue)
    {
        for (auto& passBufferPair : mPerPassConstantBuffers)
        {
            auto& buffer = passBufferPair.second;
            buffer->PrepareMemoryForNewFrame(frameFenceValue);
        }

        mGlobalRootConstantsBuffer.PrepareMemoryForNewFrame(frameFenceValue);
        mPerFrameRootConstantsBuffer.PrepareMemoryForNewFrame(frameFenceValue);
    }

    void ResourceStorage::EndFrame(uint64_t completedFrameFenceValue)
    {
        for (auto& passBufferPair : mPerPassConstantBuffers)
        {
            auto& buffer = passBufferPair.second;
            buffer->DiscardMemoryForCompletedFrames(completedFrameFenceValue);
        }

        mGlobalRootConstantsBuffer.DiscardMemoryForCompletedFrames(completedFrameFenceValue);
        mPerFrameRootConstantsBuffer.DiscardMemoryForCompletedFrames(completedFrameFenceValue);
    }

    const HAL::RTDescriptor& ResourceStorage::GetRenderTargetDescriptor(Foundation::Name resourceName)
    {
        PipelineResource& pipelineResource = GetPipelineResource(resourceName);

        std::optional<HAL::ResourceFormat::Color> format = std::nullopt;

        auto perPassData = pipelineResource.GetPerPassData(mCurrentPassName);
        if (perPassData) format = perPassData->ShaderVisibleFormat;

        auto descriptor = mDescriptorStorage.GetRTDescriptor(resourceName, format);
        assert_format(descriptor, "Resource ", resourceName.ToSring(), " was not scheduled to be used as render target");

        return *descriptor;
    }

    const HAL::RTDescriptor& ResourceStorage::GetCurrentBackBufferDescriptor()
    {
        return mBackBufferDescriptors[mCurrentBackBufferIndex];
    }

    const HAL::DSDescriptor& ResourceStorage::GetDepthStencilDescriptor(ResourceName resourceName)
    {
        auto descriptor = mDescriptorStorage.GetDSDescriptor(resourceName);
        assert_format(descriptor, "Resource ", resourceName.ToSring(), " was not scheduled to be used as depth-stencil target");
        return *descriptor;
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
        for (auto& pair : mPipelineResourceAllocators)
        {
            PipelineResourceAllocator& allocator = pair.second;
            allocator.AllocationAction();
        }
    }

    void ResourceStorage::UseSwapChain(HAL::SwapChain& swapChain)
    {
        for (auto i = 0; i < swapChain.BackBuffers().size(); i++)
        {
            mBackBufferDescriptors.push_back(mDescriptorStorage.EmplaceRTDescriptorIfNeeded(BackBufferNames[i], *swapChain.BackBuffers()[i]));
        }
    }

    const ResourceDescriptorStorage& ResourceStorage::DescriptorStorage() const
    {
        return mDescriptorStorage;
    }

    GlobalRootConstants* ResourceStorage::GlobalRootConstantData()
    {
        return mGlobalRootConstantsBuffer.At(0);
    }

    PerFrameRootConstants* ResourceStorage::PerFrameRootConstantData()
    {
        return mPerFrameRootConstantsBuffer.At(0);
    }

    bool ResourceStorage::IsResourceAllocationScheduled(ResourceName name) const
    {
        return mPipelineResourceAllocators.find(name) != mPipelineResourceAllocators.end();
    }

    void ResourceStorage::RegisterResourceNameForCurrentPass(ResourceName name)
    {
        mPerPassResourceNames[mCurrentPassName].insert(name);
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
        const HAL::ResourceFormat::ClearValue& optimizedClearValue)
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

            for (auto& pair : allocator.PerPassData)
            {
                PassName passName = pair.first;
                PipelineResourceAllocator::PerPassEntities& perPassData = pair.second;

                // TODO: Compute optimized states
                newResource.mPerPassData[passName].OptimizedState = perPassData.RequestedState;
            }

            // OptimizeStates();
            CreateDescriptors(resourceName, allocator, *texture);

            newResource.mResource = std::move(texture);
            mPipelineResources[resourceName] = std::move(newResource);
        };

        return &allocator;
    }

    void ResourceStorage::CreateDescriptors(ResourceName resourceName, const PipelineResourceAllocator& allocator, const HAL::TextureResource& texture)
    {
        for (const auto& pair : allocator.PerPassData)
        {
            const PipelineResourceAllocator::PerPassEntities& perPassData = pair.second;

            //std::optional<HAL::Res>

            //if (std::holds_alternative<HAL::ResourceFormat::Color>(texture.Format()) &&
            //    std::get<HAL::ResourceFormat::Color>(texture.Format())

            if (perPassData.RTInserter) (mDescriptorStorage.*perPassData.RTInserter)(resourceName, texture, perPassData.ShaderVisibleFormat);
            if (perPassData.DSInserter) (mDescriptorStorage.*perPassData.DSInserter)(resourceName, texture);
            if (perPassData.SRInserter) (mDescriptorStorage.*perPassData.SRInserter)(resourceName, texture, perPassData.ShaderVisibleFormat);
            if (perPassData.UAInserter) (mDescriptorStorage.*perPassData.UAInserter)(resourceName, texture, perPassData.ShaderVisibleFormat);
        }
    }

    HAL::BufferResource<uint8_t>* ResourceStorage::RootConstantBufferForCurrentPass() const
    {
        auto it = mPerPassConstantBuffers.find(mCurrentPassName);
        if (it == mPerPassConstantBuffers.end()) return nullptr;
        return it->second.get();
    }

    const HAL::BufferResource<GlobalRootConstants>& ResourceStorage::GlobalRootConstantsBuffer() const
    {
        return mGlobalRootConstantsBuffer;
    }

    const HAL::BufferResource<PerFrameRootConstants>& ResourceStorage::PerFrameRootConstantsBuffer() const
    {
        return mPerFrameRootConstantsBuffer;
    }

    const std::unordered_set<ResourceName>& ResourceStorage::ScheduledResourceNamesForCurrentPass()
    {
        return mPerPassResourceNames[mCurrentPassName];
    }

    PipelineResource& ResourceStorage::GetPipelineResource(ResourceName resourceName)
    {
        auto it = mPipelineResources.find(resourceName);
        assert_format(it != mPipelineResources.end(), "Pipeline resource ", resourceName.ToSring(), " does not exist");
        return it->second;
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
