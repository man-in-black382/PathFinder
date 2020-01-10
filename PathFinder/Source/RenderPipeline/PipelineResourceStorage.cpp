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
    {
        // Create debug buffers
        for (const RenderPass* pass : passExecutionGraph->AllPasses())
        {
            auto& passObjects = GetPerPassObjects(pass->Name());

            // Use Common to leverage automatic state promotion/decay for buffers when transitioning from/to readback and rendering
            passObjects.PassDebugBuffer = std::make_unique<HAL::BufferResource<float>>(
                *device, 1024, 1, HAL::ResourceState::Common, HAL::ResourceState::Common);

            passObjects.PassDebugReadbackBuffer = std::make_unique<HAL::RingBufferResource<float>>(
                *device, 1024, mSimultaneousFramesInFlight, 1, HAL::CPUAccessibleHeapType::Readback);

            passObjects.PassDebugBuffer->SetDebugName(StringFormat("System Memory Debug Buffer (%s)", pass->Name().ToString().c_str()));
            passObjects.PassDebugReadbackBuffer->SetDebugName(StringFormat("Readback Memory Debug Buffer (%s)", pass->Name().ToString().c_str()));
        }
    }

    void PipelineResourceStorage::BeginFrame(uint64_t newFrameNumber)
    {
        for (auto& [passName, passObjects] : mPerPassObjects)
        {
            if (passObjects.PassConstantBuffer)
            {
                passObjects.PassConstantBuffer->PrepareMemoryForNewFrame(newFrameNumber);
            }
        }

        mGlobalRootConstantsBuffer.PrepareMemoryForNewFrame(newFrameNumber);
        mPerFrameRootConstantsBuffer.PrepareMemoryForNewFrame(newFrameNumber);
    }

    void PipelineResourceStorage::EndFrame(uint64_t completedFrameNumber)
    {
        for (auto& [passName, passObjects] : mPerPassObjects)
        {
            if (passObjects.PassConstantBuffer)
            {
                passObjects.PassConstantBuffer->DiscardMemoryForCompletedFrames(completedFrameNumber);
            }
        }

        mGlobalRootConstantsBuffer.DiscardMemoryForCompletedFrames(completedFrameNumber);
        mPerFrameRootConstantsBuffer.DiscardMemoryForCompletedFrames(completedFrameNumber);
    }

    const HAL::RTDescriptor& PipelineResourceStorage::GetRenderTargetDescriptor(Foundation::Name resourceName) const
    {
        const TexturePipelineResource* pipelineResource = GetPipelineTextureResource(resourceName);
        assert_format(pipelineResource, "Resource ", resourceName.ToString(), " was not scheduled to be used as render target");

        std::optional<HAL::ResourceFormat::Color> format = std::nullopt;

        auto perPassData = pipelineResource->GetMetadataForPass(mCurrentPassName);
        if (perPassData) format = perPassData->ShaderVisibleFormat;

        auto descriptor = mDescriptorStorage->GetRTDescriptor(pipelineResource->Resource.get(), format);

        return *descriptor;
    }

    const HAL::DSDescriptor& PipelineResourceStorage::GetDepthStencilDescriptor(ResourceName resourceName) const
    {
        const TexturePipelineResource* pipelineResource = GetPipelineTextureResource(resourceName);
        assert_format(pipelineResource, "Resource ", resourceName.ToString(), " was not scheduled to be used as depth-stencil target");
        auto descriptor = mDescriptorStorage->GetDSDescriptor(pipelineResource->Resource.get());
        return *descriptor;
    }

    const HAL::UADescriptor& PipelineResourceStorage::GetUnorderedAccessDescriptor(Foundation::Name resourceName) const
    {
        const HAL::Resource* resource = GetResource(resourceName);
        assert_format(resource, "Resource ", resourceName.ToString(), " was not scheduled to be used as unordered access resource");
        auto descriptor = mDescriptorStorage->GetUADescriptor(resource);
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

        for (auto& [resourceName, resourceObjects] : mPerResourceObjects)
        {
            resourceObjects.Allocation->AllocationAction();
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
        const PerResourceObjects* resourceObjects = GetPerResourceObjects(name);

        if (!resourceObjects)
        {
            return false;
        }

        return resourceObjects->Allocation != nullptr;
    }

    void PipelineResourceStorage::RegisterResourceNameForCurrentPass(ResourceName name)
    {
        GetPerPassObjects(mCurrentPassName).ScheduledResourceNames.insert(name);
    }

    PipelineResourceAllocation* PipelineResourceStorage::GetResourceAllocator(ResourceName name)
    {
        PerResourceObjects& resourceObjects = GetPerResourceObjects(name);
        return resourceObjects.Allocation.get();
    }

    PipelineResourceAllocation* PipelineResourceStorage::QueueTextureAllocationIfNeeded(
        ResourceName resourceName,
        HAL::ResourceFormat::FormatVariant format,
        HAL::ResourceFormat::TextureKind kind,
        const Geometry::Dimensions& dimensions,
        const HAL::ResourceFormat::ClearValue& optimizedClearValue,
        uint16_t mipCount)
    {
        PerResourceObjects& resourceObjects = GetPerResourceObjects(resourceName);

        if (resourceObjects.Allocation)
        {
            return resourceObjects.Allocation.get();
        }

        resourceObjects.Allocation = std::make_unique<PipelineResourceAllocation>(
            HAL::TextureResource::ConstructResourceFormat(mDevice, format, kind, dimensions, 1, optimizedClearValue));

        resourceObjects.Allocation->AllocationAction = [=, &resourceObjects]()
        {
            PipelineResourceAllocation* allocation = resourceObjects.Allocation.get();
            HAL::Heap* heap = nullptr;

            switch (allocation->AliasingInfo.HeapAliasingGroup)
            {
            case HAL::HeapAliasingGroup::RTDSTextures: heap = mRTDSHeap.get(); break;
            case HAL::HeapAliasingGroup::NonRTDSTextures: heap = mNonRTDSHeap.get(); break;
            default: assert_format(false, "Should never be hit"); return;
            }

            resourceObjects.Texture = std::make_unique<TexturePipelineResource>();

            resourceObjects.Texture->Resource = std::make_unique<HAL::TextureResource>(
                *mDevice, *heap, allocation->AliasingInfo.HeapOffset, format, kind,
                dimensions, optimizedClearValue, allocation->InitialStates(), allocation->ExpectedStates(), mipCount);

            resourceObjects.Texture->Resource->SetDebugName(resourceName.ToString());

            CreateDescriptors(*resourceObjects.Texture, *allocation);
        };

        return resourceObjects.Allocation.get();
    }

    PipelineResourceStorage::PerPassObjects& PipelineResourceStorage::GetPerPassObjects(PassName name)
    {
        return mPerPassObjects[name];
    }

    PipelineResourceStorage::PerResourceObjects& PipelineResourceStorage::GetPerResourceObjects(ResourceName name)
    {
        return mPerResourceObjects[name];
    }

    const PipelineResourceStorage::PerPassObjects* PipelineResourceStorage::GetPerPassObjects(PassName name) const
    {
        auto it = mPerPassObjects.find(name);
        if (it == mPerPassObjects.end()) return nullptr;
        return &it->second;
    }

    const PipelineResourceStorage::PerResourceObjects* PipelineResourceStorage::GetPerResourceObjects(ResourceName name) const
    {
        auto it = mPerResourceObjects.find(name);
        if (it == mPerResourceObjects.end()) return nullptr;
        return &it->second;
    }

    void PipelineResourceStorage::CreateDescriptors(TexturePipelineResource& resource, const PipelineResourceAllocation& allocator)
    {
        for (const auto& [passName, passMetadata] : allocator.AllPassesMetadata())
        {
            TexturePipelineResource::PassMetadata& newResourcePerPassData = resource.AllocateMetadateForPass(passName);

            if (passMetadata.CreateTextureRTDescriptor)
            {
                newResourcePerPassData.IsRTDescriptorRequested = true;
                mDescriptorStorage->EmplaceRTDescriptorIfNeeded(resource.Resource.get(), passMetadata.ShaderVisibleFormat);
            }

            if (passMetadata.CreateTextureDSDescriptor)
            {
                newResourcePerPassData.IsDSDescriptorRequested = true;
                mDescriptorStorage->EmplaceDSDescriptorIfNeeded(resource.Resource.get());
            }

            if (passMetadata.CreateTextureSRDescriptor)
            {
                newResourcePerPassData.IsSRDescriptorRequested = true;
                mDescriptorStorage->EmplaceSRDescriptorIfNeeded(resource.Resource.get(), passMetadata.ShaderVisibleFormat);
            }

            if (passMetadata.CreateTextureUADescriptor)
            {
                newResourcePerPassData.IsUADescriptorRequested = true;
                mDescriptorStorage->EmplaceUADescriptorIfNeeded(resource.Resource.get(), passMetadata.ShaderVisibleFormat);
            }
        }
    }

    void PipelineResourceStorage::CreateDescriptors(BufferPipelineResource& resource, const PipelineResourceAllocation& allocator, uint64_t explicitStride)
    {
        for (const auto& [passName, passMetadata] : allocator.AllPassesMetadata())
        {
            BufferPipelineResource::PassMetadata& newResourcePerPassData = resource.AllocateMetadateForPass(passName);

            if (passMetadata.CreateBufferCBDescriptor)
            {
                newResourcePerPassData.IsCBDescriptorRequested = true;
                mDescriptorStorage->EmplaceCBDescriptorIfNeeded(resource.Resource.get(), explicitStride);
            }

            if (passMetadata.CreateBufferSRDescriptor)
            {
                newResourcePerPassData.IsSRDescriptorRequested = true;
                mDescriptorStorage->EmplaceSRDescriptorIfNeeded(resource.Resource.get(), explicitStride);
            }

            if (passMetadata.CreateBufferUADescriptor)
            {
                newResourcePerPassData.IsUADescriptorRequested = true;
                mDescriptorStorage->EmplaceUADescriptorIfNeeded(resource.Resource.get(), explicitStride);
            }
        }
    }

    void PipelineResourceStorage::PrepareAllocationsForOptimization()
    {
        for (auto& [resourceName, resourceObjects] : mPerResourceObjects)
        {
            PipelineResourceAllocation* allocation = resourceObjects.Allocation.get();
            
            allocation->GatherExpectedStates();
            mStateOptimizer.AddAllocation(resourceObjects.Allocation.get());

            // Dispatch allocations to correct aliasers
            std::visit(Foundation::MakeVisitor(
                [this, allocation](const HAL::ResourceFormat::BufferKind& kind)
                {
                    allocation->AliasingInfo.HeapAliasingGroup = HAL::HeapAliasingGroup::Buffers;
                    mBufferMemoryAliaser.AddAllocation(allocation);
                },
                [this, allocation](const HAL::ResourceFormat::TextureKind& kind)
                {
                    HAL::ResourceState expectedStates = allocation->ExpectedStates();

                    if (EnumMaskBitSet(expectedStates, HAL::ResourceState::RenderTarget) ||
                        EnumMaskBitSet(expectedStates, HAL::ResourceState::DepthWrite) ||
                        EnumMaskBitSet(expectedStates, HAL::ResourceState::DepthRead))
                    {
                        allocation->AliasingInfo.HeapAliasingGroup = HAL::HeapAliasingGroup::RTDSTextures;
                        mRTDSMemoryAliaser.AddAllocation(allocation);
                    }
                    else {
                        allocation->AliasingInfo.HeapAliasingGroup = HAL::HeapAliasingGroup::NonRTDSTextures;
                        mNonRTDSMemoryAliaser.AddAllocation(allocation);
                    }
                }),
                allocation->ResourceFormat().Kind());
        }
    }

    void PipelineResourceStorage::CreateResourceBarriers()
    {
        for (auto& [resourceName, resourceObjects] : mPerResourceObjects)
        {
            const HAL::Resource* resource = resourceObjects.GetResource();
            assert_format(resource, "Resource must be allocated before creating any transitions");
            
            // Create aliasing barriers
            if (resourceObjects.Allocation->AliasingInfo.NeedsAliasingBarrier)
            {
                PerPassObjects& passObjects = GetPerPassObjects(resourceObjects.Allocation->FirstPassName());
                passObjects.TransitionAndAliasingBarriers.AddBarrier(HAL::ResourceAliasingBarrier{ nullptr, resource });
            }

            for (auto& [passName, passData] : resourceObjects.Allocation->AllPassesMetadata())
            {
                PerPassObjects& passObjects = GetPerPassObjects(passName);

                // Create transition barriers
                if (passData.OptimizedTransitionStates)
                {
                    passObjects.TransitionAndAliasingBarriers.AddBarrier(HAL::ResourceTransitionBarrier{
                        passData.OptimizedTransitionStates->first, passData.OptimizedTransitionStates->second, resource });
                }

                // Create unordered access barriers
                if (passData.NeedsUAVBarrier)
                {
                    passObjects.UAVBarriers.AddBarrier(HAL::UnorderedAccessResourceBarrier{ resource });
                }
            }
        }

        // TODO: Create readback barriers for textures when readback functionality for pipeline-induced resources is implemented
        // Barriers for buffers aren't necessary as per DirectX documentation (auto state promotion and decay for buffers)
    }

    HAL::BufferResource<uint8_t>* PipelineResourceStorage::RootConstantBufferForCurrentPass() 
    {
        PerPassObjects& passObjects = GetPerPassObjects(mCurrentPassName);
        return passObjects.PassConstantBuffer.get();
    }

    const HAL::BufferResource<float>* PipelineResourceStorage::DebugBufferForCurrentPass() const
    {
        const PerPassObjects* passObjects = GetPerPassObjects(mCurrentPassName);
        return passObjects->PassDebugBuffer.get();
    }

    const HAL::RingBufferResource<float>* PipelineResourceStorage::DebugReadbackBufferForCurrentPass() const
    {
        const PerPassObjects* passObjects = GetPerPassObjects(mCurrentPassName);
        return passObjects->PassDebugReadbackBuffer.get();
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
        PerPassObjects& passObjects = GetPerPassObjects(mCurrentPassName);
        return passObjects.ScheduledResourceNames;
    }

    const TexturePipelineResource* PipelineResourceStorage::GetPipelineTextureResource(ResourceName resourceName) const
    {
        const PerResourceObjects* resourceObjects = GetPerResourceObjects(resourceName);

        if (!resourceObjects)
        {
            return nullptr;
        }

        return resourceObjects->Texture.get();
    }

    const BufferPipelineResource* PipelineResourceStorage::GetPipelineBufferResource(ResourceName resourceName) const
    {
        const PerResourceObjects* resourceObjects = GetPerResourceObjects(resourceName);

        if (!resourceObjects)
        {
            return nullptr;
        }

        return resourceObjects->Buffer.get();
    }

    const HAL::Resource* PipelineResourceStorage::GetResource(ResourceName resourceName) const
    {
        const PerResourceObjects* resourceObjects = GetPerResourceObjects(resourceName);

        if (!resourceObjects)
        {
            return nullptr;
        }
        else {
            return resourceObjects->GetResource();
        }
    }

    const HAL::ResourceBarrierCollection& PipelineResourceStorage::TransitionAndAliasingBarriersForCurrentPass()
    {
        PerPassObjects& passObjects = GetPerPassObjects(mCurrentPassName);
        return passObjects.TransitionAndAliasingBarriers;
    }

    const HAL::ResourceBarrierCollection& PipelineResourceStorage::UnorderedAccessBarriersForCurrentPass()
    {
        PerPassObjects& passObjects = GetPerPassObjects(mCurrentPassName);
        return passObjects.UAVBarriers;
    }

    const HAL::ResourceBarrierCollection& PipelineResourceStorage::ReadbackBarriers()
    {
        return mReadbackBarriers;
    }

    const Foundation::Name PipelineResourceStorage::CurrentPassName() const
    {
        return mCurrentPassName;
    }

    const PathFinder::ResourceDescriptorStorage* PipelineResourceStorage::DescriptorStorage() const
    {
        return mDescriptorStorage;
    }

    void PipelineResourceStorage::IterateDebugBuffers(const DebugBufferIteratorFunc& func) const
    {
        for (const auto& [passName, passObjects] : mPerPassObjects)
        {
            func(passName, passObjects.PassDebugBuffer.get(), passObjects.PassDebugReadbackBuffer.get());
        }
    }

    const HAL::Resource* PipelineResourceStorage::PerResourceObjects::GetResource() const
    {
        if (Texture) return Texture->Resource.get();
        else if (Buffer) return Buffer->Resource.get();
        else return nullptr;
    }

}
