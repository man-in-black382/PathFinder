#include "PipelineResourceStorage.hpp"
#include "RenderPass.hpp"
#include "RenderPassExecutionGraph.hpp"

#include "../Foundation/StringUtils.hpp"
#include "../Foundation/Assert.hpp"
#include "../Foundation/STDHelpers.hpp"

namespace PathFinder
{

    PipelineResourceStorage::PipelineResourceStorage(
        HAL::Device* device, ResourceDescriptorStorage* descriptorStorage,
        const RenderSurfaceDescription& defaultRenderSurface, uint8_t simultaneousFramesInFlight,
        const RenderPassExecutionGraph* passExecutionGraph)
        :
        mDevice{ device },
        mStateOptimizer{ passExecutionGraph },
        mRTDSMemoryAliaser{ passExecutionGraph },
        mNonRTDSMemoryAliaser{ passExecutionGraph },
        mBufferMemoryAliaser{ passExecutionGraph },
        mDefaultRenderSurface{ defaultRenderSurface },
        mDescriptorStorage{ descriptorStorage },
        mSimultaneousFramesInFlight{ simultaneousFramesInFlight },
        mGlobalRootConstantsBuffer{*device, 1, simultaneousFramesInFlight, 256, HAL::CPUAccessibleHeapType::Upload },
        mPerFrameRootConstantsBuffer{*device, 1, simultaneousFramesInFlight, 256, HAL::CPUAccessibleHeapType::Upload }
    {}

    void PipelineResourceStorage::BeginFrame(uint64_t frameFenceValue)
    {
        for (auto& passBufferPair : mPerPassConstantBuffers)
        {
            auto& buffer = passBufferPair.second;
            buffer->PrepareMemoryForNewFrame(frameFenceValue);
        }

        mGlobalRootConstantsBuffer.PrepareMemoryForNewFrame(frameFenceValue);
        mPerFrameRootConstantsBuffer.PrepareMemoryForNewFrame(frameFenceValue);
    }

    void PipelineResourceStorage::EndFrame(uint64_t completedFrameFenceValue)
    {
        for (auto& passBufferPair : mPerPassConstantBuffers)
        {
            auto& buffer = passBufferPair.second;
            buffer->DiscardMemoryForCompletedFrames(completedFrameFenceValue);
        }

        mGlobalRootConstantsBuffer.DiscardMemoryForCompletedFrames(completedFrameFenceValue);
        mPerFrameRootConstantsBuffer.DiscardMemoryForCompletedFrames(completedFrameFenceValue);
    }

    const HAL::RTDescriptor& PipelineResourceStorage::GetRenderTargetDescriptor(Foundation::Name resourceName) const
    {
        const PipelineResource* pipelineResource = GetPipelineResource(resourceName);
        assert_format(pipelineResource, "Resource ", resourceName.ToString(), " was not scheduled to be used as render target");

        std::optional<HAL::ResourceFormat::Color> format = std::nullopt;

        auto perPassData = pipelineResource->GetMetadataForPass(mCurrentPassName);
        if (perPassData) format = perPassData->ShaderVisibleFormat;

        auto descriptor = mDescriptorStorage->GetRTDescriptor(pipelineResource->Resource.get(), format);

        return *descriptor;
    }

    const HAL::DSDescriptor& PipelineResourceStorage::GetDepthStencilDescriptor(ResourceName resourceName) const
    {
        const PipelineResource* pipelineResource = GetPipelineResource(resourceName);
        assert_format(pipelineResource, "Resource ", resourceName.ToString(), " was not scheduled to be used as depth-stencil target");
        auto descriptor = mDescriptorStorage->GetDSDescriptor(pipelineResource->Resource.get());
        return *descriptor;
    }

    const HAL::RTDescriptor& PipelineResourceStorage::GetCurrentBackBufferDescriptor() const
    {
        return mBackBufferDescriptors[mCurrentBackBufferIndex];
    }

    void PipelineResourceStorage::SetCurrentBackBufferIndex(uint8_t index)
    {
        mCurrentBackBufferIndex = index;
    }

    void PipelineResourceStorage::SetCurrentPassName(PassName passName)
    {
        mCurrentPassName = passName;
    }

    void PipelineResourceStorage::AllocateScheduledResources()
    {
        PrepareAllocationsForOptimization();

        mStateOptimizer.Optimize();

        auto RTDSHeapSize = mRTDSMemoryAliaser.Alias();
        auto nonRTDSHeapSize = mNonRTDSMemoryAliaser.Alias();
        auto bufferHeapSize = mBufferMemoryAliaser.Alias();

        mRTDSHeap = std::make_unique<HAL::Heap>(*mDevice, RTDSHeapSize, HAL::HeapAliasingGroup::RTDSTextures);
        mNonRTDSHeap = std::make_unique<HAL::Heap>(*mDevice, nonRTDSHeapSize, HAL::HeapAliasingGroup::NonRTDSTextures);
        mBufferHeap = std::make_unique<HAL::Heap>(*mDevice, bufferHeapSize, HAL::HeapAliasingGroup::Buffers);

        for (auto& pair : mPipelineResourceAllocations)
        {
            PipelineResourceAllocation& allocation = pair.second;
            allocation.AllocationAction();
        }

        CreateResourceBarriers();
    }

    void PipelineResourceStorage::CreateSwapChainBackBufferDescriptors(const HAL::SwapChain& swapChain)
    {
        for (auto i = 0; i < swapChain.BackBuffers().size(); i++)
        {
            mBackBufferDescriptors.push_back(mDescriptorStorage->EmplaceRTDescriptorIfNeeded(swapChain.BackBuffers()[i].get()));
        }
    }

    GlobalRootConstants* PipelineResourceStorage::GlobalRootConstantData()
    {
        return mGlobalRootConstantsBuffer.At(0);
    }

    PerFrameRootConstants* PipelineResourceStorage::PerFrameRootConstantData()
    {
        return mPerFrameRootConstantsBuffer.At(0);
    }

    bool PipelineResourceStorage::IsResourceAllocationScheduled(ResourceName name) const
    {
        return mPipelineResourceAllocations.find(name) != mPipelineResourceAllocations.end();
    }

    void PipelineResourceStorage::RegisterResourceNameForCurrentPass(ResourceName name)
    {
        mPerPassResourceNames[mCurrentPassName].insert(name);
    }

    PipelineResourceAllocation* PipelineResourceStorage::GetResourceAllocator(ResourceName name)
    {
        return mPipelineResourceAllocations.find(name) != mPipelineResourceAllocations.end()
            ? &mPipelineResourceAllocations.at(name) : nullptr;
    }

    PipelineResourceAllocation* PipelineResourceStorage::QueueTextureAllocationIfNeeded(
        ResourceName resourceName,
        HAL::ResourceFormat::FormatVariant format,
        HAL::ResourceFormat::TextureKind kind,
        const Geometry::Dimensions& dimensions,
        const HAL::ResourceFormat::ClearValue& optimizedClearValue)
    {
        auto it = mPipelineResourceAllocations.find(resourceName);

        if (it != mPipelineResourceAllocations.end())
        {
            return &it->second;
        }

        PassName passThatRequestedAllocation = mCurrentPassName;

        auto [iter, success] = mPipelineResourceAllocations.emplace(resourceName, HAL::ResourceFormat{ *mDevice, format, kind, dimensions, 1, optimizedClearValue });
        PipelineResourceAllocation& allocation = iter->second;

        allocation.AllocationAction = [=, &allocation]()
        {
            HAL::ResourceState expectedStates = allocation.GatherExpectedStates();
            HAL::ResourceState initialState = HAL::ResourceState::Common;
            
            HAL::Heap* heap = nullptr;

            switch (allocation.AliasingInfo.HeapAliasingGroup)
            {
            case HAL::HeapAliasingGroup::RTDSTextures: heap = mRTDSHeap.get(); break;
            case HAL::HeapAliasingGroup::NonRTDSTextures: heap = mNonRTDSHeap.get(); break;
            case HAL::HeapAliasingGroup::Buffers: heap = mBufferHeap.get(); break;
            }

            auto texture = std::make_unique<HAL::TextureResource>(
                *mDevice, *heap, allocation.AliasingInfo.HeapOffset, format, kind,
                dimensions, optimizedClearValue, initialState, expectedStates);

            texture->SetDebugName(resourceName.ToString());

            PipelineResource newResource;
            CreateDescriptors(resourceName, newResource, allocation, *texture);

            newResource.Resource = std::move(texture);
            mPipelineResources[resourceName] = std::move(newResource);
        };

        return &allocation;
    }

    void PipelineResourceStorage::CreateDescriptors(ResourceName resourceName, PipelineResource& resource, const PipelineResourceAllocation& allocator, const HAL::TextureResource& texture)
    {
        for (const auto& [passName, passMetadata] : allocator.AllPassesMetadata())
        {
            PipelineResource::PassMetadata& newResourcePerPassData = resource.AllocateMetadateForPass(passName);

            if (passMetadata.RTInserter)
            {
                newResourcePerPassData.IsRTDescriptorRequested = true;
                (mDescriptorStorage->*passMetadata.RTInserter)(&texture, passMetadata.ShaderVisibleFormat);
            }

            if (passMetadata.DSInserter)
            {
                newResourcePerPassData.IsDSDescriptorRequested = true;
                (mDescriptorStorage->*passMetadata.DSInserter)(&texture);
            }

            if (passMetadata.SRInserter)
            {
                newResourcePerPassData.IsSRDescriptorRequested = true;
                (mDescriptorStorage->*passMetadata.SRInserter)(&texture, passMetadata.ShaderVisibleFormat);
            }

            if (passMetadata.UAInserter)
            {
                newResourcePerPassData.IsUADescriptorRequested = true;
                (mDescriptorStorage->*passMetadata.UAInserter)(&texture, passMetadata.ShaderVisibleFormat);
            }
        }
    }

    void PipelineResourceStorage::PrepareAllocationsForOptimization()
    {
        for (auto& [resourceName, allocation] : mPipelineResourceAllocations)
        {
            mStateOptimizer.AddAllocation(&allocation);

            std::visit(Foundation::MakeVisitor(
                [this, &allocation](const HAL::ResourceFormat::BufferKind& kind)
                {
                    allocation.AliasingInfo.HeapAliasingGroup = HAL::HeapAliasingGroup::Buffers;
                    mBufferMemoryAliaser.AddAllocation(&allocation);
                },
                [this, &allocation](const HAL::ResourceFormat::TextureKind& kind)
                {
                    HAL::ResourceState expectedStates = allocation.GatherExpectedStates();

                    if (EnumMaskBitSet(expectedStates, HAL::ResourceState::RenderTarget) ||
                        EnumMaskBitSet(expectedStates, HAL::ResourceState::DepthWrite) ||
                        EnumMaskBitSet(expectedStates, HAL::ResourceState::DepthRead))
                    {
                        allocation.AliasingInfo.HeapAliasingGroup = HAL::HeapAliasingGroup::RTDSTextures;
                        mRTDSMemoryAliaser.AddAllocation(&allocation);
                    } 
                    else {
                        allocation.AliasingInfo.HeapAliasingGroup = HAL::HeapAliasingGroup::NonRTDSTextures;
                        mNonRTDSMemoryAliaser.AddAllocation(&allocation);
                    }
                }),
                allocation.ResourceFormat().Kind());
        }
    }

    void PipelineResourceStorage::CreateResourceBarriers()
    {
        for (auto& [resourceName, allocation] : mPipelineResourceAllocations)
        {
            const PipelineResource* pipelineResource = GetPipelineResource(resourceName);
            assert_format(pipelineResource, "Resource must be allocated before creating any transitions");

            if (allocation.AliasingInfo.NeedsAliasingBarrier)
            {
                mPerPassResourceBarriers[allocation.FirstPassName()].AddBarrier(HAL::ResourceAliasingBarrier{ nullptr, pipelineResource->Resource.get() });
            }

            if (allocation.OneTimeTransitionStates)
            {
                mOneTimeResourceBarriers.AddBarrier(HAL::ResourceTransitionBarrier{
                        allocation.OneTimeTransitionStates->first, allocation.OneTimeTransitionStates->second, pipelineResource->Resource.get() });
            }

            for (auto& [passName, passData] : allocation.AllPassesMetadata())
            {
                if (passData.OptimizedTransitionStates)
                {
                    mPerPassResourceBarriers[passName].AddBarrier(HAL::ResourceTransitionBarrier{
                        passData.OptimizedTransitionStates->first, passData.OptimizedTransitionStates->second, pipelineResource->Resource.get() });
                }
            }
        }
    }

    HAL::BufferResource<uint8_t>* PipelineResourceStorage::RootConstantBufferForCurrentPass() const
    {
        auto it = mPerPassConstantBuffers.find(mCurrentPassName);
        if (it == mPerPassConstantBuffers.end()) return nullptr;
        return it->second.get();
    }

    const HAL::BufferResource<GlobalRootConstants>& PipelineResourceStorage::GlobalRootConstantsBuffer() const
    {
        return mGlobalRootConstantsBuffer;
    }

    const HAL::BufferResource<PerFrameRootConstants>& PipelineResourceStorage::PerFrameRootConstantsBuffer() const
    {
        return mPerFrameRootConstantsBuffer;
    }

    const std::unordered_set<ResourceName>& PipelineResourceStorage::ScheduledResourceNamesForCurrentPass()
    {
        return mPerPassResourceNames[mCurrentPassName];
    }

    const PathFinder::PipelineResource* PipelineResourceStorage::GetPipelineResource(ResourceName resourceName) const 
    {
        auto it = mPipelineResources.find(resourceName);
        if (it == mPipelineResources.end()) return nullptr;
        return &it->second;
    }

    const HAL::ResourceBarrierCollection& PipelineResourceStorage::OneTimeResourceBarriers() const
    {
        return mOneTimeResourceBarriers;
    }

    const HAL::ResourceBarrierCollection& PipelineResourceStorage::ResourceBarriersForCurrentPass()
    {
        return mPerPassResourceBarriers[mCurrentPassName];
    }

    const Foundation::Name PipelineResourceStorage::CurrentPassName() const
    {
        return mCurrentPassName;
    }

    const PathFinder::ResourceDescriptorStorage* PipelineResourceStorage::DescriptorStorage() const
    {
        return mDescriptorStorage;
    }

}
