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

    const HAL::RTDescriptor* PipelineResourceStorage::GetRenderTargetDescriptor(Foundation::Name resourceName, uint64_t resourceIndex, uint64_t mipIndex)
    {
        const PipelineResourceStorageResource* resourceObjects = GetPerResourceData(resourceName);
        const Memory::Texture* texture = resourceObjects->GetTexture(resourceIndex);
        assert_format(texture, "Resource ", resourceName.ToString(), " doesn't exist");

        auto perPassData = resourceObjects->SchedulingInfo.GetInfoForPass(mCurrentRenderPassGraphNode.PassMetadata.Name, resourceIndex, mipIndex);
        assert_format(perPassData && perPassData->IsTextureRTRequested(), "Resource ", resourceName.ToString(), " was not scheduled to be used as render target");
        
        return texture->GetRTDescriptor(mipIndex);
    }

    const HAL::DSDescriptor* PipelineResourceStorage::GetDepthStencilDescriptor(ResourceName resourceName, uint64_t resourceIndex)
    {
        const PipelineResourceStorageResource* resourceObjects = GetPerResourceData(resourceName);
        const Memory::Texture* texture = resourceObjects->GetTexture(resourceIndex);
        assert_format(texture, "Resource ", resourceName.ToString(), " doesn't exist");

        auto perPassData = resourceObjects->SchedulingInfo.GetInfoForPass(mCurrentRenderPassGraphNode.PassMetadata.Name, resourceIndex, 0);
        assert_format(perPassData && perPassData->IsTextureDSRequested(), "Resource ", resourceName.ToString(), " was not scheduled to be used as depth-stencil target");

        return texture->GetDSDescriptor();
    }

    void PipelineResourceStorage::SetCurrentRenderPassGraphNode(const RenderPassExecutionGraph::Node& node)
    {
        mCurrentRenderPassGraphNode = node;
        mCurrentPassData = GetPerPassData(mCurrentRenderPassGraphNode.PassMetadata.Name);
        mCurrentPassData->PassConstantBufferMemoryOffset = 0;
        mCurrentPassData->LastSetConstantBufferDataSize = 0;
        mCurrentPassData->IsAllowedToAdvanceConstantBufferOffset = false;
    }

    void PipelineResourceStorage::CommitRenderPasses()
    {
        for (auto& passNode : mPassExecutionGraph->AllPasses())
        {
            CreatePerPassData(passNode.PassMetadata.Name);
        }

        CreateDebugBuffers();
    }

    void PipelineResourceStorage::StartResourceScheduling()
    {
        mPreviousFrameResources->clear();
        mPreviousFrameDiffEntries->clear();
        mResourceCreationRequests.clear();
        mResourceUsageRequests.clear();
        mResourceCreationRequestTracker.clear();

        std::swap(mPreviousFrameDiffEntries, mCurrentFrameDiffEntries);
        std::swap(mPreviousFrameResources, mCurrentFrameResources);

        for (auto& [passName, passData] : mPerPassData)
        {
            passData.ScheduledResourceNames.clear();
        }
    }

    void PipelineResourceStorage::EndResourceScheduling()
    {
        for (const DelayedSchedulingAction& action : mResourceCreationRequests) action();
        for (const DelayedSchedulingAction& action : mResourceUsageRequests) action();

        FinalizeSchedulingInfo();

        bool memoryValid = TransferPreviousFrameResources();

        if (!memoryValid)
        {
            // Re-alias memory, then reallocate resources and then create new aliasing barriers only if memory was invalidated
            // which can happen on first run or when resource properties were changed by the user.
            //
            if (!mRTDSMemoryAliaser.IsEmpty()) mRTDSHeap = std::make_unique<HAL::Heap>(*mDevice, mRTDSMemoryAliaser.Alias(), HAL::HeapAliasingGroup::RTDSTextures);
            if (!mNonRTDSMemoryAliaser.IsEmpty()) mNonRTDSHeap = std::make_unique<HAL::Heap>(*mDevice, mNonRTDSMemoryAliaser.Alias(), HAL::HeapAliasingGroup::NonRTDSTextures);
            if (!mBufferMemoryAliaser.IsEmpty()) mBufferHeap = std::make_unique<HAL::Heap>(*mDevice, mBufferMemoryAliaser.Alias(), HAL::HeapAliasingGroup::Buffers);
            if (!mUniversalMemoryAliaser.IsEmpty()) mUniversalHeap = std::make_unique<HAL::Heap>(*mDevice, mUniversalMemoryAliaser.Alias(), HAL::HeapAliasingGroup::Universal);

            for (auto& [resourceName, resourceObjects] : *mCurrentFrameResources)
            {
                resourceObjects.SchedulingInfo.AllocationAction();
            }

            CreateAliasingBarriers();
        }

        // Assume resource states change every frame
        mStateOptimizer.Optimize();
        CreateUAVBarriers();
    }

    PipelineResourceStorageResource& PipelineResourceStorage::QueueTexturesAllocationIfNeeded(
        ResourceName resourceName,
        HAL::ResourceFormat::FormatVariant format,
        HAL::TextureKind kind,
        const Geometry::Dimensions& dimensions,
        const HAL::ClearValue& optimizedClearValue,
        uint16_t mipCount,
        uint64_t textureCount)
    {
        HAL::Texture::Properties textureProperties{ format, kind, dimensions, optimizedClearValue, HAL::ResourceState::Common, mipCount };
        HAL::ResourceFormat textureFormat = HAL::Texture::ConstructResourceFormat(mDevice, textureProperties);

        PipelineResourceStorageResource* resourceObjects = GetPerResourceData(resourceName);

        if (resourceObjects)
        {
            return *resourceObjects;
        }

        resourceObjects = &CreatePerResourceData(resourceName, textureFormat, textureCount);

        resourceObjects->SchedulingInfo.AllocationAction = [=]()
        {
            HAL::Heap* heap = nullptr;

            switch (resourceObjects->SchedulingInfo.ResourceFormat().ResourceAliasingGroup())
            {
            case HAL::HeapAliasingGroup::RTDSTextures: heap = mRTDSHeap.get(); break;
            case HAL::HeapAliasingGroup::NonRTDSTextures: heap = mNonRTDSHeap.get(); break;
            case HAL::HeapAliasingGroup::Buffers: heap = mBufferHeap.get(); break;
            case HAL::HeapAliasingGroup::Universal: heap = mUniversalHeap.get(); break;
            }

            HAL::Texture::Properties completeProperties{
                format, kind, dimensions, optimizedClearValue, resourceObjects->SchedulingInfo.InitialStates(), resourceObjects->SchedulingInfo.ExpectedStates(), mipCount };

            for (auto textureIdx = 0u; textureIdx < textureCount; ++textureIdx)
            {
                if (resourceObjects->SchedulingInfo.MemoryAliasingInfo.IsAliased)
                {
                    resourceObjects->Textures.emplace_back(mResourceProducer->NewTexture(completeProperties, *heap, resourceObjects->SchedulingInfo.MemoryAliasingInfo.HeapOffset));
                }
                else
                {
                    resourceObjects->Textures.emplace_back(mResourceProducer->NewTexture(completeProperties));
                }

                std::string debugName = resourceName.ToString() + (textureCount > 1 ? ("[" + std::to_string(textureIdx) + "]") : "");
                resourceObjects->Textures.back()->SetDebugName(debugName);
            }
        };

        return *resourceObjects;
    }

    PipelineResourceStoragePass* PipelineResourceStorage::GetPerPassData(PassName name)
    {
        auto it = mPerPassData.find(name);
        if (it == mPerPassData.end()) return nullptr;
        return &it->second;
    }

    PipelineResourceStorageResource* PipelineResourceStorage::GetPerResourceData(ResourceName name)
    {
        auto it = mCurrentFrameResources->find(name);
        if (it == mCurrentFrameResources->end()) return nullptr;
        return &it->second;
    }

    const PipelineResourceStoragePass* PipelineResourceStorage::GetPerPassData(PassName name) const
    {
        auto it = mPerPassData.find(name);
        if (it == mPerPassData.end()) return nullptr;
        return &it->second;
    }

    const PipelineResourceStorageResource* PipelineResourceStorage::GetPerResourceData(ResourceName name) const
    {
        auto it = mCurrentFrameResources->find(name);
        if (it == mCurrentFrameResources->end()) return nullptr;
        return &it->second;
    }

    PipelineResourceStoragePass& PipelineResourceStorage::CreatePerPassData(PassName name)
    {
        auto [it, success] = mPerPassData.emplace(name, PipelineResourceStoragePass{});
        return it->second;
    }

    PipelineResourceStorageResource& PipelineResourceStorage::CreatePerResourceData(ResourceName name, const HAL::ResourceFormat& resourceFormat, uint64_t resourceCount)
    {
        auto [it, success] = mCurrentFrameResources->emplace(name, PipelineResourceStorageResource{ name, resourceFormat, resourceCount });
        return it->second;
    }

    void PipelineResourceStorage::CreateDebugBuffers()
    {
        for (auto& [passName, passObjects] : mPerPassData)
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

    bool PipelineResourceStorage::TransferPreviousFrameResources()
{
        for (auto& [resourceName, resourceData] : *mCurrentFrameResources)
        {
            mCurrentFrameDiffEntries->push_back(resourceData.GetDiffEntry());
        }

        dtl::Diff<PipelineResourceStorageResource::DiffEntry> diff{ *mPreviousFrameDiffEntries, *mCurrentFrameDiffEntries };

        diff.compose();
        dtl::Ses ses = diff.getSes();
        auto sequence = ses.getSequence();

        if (ses.isChange())
        {
            // Any ADD or DELETE will invalidate aliased memory layout
            // so we'll need to reallocate everything.
            // Return because there is nothing to transfer from previous frame:
            // whole memory is invalidated.
            return false;
        }

        for (auto& [diffEntry, elementInfo] : sequence)
        {
            dtl::edit_t diffOperation = elementInfo.type;

            switch (diffOperation)
            {
            case dtl::SES_COMMON:
            {
                // COMMON case means resource should be transfered from previous frame
                PipelineResourceStorageResource& resourceData = mCurrentFrameResources->at(diffEntry.ResourceName);
                PipelineResourceStorageResource& prevResourceData = mPreviousFrameResources->at(diffEntry.ResourceName);

                // Transfer GPU resources from previous frame
                resourceData.Textures = std::move(prevResourceData.Textures);
                resourceData.Buffers = std::move(prevResourceData.Buffers);
            }
                
            default:
                break;
            }
        }

        return true;
    }

    void PipelineResourceStorage::CreateAliasingBarriers()
    {
        // Clear barriers before creating new ones
        for (auto& [passName, passData] : mPerPassData)
        {
            passData.AliasingBarriers = {};
        }

        // Go through every resources scheduled for the frame
        for (auto& [resourceName, resourceObjects] : *mCurrentFrameResources)
        {
            // Go through every GPU resource in scheduled resource's array
            for (auto resourceIdx = 0u; resourceIdx < resourceObjects.ResourceCount(); ++resourceIdx)
            {
                const Memory::GPUResource* resource = resourceObjects.GetGPUResource(resourceIdx);
                assert_format(resource, "Resource must be allocated before creating any transitions");

                // Insert aliasing barriers in render passes this resource is first used in
                if (resourceObjects.SchedulingInfo.MemoryAliasingInfo.NeedsAliasingBarrier)
                {
                    PipelineResourceStoragePass* passObjects = GetPerPassData(resourceObjects.SchedulingInfo.FirstPassGraphNode().PassMetadata.Name);
                    passObjects->AliasingBarriers.AddBarrier(HAL::ResourceAliasingBarrier{ nullptr, resource->HALResource() });
                }
            }
        }
    }

    void PipelineResourceStorage::CreateUAVBarriers()
    {
        // Clear barriers before creating new ones
        for (auto& [passName, passData] : mPerPassData)
        {
            passData.UAVBarriers = {};
        }

        // Go through every resources scheduled for the frame
        for (auto& [resourceName, resourceObjects] : *mCurrentFrameResources)
        {
            // Go through every GPU resource in scheduled resource's array
            for (auto resourceIdx = 0u; resourceIdx < resourceObjects.ResourceCount(); ++resourceIdx)
            {
                const Memory::GPUResource* resource = resourceObjects.GetGPUResource(resourceIdx);
                assert_format(resource, "Resource must be allocated before creating any transitions");

                // Go through all scheduled render passes and add barriers for this resource where needed
                for (auto& [passName, passData] : mPerPassData)
                {
                    // Go through every subresource
                    for (auto subresourceIdx = 0u; subresourceIdx < resourceObjects.SchedulingInfo.SubresourceCount(); ++subresourceIdx)
                    {
                        auto resourcePassMetadata = resourceObjects.SchedulingInfo.GetInfoForPass(passName, resourceIdx, subresourceIdx);

                        // If resource is scheduled for usage in this render pass and is used as a UAV then we create a corresponding barrier
                        if (resourcePassMetadata && resourcePassMetadata->NeedsUAVBarrier)
                        {
                            // If at least one subresource is used as UAV, issue a UAV barrier for the whole resource and break
                            passData.UAVBarriers.AddBarrier(HAL::UnorderedAccessResourceBarrier{ resource->HALResource() });
                            break;
                        }
                    }
                }
            }
        }
    }

    void PipelineResourceStorage::FinalizeSchedulingInfo()
    {
        mStateOptimizer = { mPassExecutionGraph };
        mRTDSMemoryAliaser = { mPassExecutionGraph };
        mNonRTDSMemoryAliaser = { mPassExecutionGraph };
        mBufferMemoryAliaser = { mPassExecutionGraph };
        mUniversalMemoryAliaser = { mPassExecutionGraph };

        for (auto& [resourceName, resourceData] : *mCurrentFrameResources)
        {
            resourceData.SchedulingInfo.FinishScheduling();

            mStateOptimizer.AddSchedulingInfo(&resourceData.SchedulingInfo);

            // Resource arrays are usually used across frames which makes them unaliasable
            bool shouldAlias = false;// resourceData.ResourceCount() == 1;

            if (shouldAlias)
            {
                switch (resourceData.SchedulingInfo.ResourceFormat().ResourceAliasingGroup())
                {
                case HAL::HeapAliasingGroup::RTDSTextures:
                    mRTDSMemoryAliaser.AddSchedulingInfo(&resourceData.SchedulingInfo);
                    break;
                case HAL::HeapAliasingGroup::NonRTDSTextures:
                    mNonRTDSMemoryAliaser.AddSchedulingInfo(&resourceData.SchedulingInfo);
                    break;
                case HAL::HeapAliasingGroup::Buffers:
                    mBufferMemoryAliaser.AddSchedulingInfo(&resourceData.SchedulingInfo);
                    break;
                case HAL::HeapAliasingGroup::Universal:
                    mUniversalMemoryAliaser.AddSchedulingInfo(&resourceData.SchedulingInfo);
                    break;
                }
            }
        }
    }

    void PipelineResourceStorage::RequestResourceTransitionsToCurrentPassStates()
    {
        Memory::ResourceStateTracker::SubresourceStateList stateList{};

        for (ResourceName resourceName : mCurrentPassData->ScheduledResourceNames)
        {
            PipelineResourceStorageResource* resourceData = GetPerResourceData(resourceName);

            for (auto resourceIdx = 0u; resourceIdx < resourceData->ResourceCount(); ++resourceIdx)
            {
                for (auto subresourceIdx = 0u; subresourceIdx < resourceData->SchedulingInfo.SubresourceCount(); ++subresourceIdx)
                {
                    if (auto passMetadata = resourceData->SchedulingInfo.GetInfoForPass(mCurrentRenderPassGraphNode.PassMetadata.Name, resourceIdx, subresourceIdx))
                    {
                        HAL::ResourceState newState = passMetadata->OptimizedState;
                        stateList.push_back({ subresourceIdx, newState });
                    }
                }

                if (!stateList.empty())
                {
                    resourceData->GetGPUResource(resourceIdx)->RequestNewSubresourceStates(stateList);
                    stateList.clear();
                }
            }
        }
    }

    void PipelineResourceStorage::RequestCurrentPassDebugReadback()
    {
        mCurrentPassData->PassDebugBuffer->RequestRead();
    }

    void PipelineResourceStorage::AllowCurrentPassConstantBufferSingleOffsetAdvancement()
    {
        mCurrentPassData->IsAllowedToAdvanceConstantBufferOffset = true;
    }

    const Memory::Buffer* PipelineResourceStorage::GlobalRootConstantsBuffer() const
    {
        return mGlobalRootConstantsBuffer.get();
    }

    const Memory::Buffer* PipelineResourceStorage::PerFrameRootConstantsBuffer() const
    {
        return mPerFrameRootConstantsBuffer.get();
    }

    const Memory::Buffer* PipelineResourceStorage::DebugBufferForCurrentPass() const
    {
        return mCurrentPassData->PassDebugBuffer.get();
    }

    HAL::GPUAddress PipelineResourceStorage::RootConstantsBufferAddressForCurrentPass() const
    {
        if (auto buffer = mCurrentPassData->PassConstantBuffer.get())
        {
            return buffer->HALBuffer()->GPUVirtualAddress() + mCurrentPassData->PassConstantBufferMemoryOffset;
        }

        return 0;
    }

    const HAL::ResourceBarrierCollection& PipelineResourceStorage::AliasingBarriersForCurrentPass() 
    {
        PipelineResourceStoragePass* passObjects = GetPerPassData(mCurrentRenderPassGraphNode.PassMetadata.Name);
        return passObjects->AliasingBarriers;
    }

    const HAL::ResourceBarrierCollection& PipelineResourceStorage::UnorderedAccessBarriersForCurrentPass() 
    {
        PipelineResourceStoragePass* passObjects = GetPerPassData(mCurrentRenderPassGraphNode.PassMetadata.Name);
        return passObjects->UAVBarriers;
    }

    const RenderPassExecutionGraph::Node& PipelineResourceStorage::CurrentPassGraphNode() const
    {
        return mCurrentRenderPassGraphNode;
    }

    void PipelineResourceStorage::AddResourceCreationAction(const DelayedSchedulingAction& action, ResourceName resourceName, PassName passName)
    {
        auto trackerIt = mResourceCreationRequestTracker.find(resourceName);
        assert_format(trackerIt == mResourceCreationRequestTracker.end(),
            "Resource ", resourceName.ToString(), " creation was already requested in ", passName.ToString(), " render pass");

        mResourceCreationRequestTracker.emplace(resourceName, passName);
        mResourceCreationRequests.push_back(action);
    }

    void PipelineResourceStorage::AddResourceUsageAction(const DelayedSchedulingAction& action)
    {
        mResourceUsageRequests.push_back(action);
    }

    void PipelineResourceStorage::IterateDebugBuffers(const DebugBufferIteratorFunc& func) const
    {
        /*for (auto& [resourceName, passObjects] : mPerPassData)
        {
            passObjects.PassDebugBuffer->Read<float>([&func, resourceName](const float* debugData)
            {
                func(resourceName, debugData);
            });
        }*/
    }

}
