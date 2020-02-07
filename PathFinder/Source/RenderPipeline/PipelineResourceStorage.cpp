#include "PipelineResourceStorage.hpp"
#include "RenderPass.hpp"
#include "RenderPassExecutionGraph.hpp"

#include "../Foundation/StringUtils.hpp"
#include "../Foundation/Assert.hpp"
#include "../Foundation/STDHelpers.hpp"

namespace PathFinder
{

    PipelineResourceStorage::PipelineResourceStorage(
        HAL::Device* device, 
        Memory::GPUResourceProducer* resourceProducer,
        Memory::PoolDescriptorAllocator* descriptorAllocator,
        const RenderSurfaceDescription& defaultRenderSurface,
        const RenderPassExecutionGraph* passExecutionGraph)
        :
        mDevice{ device },
        mStateOptimizer{ passExecutionGraph },
        mRTDSMemoryAliaser{ passExecutionGraph },
        mNonRTDSMemoryAliaser{ passExecutionGraph },
        mUniversalMemoryAliaser{ passExecutionGraph },
        mBufferMemoryAliaser{ passExecutionGraph },
        mDefaultRenderSurface{ defaultRenderSurface },
        mResourceProducer{ resourceProducer },
        mDescriptorAllocator{ descriptorAllocator }/*,
        mGlobalRootConstantsBuffer{*device, 1, simultaneousFramesInFlight, 256, HAL::CPUAccessibleHeapType::Upload },
        mPerFrameRootConstantsBuffer{*device, 1, simultaneousFramesInFlight, 256, HAL::CPUAccessibleHeapType::Upload }*/
    {
        CreateDebugBuffers(passExecutionGraph);
    }

    const HAL::RTDescriptor& PipelineResourceStorage::GetRenderTargetDescriptor(Foundation::Name resourceName)
    {
        const TexturePipelineResource* pipelineResource = GetPipelineTextureResource(resourceName);
        assert_format(pipelineResource, "Resource ", resourceName.ToString(), " doesn't exist");

        auto perPassData = pipelineResource->GetMetadataForPass(mCurrentPassName);
        assert_format(perPassData && perPassData->IsRTDescriptorRequested, "Resource ", resourceName.ToString(), " was not scheduled to be used as render target");
        
        return *pipelineResource->Resource->GetOrCreateRTDescriptor();
    }

    const HAL::DSDescriptor& PipelineResourceStorage::GetDepthStencilDescriptor(ResourceName resourceName)
    {
        const TexturePipelineResource* pipelineResource = GetPipelineTextureResource(resourceName);
        assert_format(pipelineResource, "Resource ", resourceName.ToString(), " doesn't exist");

        auto perPassData = pipelineResource->GetMetadataForPass(mCurrentPassName);
        assert_format(perPassData && perPassData->IsDSDescriptorRequested, "Resource ", resourceName.ToString(), " was not scheduled to be used as depth-stencil target");

        return *pipelineResource->Resource->GetOrCreateDSDescriptor();
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
        PrepareSchedulingInfoForOptimization();

        mStateOptimizer.Optimize();

        if (!mRTDSMemoryAliaser.IsEmpty()) mRTDSHeap = std::make_unique<HAL::Heap>(*mDevice, mRTDSMemoryAliaser.Alias(), HAL::HeapAliasingGroup::RTDSTextures);
        if (!mNonRTDSMemoryAliaser.IsEmpty()) mNonRTDSHeap = std::make_unique<HAL::Heap>(*mDevice, mNonRTDSMemoryAliaser.Alias(), HAL::HeapAliasingGroup::NonRTDSTextures);
        if (!mBufferMemoryAliaser.IsEmpty()) mBufferHeap = std::make_unique<HAL::Heap>(*mDevice, mBufferMemoryAliaser.Alias(), HAL::HeapAliasingGroup::Buffers);
        if (!mUniversalMemoryAliaser.IsEmpty()) mUniversalHeap = std::make_unique<HAL::Heap>(*mDevice, mUniversalMemoryAliaser.Alias(), HAL::HeapAliasingGroup::Universal);
 
        for (auto& [resourceName, resourceObjects] : mPerResourceObjects)
        {
            resourceObjects.SchedulingInfo->AllocationAction();
        }

        CreateAliasingAndUAVBarriers();
    }

    void PipelineResourceStorage::CreateSwapChainBackBufferDescriptors(const HAL::SwapChain& swapChain)
    {
        for (auto i = 0; i < swapChain.BackBuffers().size(); i++)
        {
            //mBackBufferDescriptors.push_back(mDescriptorStorage->EmplaceRTDescriptorIfNeeded(swapChain.BackBuffers()[i].get()));
        }
    }

 /*   GlobalRootConstants* PipelineResourceStorage::GlobalRootConstantData()
    {
        return mGlobalRootConstantsBuffer.At(0);
    }

    PerFrameRootConstants* PipelineResourceStorage::PerFrameRootConstantData()
    {
        return mPerFrameRootConstantsBuffer.At(0);
    }*/

    bool PipelineResourceStorage::IsResourceAllocationScheduled(ResourceName name) const
    {
        const PerResourceObjects* resourceObjects = GetPerResourceObjects(name);

        if (!resourceObjects)
        {
            return false;
        }

        return resourceObjects->SchedulingInfo != nullptr;
    }

    void PipelineResourceStorage::RegisterResourceNameForCurrentPass(ResourceName name)
    {
        GetPerPassObjects(mCurrentPassName).ScheduledResourceNames.insert(name);
    }

    PipelineResourceSchedulingInfo* PipelineResourceStorage::GetResourceSchedulingInfo(ResourceName name)
    {
        PerResourceObjects& resourceObjects = GetPerResourceObjects(name);
        return resourceObjects.SchedulingInfo.get();
    }

    PipelineResourceSchedulingInfo* PipelineResourceStorage::QueueTextureAllocationIfNeeded(
        ResourceName resourceName,
        HAL::ResourceFormat::FormatVariant format,
        HAL::TextureKind kind,
        const Geometry::Dimensions& dimensions,
        const HAL::ClearValue& optimizedClearValue,
        uint16_t mipCount)
    {
        PerResourceObjects& resourceObjects = GetPerResourceObjects(resourceName);

        if (resourceObjects.SchedulingInfo)
        {
            return resourceObjects.SchedulingInfo.get();
        }

        HAL::Texture::Properties textureProperties{ format, kind, dimensions, optimizedClearValue, HAL::ResourceState::Common, mipCount };

        resourceObjects.SchedulingInfo = std::make_unique<PipelineResourceSchedulingInfo>(
            HAL::Texture::ConstructResourceFormat(mDevice, textureProperties));

        resourceObjects.SchedulingInfo->AllocationAction = [=, &resourceObjects]()
        {
            PipelineResourceSchedulingInfo* schedulingInfo = resourceObjects.SchedulingInfo.get();
            HAL::Heap* heap = nullptr;

            switch (schedulingInfo->ResourceFormat().ResourceAliasingGroup())
            {
            case HAL::HeapAliasingGroup::RTDSTextures: heap = mRTDSHeap.get(); break;
            case HAL::HeapAliasingGroup::NonRTDSTextures: heap = mNonRTDSHeap.get(); break;
            case HAL::HeapAliasingGroup::Buffers: heap = mBufferHeap.get(); break;
            case HAL::HeapAliasingGroup::Universal: heap = mUniversalHeap.get(); break;
            }

            resourceObjects.Texture = std::make_unique<TexturePipelineResource>();

            HAL::Texture::Properties completeProperties{ 
                format, kind, dimensions, optimizedClearValue, schedulingInfo->InitialStates(), schedulingInfo->ExpectedStates(), mipCount };

            resourceObjects.Texture->Resource = mResourceProducer->NewTexture(textureProperties, *heap, schedulingInfo->AliasingInfo.HeapOffset);
            resourceObjects.Texture->Resource->SetDebugName(resourceName.ToString());

            for (const auto& [passName, passMetadata] : schedulingInfo->AllPassesMetadata())
            {
                TexturePipelineResource::PassMetadata& newResourcePerPassData = resourceObjects.Texture->AllocateMetadateForPass(passName);
                newResourcePerPassData.IsRTDescriptorRequested = passMetadata.CreateTextureRTDescriptor;
                newResourcePerPassData.IsDSDescriptorRequested = passMetadata.CreateTextureDSDescriptor;
                newResourcePerPassData.IsSRDescriptorRequested = passMetadata.CreateTextureSRDescriptor;
                newResourcePerPassData.IsUADescriptorRequested = passMetadata.CreateTextureUADescriptor;
                newResourcePerPassData.RequiredState = passMetadata.OptimizedState;
                newResourcePerPassData.ShaderVisibleFormat = passMetadata.ShaderVisibleFormat;
            }
        };

        return resourceObjects.SchedulingInfo.get();
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

    void PipelineResourceStorage::CreateDebugBuffers(const RenderPassExecutionGraph* passExecutionGraph)
    {
        //for (const RenderPass* pass : passExecutionGraph->AllPasses())
        //{
        //    auto& passObjects = GetPerPassObjects(pass->Name());

        //    // Use Common to leverage automatic state promotion/decay for buffers when transitioning from/to readback and rendering
        //    passObjects.PassDebugBuffer = std::make_unique<HAL::Buffer<float>>(
        //        *mDevice, 512, 1, HAL::ResourceState::Common, HAL::ResourceState::UnorderedAccess);

        //    passObjects.PassDebugReadbackBuffer = std::make_unique<HAL::RingBufferResource<float>>(
        //        *mDevice, 512, mSimultaneousFramesInFlight, 1, HAL::CPUAccessibleHeapType::Readback);

        //    passObjects.UAVBarriers.AddBarrier(HAL::UnorderedAccessResourceBarrier{ passObjects.PassDebugBuffer.get() });

        //    passObjects.PassDebugBuffer->SetDebugName(StringFormat("System Memory Debug Buffer (%s)", pass->Name().ToString().c_str()));
        //    passObjects.PassDebugReadbackBuffer->SetDebugName(StringFormat("Readback Memory Debug Buffer (%s)", pass->Name().ToString().c_str()));
        //}
    }

    void PipelineResourceStorage::PrepareSchedulingInfoForOptimization()
    {
        for (auto& [resourceName, resourceObjects] : mPerResourceObjects)
        {
            PipelineResourceSchedulingInfo* schedulingInfo = resourceObjects.SchedulingInfo.get();

            schedulingInfo->GatherExpectedStates();
            mStateOptimizer.AddSchedulingInfo(resourceObjects.SchedulingInfo.get());

            switch (schedulingInfo->ResourceFormat().ResourceAliasingGroup())
            {
            case HAL::HeapAliasingGroup::RTDSTextures: 
                mRTDSMemoryAliaser.AddSchedulingInfo(schedulingInfo);
                break;
            case HAL::HeapAliasingGroup::NonRTDSTextures:
                mNonRTDSMemoryAliaser.AddSchedulingInfo(schedulingInfo);
                break;
            case HAL::HeapAliasingGroup::Buffers:
                mBufferMemoryAliaser.AddSchedulingInfo(schedulingInfo);
                break;
            case HAL::HeapAliasingGroup::Universal:
                mUniversalMemoryAliaser.AddSchedulingInfo(schedulingInfo);
                break;
            }
        }
    }

    void PipelineResourceStorage::CreateAliasingAndUAVBarriers()
    {
        for (auto& [resourceName, resourceObjects] : mPerResourceObjects)
        {
            const Memory::GPUResource* resource = resourceObjects.GetGPUResource();
            assert_format(resource, "Resource must be allocated before creating any transitions");
            
            // Create aliasing barriers
            if (resourceObjects.SchedulingInfo->AliasingInfo.NeedsAliasingBarrier)
            {
                PerPassObjects& passObjects = GetPerPassObjects(resourceObjects.SchedulingInfo->FirstPassName());
                passObjects.AliasingBarriers.AddBarrier(HAL::ResourceAliasingBarrier{ nullptr, resource->HALResource() });
            }

            for (auto& [passName, passData] : resourceObjects.SchedulingInfo->AllPassesMetadata())
            {
                PerPassObjects& passObjects = GetPerPassObjects(passName);

                // Create unordered access barriers
                if (passData.NeedsUAVBarrier)
                {
                    passObjects.UAVBarriers.AddBarrier(HAL::UnorderedAccessResourceBarrier{ resource->HALResource() });
                }
            }
        }
    }

    /* HAL::Buffer<uint8_t>* PipelineResourceStorage::RootConstantBufferForCurrentPass()
    {
        PerPassObjects& passObjects = GetPerPassObjects(mCurrentPassName);
        return passObjects.PassConstantBuffer.get();
    }

    const HAL::Buffer<float>* PipelineResourceStorage::DebugBufferForCurrentPass() const
    {
        const PerPassObjects* passObjects = GetPerPassObjects(mCurrentPassName);
        return passObjects->PassDebugBuffer.get();
    }

    const HAL::RingBufferResource<float>* PipelineResourceStorage::DebugReadbackBufferForCurrentPass() const
    {
        const PerPassObjects* passObjects = GetPerPassObjects(mCurrentPassName);
        return passObjects->PassDebugReadbackBuffer.get();
    }

    const HAL::Buffer<GlobalRootConstants>& PipelineResourceStorage::GlobalRootConstantsBuffer() const
    {
        return mGlobalRootConstantsBuffer;
    }

    const HAL::Buffer<PerFrameRootConstants>& PipelineResourceStorage::PerFrameRootConstantsBuffer() const
    {
        return mPerFrameRootConstantsBuffer;
    }*/

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

    const Memory::GPUResource* PipelineResourceStorage::GetGPUResource(ResourceName resourceName) const
    {
        const PerResourceObjects* resourceObjects = GetPerResourceObjects(resourceName);

        if (!resourceObjects)
        {
            return nullptr;
        }
        else {
            return resourceObjects->GetGPUResource();
        }
    }

    const HAL::ResourceBarrierCollection& PipelineResourceStorage::AliasingBarriersForCurrentPass()
    {
        PerPassObjects& passObjects = GetPerPassObjects(mCurrentPassName);
        return passObjects.AliasingBarriers;
    }

    const HAL::ResourceBarrierCollection& PipelineResourceStorage::UnorderedAccessBarriersForCurrentPass()
    {
        PerPassObjects& passObjects = GetPerPassObjects(mCurrentPassName);
        return passObjects.UAVBarriers;
    }

    const Foundation::Name PipelineResourceStorage::CurrentPassName() const
    {
        return mCurrentPassName;
    }

    //void PipelineResourceStorage::IterateDebugBuffers(const DebugBufferIteratorFunc& func) const
    //{
    //    /*for (const auto& [passName, passObjects] : mPerPassObjects)
    //    {
    //        func(passName, passObjects.PassDebugBuffer.get(), passObjects.PassDebugReadbackBuffer.get());
    //    }*/
    //}

    const Memory::GPUResource* PipelineResourceStorage::PerResourceObjects::GetGPUResource() const
    {
        if (Texture) return Texture->Resource.get();
        else if (Buffer) return Buffer->Resource.get();
        else return nullptr;
    }

}
