#include "ResourceStorage.hpp"
#include "RenderPass.hpp"
#include "RenderPassExecutionGraph.hpp"

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
        PipelineResource* pipelineResource = GetPipelineResource(resourceName);

        std::optional<HAL::ResourceFormat::Color> format = std::nullopt;

        auto perPassData = pipelineResource->GetPerPassData(mCurrentPassName);
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

    void ResourceStorage::AllocateScheduledResources(const RenderPassExecutionGraph& executionGraph)
    {
        for (auto& pair : mPipelineResourceAllocators)
        {
            PipelineResourceAllocator& allocator = pair.second;
            allocator.mAllocationAction();
        }

        OptimizeResourceStates(executionGraph);
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
        return mPipelineResourceAllocators.find(name) != mPipelineResourceAllocators.end()
            ? &mPipelineResourceAllocators.at(name) : nullptr;
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

        allocator.mAllocationAction = [=, &allocator]()
        {
            HAL::ResourceState expectedStates = allocator.GatherExpectedStates();
            HAL::ResourceState initialState = HAL::ResourceState::Common;
            
            auto texture = std::make_unique<HAL::TextureResource>(
                *mDevice, format, kind, dimensions, optimizedClearValue, initialState, expectedStates);

            texture->D3DPtr()->SetName(StringToWString(resourceName.ToSring()).c_str());

            PipelineResource newResource;

            for (auto& pair : allocator.mPerPassData)
            {
                PassName passName = pair.first;
                PipelineResourceAllocator::PerPassEntities& allocatorPerPassData = pair.second;
                PipelineResource::PerPassEntities& newResourcePerPassData = newResource.mPerPassData[passName];

                newResourcePerPassData.IsRTDescriptorRequested = allocatorPerPassData.RTInserter != nullptr;
                newResourcePerPassData.IsDSDescriptorRequested = allocatorPerPassData.DSInserter != nullptr;
                newResourcePerPassData.IsSRDescriptorRequested = allocatorPerPassData.SRInserter != nullptr;
                newResourcePerPassData.IsUADescriptorRequested = allocatorPerPassData.UAInserter != nullptr;
            }

            CreateDescriptors(resourceName, allocator, *texture);

            newResource.mResource = std::move(texture);
            mPipelineResources[resourceName] = std::move(newResource);
        };

        return &allocator;
    }

    void ResourceStorage::CreateDescriptors(ResourceName resourceName, const PipelineResourceAllocator& allocator, const HAL::TextureResource& texture)
    {
        for (const auto& pair : allocator.mPerPassData)
        {
            const PipelineResourceAllocator::PerPassEntities& perPassData = pair.second;

            if (perPassData.RTInserter) (mDescriptorStorage.*perPassData.RTInserter)(resourceName, texture, perPassData.ShaderVisibleFormat);
            if (perPassData.DSInserter) (mDescriptorStorage.*perPassData.DSInserter)(resourceName, texture);
            if (perPassData.SRInserter) (mDescriptorStorage.*perPassData.SRInserter)(resourceName, texture, perPassData.ShaderVisibleFormat);
            if (perPassData.UAInserter) (mDescriptorStorage.*perPassData.UAInserter)(resourceName, texture, perPassData.ShaderVisibleFormat);
        }
    }

    void ResourceStorage::OptimizeResourceStates(const RenderPassExecutionGraph& executionGraph)
    {
        for (auto& pair : mPipelineResourceAllocators)
        {
            ResourceName resourceName = pair.first;
            PipelineResourceAllocator& allocator = pair.second;
            PipelineResource& resource = mPipelineResources.at(resourceName);

            auto optimizedStateChain = CollapseStateSequences(executionGraph, allocator);

            assert_format(!optimizedStateChain.empty(), "Resource mush have at least one state");
          
            // If resource will have only one state then it's sufficient 
            // to make a single transition in the first rendering loop
            //
            if (optimizedStateChain.size() == 1)
            {
                mOneTimeResourceBarriers.AddBarrier(HAL::ResourceTransitionBarrier{ HAL::ResourceState::Common, optimizedStateChain.back().second, resource.Resource() });
                continue;
            }

            // See whether automatic state transitions are possible
            bool canImplicitlyPromoteToFirstState = resource.Resource()->CanImplicitlyPromoteFromCommonStateToState(optimizedStateChain.front().second);
            bool canImplicitlyDecayFromLastState = resource.Resource()->CanImplicitlyDecayToCommonStateFromState(optimizedStateChain.back().second);
            bool canSkipFirstTransition = canImplicitlyPromoteToFirstState && canImplicitlyDecayFromLastState;

            uint32_t firstExplicitTransitionIndex = 0;

            if (canSkipFirstTransition)
            {
                // Skip first transition if possible
                //
                PassName secondPassName = optimizedStateChain[1].first;
                HAL::ResourceState implicitFirstState = optimizedStateChain[0].second;
                HAL::ResourceState explicitSecondState = optimizedStateChain[1].second;

                mPerPassResourceBarriers[secondPassName].AddBarrier(HAL::ResourceTransitionBarrier{ implicitFirstState, explicitSecondState, resource.Resource() });

                firstExplicitTransitionIndex = 1;
            }
            else {
                // No way to skip first transition, loop last transition to first
                //
                PassName firstPassName = optimizedStateChain[0].first;
                HAL::ResourceState explicitLastState = optimizedStateChain[optimizedStateChain.size() - 1].second;
                HAL::ResourceState explicitFirstState = optimizedStateChain[0].second;

                mPerPassResourceBarriers[firstPassName].AddBarrier(HAL::ResourceTransitionBarrier{ explicitLastState, explicitFirstState, resource.Resource() });
            }

            // Create the rest of the transitions
            for (auto i = firstExplicitTransitionIndex + 1; i < optimizedStateChain.size(); ++i)
            {
                PassName currentPassName = optimizedStateChain[i].first;
                mPerPassResourceBarriers[currentPassName].AddBarrier(HAL::ResourceTransitionBarrier{ optimizedStateChain[i - 1].second, optimizedStateChain[i].second, resource.Resource() });
            }
        }
    }

    std::vector<std::pair<PassName, HAL::ResourceState>> ResourceStorage::CollapseStateSequences(const RenderPassExecutionGraph& executionGraph, const PipelineResourceAllocator& allocator)
    {
        std::vector<PassName> relevantPassNames;
        std::vector<std::pair<PassName, HAL::ResourceState>> optimizedStateChain;
        HAL::ResourceState stateSequence = HAL::ResourceState::Common;
        PassName stateTransitionPassName;

        // Build a list of passes this resource is scheduled for
        for (const RenderPass* pass : executionGraph.ExecutionOrder())
        {
            if (allocator.GetPerPassData(pass->Name()))
            {
                relevantPassNames.push_back(pass->Name());
            }
        }

        if (relevantPassNames.empty())
        {
            return {};
        }

        // Gather first state sequence
        for (auto relevantPassIdx = 0u; relevantPassIdx < relevantPassNames.size(); ++relevantPassIdx)
        {
            PassName currentPassName = relevantPassNames[relevantPassIdx];
            auto perPassData = allocator.GetPerPassData(currentPassName);

            bool isLastPass = relevantPassIdx == relevantPassNames.size() - 1;

            // Is state read-only
            if (HAL::IsResourceStateReadOnly(perPassData->RequestedState))
            {
                if (HAL::IsResourceStateReadOnly(stateSequence))
                {
                    // If previous state was read-only, then combine next with previous
                    stateSequence |= perPassData->RequestedState;
                }
                else {
                    // This is a first read-only state in a possible read-only sequence
                    stateSequence = perPassData->RequestedState;
                    stateTransitionPassName = currentPassName;
                }

                if (!isLastPass)
                {
                    // If next state is not read-only then this sequence should be dumped
                    PassName nextPassName = relevantPassNames[relevantPassIdx + 1];
                    auto nextPerPassData = allocator.GetPerPassData(nextPassName);

                    if (!HAL::IsResourceStateReadOnly(nextPerPassData->RequestedState))
                    {
                        optimizedStateChain.push_back({ stateTransitionPassName, stateSequence });
                    }
                }
                else {
                    // If there is no next state then this sequence definitely should be dumped
                    optimizedStateChain.push_back({ stateTransitionPassName, stateSequence });
                }
            }
            else {
                stateSequence = perPassData->RequestedState;
                stateTransitionPassName = currentPassName;

                optimizedStateChain.push_back({ stateTransitionPassName, stateSequence });
            }
        }

        return optimizedStateChain;
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

    PathFinder::PipelineResource* ResourceStorage::GetPipelineResource(ResourceName resourceName)
    {
        auto it = mPipelineResources.find(resourceName);
        if (it == mPipelineResources.end()) return nullptr;
        return &it->second;
    }

    const HAL::ResourceBarrierCollection& ResourceStorage::OneTimeResourceBarriers() const
    {
        return mOneTimeResourceBarriers;
    }

    const HAL::ResourceBarrierCollection& ResourceStorage::ResourceBarriersForCurrentPass()
    {
        return mPerPassResourceBarriers[mCurrentPassName];
    }

    HAL::ResourceState PipelineResourceAllocator::GatherExpectedStates() const
    {
        HAL::ResourceState expectedStates = HAL::ResourceState::Common;

        for (const auto& pair : mPerPassData)
        {
            const PerPassEntities& perPassData = pair.second;
            expectedStates |= perPassData.RequestedState;
        }

        return expectedStates;
    }

    std::optional<PipelineResourceAllocator::PerPassEntities> PipelineResourceAllocator::GetPerPassData(PassName passName) const
    {
        auto it = mPerPassData.find(passName);
        if (it == mPerPassData.end()) return std::nullopt;
        return it->second;
    }

    std::optional<PipelineResource::PerPassEntities> PipelineResource::GetPerPassData(PassName passName) const
    {
        auto it = mPerPassData.find(passName);
        if (it == mPerPassData.end()) return std::nullopt;
        return it->second;
    }

    std::optional<HAL::ResourceTransitionBarrier> PipelineResource::GetStateTransition() const
    {
        return mOneTimeStateTransition;
    }

}
