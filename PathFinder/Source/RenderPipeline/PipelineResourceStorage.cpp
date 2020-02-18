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
        Memory::ResourceStateTracker* stateTracker,
        const RenderSurfaceDescription& defaultRenderSurface,
        const RenderPassExecutionGraph* passExecutionGraph)
        :
        mDevice{ device },
        mStateOptimizer{ passExecutionGraph },
        mResourceStateTracker{ stateTracker },
        mRTDSMemoryAliaser{ passExecutionGraph },
        mNonRTDSMemoryAliaser{ passExecutionGraph },
        mUniversalMemoryAliaser{ passExecutionGraph },
        mBufferMemoryAliaser{ passExecutionGraph },
        mDefaultRenderSurface{ defaultRenderSurface },
        mResourceProducer{ resourceProducer },
        mDescriptorAllocator{ descriptorAllocator },
        mPassExecutionGraph{ passExecutionGraph } {}

    const HAL::RTDescriptor& PipelineResourceStorage::GetRenderTargetDescriptor(Foundation::Name resourceName)
    {
        PerResourceObjects& resourceObjects = GetPerResourceObjects(resourceName);

        Memory::Texture* texture = resourceObjects.Texture.get();
        assert_format(texture, "Resource ", resourceName.ToString(), " doesn't exist");

        auto perPassData = resourceObjects.SchedulingInfo->GetMetadataForPass(mCurrentPassName);
        assert_format(perPassData && perPassData->CreateTextureRTDescriptor, "Resource ", resourceName.ToString(), " was not scheduled to be used as render target");
        
        return *texture->GetOrCreateRTDescriptor();
    }

    const HAL::DSDescriptor& PipelineResourceStorage::GetDepthStencilDescriptor(ResourceName resourceName)
    {
        PerResourceObjects& resourceObjects = GetPerResourceObjects(resourceName);

        Memory::Texture* texture = resourceObjects.Texture.get();
        assert_format(texture, "Resource ", resourceName.ToString(), " doesn't exist");

        auto perPassData = resourceObjects.SchedulingInfo->GetMetadataForPass(mCurrentPassName);
        assert_format(perPassData && perPassData->CreateTextureDSDescriptor, "Resource ", resourceName.ToString(), " was not scheduled to be used as depth-stencil target");

        return *texture->GetOrCreateDSDescriptor();
    }

    const HAL::RTDescriptor& PipelineResourceStorage::GetCurrentBackBufferDescriptor() const
    {
        return *mBackBufferDescriptors[mCurrentBackBufferIndex];
    }

    void PipelineResourceStorage::SetCurrentBackBufferIndex(uint8_t index)
    {
        mCurrentBackBufferIndex = index;
    }

    void PipelineResourceStorage::SetCurrentPassName(PassName passName, bool isFirstInFrame)
    {
        mCurrentPassName = passName;
        mIsCurrentPassFirstInFrame = isFirstInFrame;
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

        CreateDebugBuffers();
        CreateAliasingAndUAVBarriers();
    }

    void PipelineResourceStorage::CreateSwapChainBackBufferDescriptors(const HAL::SwapChain& swapChain)
    {
        for (auto i = 0; i < swapChain.BackBuffers().size(); i++)
        {
            mBackBufferDescriptors.push_back(mDescriptorAllocator->AllocateRTDescriptor(*swapChain.BackBuffers()[i]));
        }
    }

    bool PipelineResourceStorage::IsResourceAllocationScheduled(ResourceName name) const
    {
        const PerResourceObjects* resourceObjects = GetPerResourceObjects(name);

        if (!resourceObjects)
        {
            return false;
        }

        return resourceObjects->SchedulingInfo.has_value();
    }

    void PipelineResourceStorage::RegisterResourceNameForCurrentPass(ResourceName name)
    {
        GetPerPassObjects(mCurrentPassName).ScheduledResourceNames.insert(name);
    }

    PipelineResourceSchedulingInfo* PipelineResourceStorage::GetResourceSchedulingInfo(ResourceName name)
    {
        PerResourceObjects& resourceObjects = GetPerResourceObjects(name);
        return resourceObjects.SchedulingInfo.has_value() ? &resourceObjects.SchedulingInfo.value() : nullptr;
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
            return &(resourceObjects.SchedulingInfo.value());
        }

        HAL::Texture::Properties textureProperties{ format, kind, dimensions, optimizedClearValue, HAL::ResourceState::Common, mipCount };

        resourceObjects.SchedulingInfo = PipelineResourceSchedulingInfo{ HAL::Texture::ConstructResourceFormat(mDevice, textureProperties) };

        resourceObjects.SchedulingInfo->AllocationAction = [=, &resourceObjects]()
        {
            PipelineResourceSchedulingInfo& schedulingInfo = *resourceObjects.SchedulingInfo;
            HAL::Heap* heap = nullptr;

            switch (schedulingInfo.ResourceFormat().ResourceAliasingGroup())
            {
            case HAL::HeapAliasingGroup::RTDSTextures: heap = mRTDSHeap.get(); break;
            case HAL::HeapAliasingGroup::NonRTDSTextures: heap = mNonRTDSHeap.get(); break;
            case HAL::HeapAliasingGroup::Buffers: heap = mBufferHeap.get(); break;
            case HAL::HeapAliasingGroup::Universal: heap = mUniversalHeap.get(); break;
            }

            HAL::Texture::Properties completeProperties{ 
                format, kind, dimensions, optimizedClearValue, schedulingInfo.InitialStates(), schedulingInfo.ExpectedStates(), mipCount };

            resourceObjects.Texture = mResourceProducer->NewTexture(completeProperties, *heap, schedulingInfo.AliasingInfo.HeapOffset);
            resourceObjects.Texture->SetDebugName(resourceName.ToString());
        };

        return &(resourceObjects.SchedulingInfo.value());
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

    void PipelineResourceStorage::CreateDebugBuffers()
    {
        for (auto& [resourceName, passObjects] : mPerPassObjects)
        {
            HAL::Buffer::Properties<float> properties{ 1024 };
            passObjects.PassDebugBuffer = mResourceProducer->NewBuffer(properties);
            passObjects.PassDebugBuffer->RequestWrite();

            // Avoid garbage on first use
            uint8_t* uploadMemory = passObjects.PassDebugBuffer->WriteOnlyPtr();
            memset(uploadMemory, 0, sizeof(float) * 1024);
        }
    }

    void PipelineResourceStorage::PrepareSchedulingInfoForOptimization()
    {
        for (auto& [resourceName, resourceObjects] : mPerResourceObjects)
        {
            resourceObjects.SchedulingInfo->FinishScheduling();
            mStateOptimizer.AddSchedulingInfo(&(*resourceObjects.SchedulingInfo));

            switch (resourceObjects.SchedulingInfo->ResourceFormat().ResourceAliasingGroup())
            {
            case HAL::HeapAliasingGroup::RTDSTextures: 
                mRTDSMemoryAliaser.AddSchedulingInfo(&(*resourceObjects.SchedulingInfo));
                break;
            case HAL::HeapAliasingGroup::NonRTDSTextures:
                mNonRTDSMemoryAliaser.AddSchedulingInfo(&(*resourceObjects.SchedulingInfo));
                break;
            case HAL::HeapAliasingGroup::Buffers:
                mBufferMemoryAliaser.AddSchedulingInfo(&(*resourceObjects.SchedulingInfo));
                break;
            case HAL::HeapAliasingGroup::Universal:
                mUniversalMemoryAliaser.AddSchedulingInfo(&(*resourceObjects.SchedulingInfo));
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

    void PipelineResourceStorage::TransitionResourcesToCurrentPassStates()
    {
        PerPassObjects& passObjects = GetPerPassObjects(mCurrentPassName);
        for (ResourceName resourceName : passObjects.ScheduledResourceNames)
        {
            PerResourceObjects& resourceObjects = GetPerResourceObjects(resourceName);
            resourceObjects.GetGPUResource()->RequestNewState(resourceObjects.SchedulingInfo->GetMetadataForPass(mCurrentPassName)->OptimizedState);
        }
        
        passObjects.TransitionBarriers = mResourceStateTracker->ApplyRequestedTransitions(mIsCurrentPassFirstInFrame);
    }

    void PipelineResourceStorage::RequestCurrentPassDebugReadback()
    {
        PerPassObjects& passObjects = GetPerPassObjects(mCurrentPassName);
        passObjects.PassDebugBuffer->RequestRead();
    }

    const Memory::Buffer* PipelineResourceStorage::GlobalRootConstantsBuffer() const
    {
        return mGlobalRootConstantsBuffer.get();
    }

    const Memory::Buffer* PipelineResourceStorage::PerFrameRootConstantsBuffer() const
    {
        return mPerFrameRootConstantsBuffer.get();
    }

    const Memory::Buffer* PipelineResourceStorage::RootConstantsBufferForCurrentPass() const
    {
        const PerPassObjects* passObjects = GetPerPassObjects(mCurrentPassName);
        return passObjects->PassConstantBuffer.get();
    }

    const Memory::Buffer* PipelineResourceStorage::DebugBufferForCurrentPass() const
    {
        const PerPassObjects* passObjects = GetPerPassObjects(mCurrentPassName);
        return passObjects->PassDebugBuffer.get();
    }

    const std::unordered_set<ResourceName>& PipelineResourceStorage::ScheduledResourceNamesForCurrentPass()
    {
        PerPassObjects& passObjects = GetPerPassObjects(mCurrentPassName);
        return passObjects.ScheduledResourceNames;
    }

    const Memory::Texture* PipelineResourceStorage::GetTextureResource(ResourceName resourceName) const
    {
        const PerResourceObjects* resourceObjects = GetPerResourceObjects(resourceName);
        return resourceObjects ? resourceObjects->Texture.get() : nullptr;
    }

    const Memory::Buffer* PipelineResourceStorage::GetBufferResource(ResourceName resourceName) const
    {
        const PerResourceObjects* resourceObjects = GetPerResourceObjects(resourceName);
        return resourceObjects ? resourceObjects->Buffer.get() : nullptr;
    }

    const Memory::GPUResource* PipelineResourceStorage::GetGPUResource(ResourceName resourceName) const
    {
        const PerResourceObjects* resourceObjects = GetPerResourceObjects(resourceName);
        return resourceObjects ? resourceObjects->GetGPUResource() : nullptr;
    }

    const HAL::ResourceBarrierCollection& PipelineResourceStorage::AliasingBarriersForCurrentPass() 
    {
        PerPassObjects& passObjects = GetPerPassObjects(mCurrentPassName);
        return passObjects.AliasingBarriers;
    }

    const HAL::ResourceBarrierCollection& PipelineResourceStorage::TransitionBarriersForCurrentPass() 
    {
        PerPassObjects& passObjects = GetPerPassObjects(mCurrentPassName);
        return passObjects.TransitionBarriers;
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

    void PipelineResourceStorage::IterateDebugBuffers(const DebugBufferIteratorFunc& func) const
    {

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
        if (Texture) return Texture.get();
        else if (Buffer) return Buffer.get();
        else return nullptr;
    }

    Memory::GPUResource* PipelineResourceStorage::PerResourceObjects::GetGPUResource()
    {
        if (Texture) return Texture.get();
        else if (Buffer) return Buffer.get();
        else return nullptr;
    }

}
