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

        auto perPassData = resourceObjects.SchedulingInfo->GetMetadataForPass(mCurrentRenderPassGraphNode.PassMetadata.Name);
        assert_format(perPassData && perPassData->CreateTextureRTDescriptor, "Resource ", resourceName.ToString(), " was not scheduled to be used as render target");
        
        return *texture->GetOrCreateRTDescriptor();
    }

    const HAL::DSDescriptor& PipelineResourceStorage::GetDepthStencilDescriptor(ResourceName resourceName)
    {
        PerResourceObjects& resourceObjects = GetPerResourceObjects(resourceName);

        Memory::Texture* texture = resourceObjects.Texture.get();
        assert_format(texture, "Resource ", resourceName.ToString(), " doesn't exist");

        auto perPassData = resourceObjects.SchedulingInfo->GetMetadataForPass(mCurrentRenderPassGraphNode.PassMetadata.Name);
        assert_format(perPassData && perPassData->CreateTextureDSDescriptor, "Resource ", resourceName.ToString(), " was not scheduled to be used as depth-stencil target");

        return *texture->GetOrCreateDSDescriptor();
    }

    void PipelineResourceStorage::SetCurrentRenderPassGraphNode(const RenderPassExecutionGraph::Node& node)
    {
        mCurrentRenderPassGraphNode = node;
        mCurrentPassObjects = &GetPerPassObjects(mCurrentRenderPassGraphNode.PassMetadata.Name);
    }

    void PipelineResourceStorage::AllocateScheduledResources()
    {
        PrepareSchedulingInfoForOptimization();

        // Prepare empty per-pass objects
        for (auto& passNode : mPassExecutionGraph->AllPasses())
        {
            mPerPassObjects.emplace(passNode.PassMetadata.Name, PerPassObjects{});
        }

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
        mCurrentPassObjects->ScheduledResourceNames.insert(name);
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

        resourceObjects.SchedulingInfo = PipelineResourceSchedulingInfo{ HAL::Texture::ConstructResourceFormat(mDevice, textureProperties), resourceName };

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

            resourceObjects.Texture = mResourceProducer->NewTexture(completeProperties/*, *heap, schedulingInfo.AliasingInfo.HeapOffset*/);
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
        for (auto& [passName, passObjects] : mPerPassObjects)
        {
            HAL::Buffer::Properties<float> properties{ 1024 };
            passObjects.PassDebugBuffer = mResourceProducer->NewBuffer(properties);
            passObjects.PassDebugBuffer->SetDebugName(passName.ToString() + " Debug Constant Buffer");
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
                PerPassObjects& passObjects = GetPerPassObjects(resourceObjects.SchedulingInfo->FirstPassGraphNode().PassMetadata.Name);
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

    void PipelineResourceStorage::RequestResourceTransitionsToCurrentPassStates()
    {
        for (ResourceName resourceName : mCurrentPassObjects->ScheduledResourceNames)
        {
            PerResourceObjects& resourceObjects = GetPerResourceObjects(resourceName);
            HAL::ResourceState newState = resourceObjects.SchedulingInfo->GetMetadataForPass(mCurrentRenderPassGraphNode.PassMetadata.Name)->OptimizedState;
            resourceObjects.GetGPUResource()->RequestNewState(newState);
        }
    }

    void PipelineResourceStorage::RequestCurrentPassDebugReadback()
    {
        mCurrentPassObjects->PassDebugBuffer->RequestRead();
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
        return mCurrentPassObjects->PassConstantBuffer.get();
    }

    const Memory::Buffer* PipelineResourceStorage::DebugBufferForCurrentPass() const
    {
        return mCurrentPassObjects->PassDebugBuffer.get();
    }

    const std::unordered_set<ResourceName>& PipelineResourceStorage::ScheduledResourceNamesForCurrentPass()
    {
        return mCurrentPassObjects->ScheduledResourceNames;
    }

    Memory::Texture* PipelineResourceStorage::GetTextureResource(ResourceName resourceName)
    {
        PerResourceObjects& resourceObjects = GetPerResourceObjects(resourceName);
        return resourceObjects.Texture.get();
    }

    Memory::Buffer* PipelineResourceStorage::GetBufferResource(ResourceName resourceName)
    {
        PerResourceObjects& resourceObjects = GetPerResourceObjects(resourceName);
        return resourceObjects.Buffer.get();
    }

    Memory::GPUResource* PipelineResourceStorage::GetGPUResource(ResourceName resourceName)
    {
        PerResourceObjects& resourceObjects = GetPerResourceObjects(resourceName);
        return resourceObjects.GetGPUResource();
    }

    const HAL::ResourceBarrierCollection& PipelineResourceStorage::AliasingBarriersForCurrentPass() 
    {
        PerPassObjects& passObjects = GetPerPassObjects(mCurrentRenderPassGraphNode.PassMetadata.Name);
        return passObjects.AliasingBarriers;
    }

    const HAL::ResourceBarrierCollection& PipelineResourceStorage::UnorderedAccessBarriersForCurrentPass() 
    {
        PerPassObjects& passObjects = GetPerPassObjects(mCurrentRenderPassGraphNode.PassMetadata.Name);
        return passObjects.UAVBarriers;
    }

    const RenderPassExecutionGraph::Node& PipelineResourceStorage::CurrentPassGraphNode() const
    {
        return mCurrentRenderPassGraphNode;
    }

    void PipelineResourceStorage::IterateDebugBuffers(const DebugBufferIteratorFunc& func) const
    {
        for (auto& [resourceName, passObjects] : mPerPassObjects)
        {
            passObjects.PassDebugBuffer->Read<float>([&func, resourceName](const float* debugData)
            {
                func(resourceName, debugData);
            });
        }
    }

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
