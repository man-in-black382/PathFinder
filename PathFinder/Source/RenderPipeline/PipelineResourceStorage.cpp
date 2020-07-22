#include "PipelineResourceStorage.hpp"
#include "RenderPass.hpp"
#include "RenderPassGraph.hpp"

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
        const RenderPassGraph* passExecutionGraph)
        :
        mDevice{ device },
        mResourceStateTracker{ stateTracker },
        mRTDSMemoryAliaser{ passExecutionGraph },
        mNonRTDSMemoryAliaser{ passExecutionGraph },
        mUniversalMemoryAliaser{ passExecutionGraph },
        mBufferMemoryAliaser{ passExecutionGraph },
        mDefaultRenderSurface{ defaultRenderSurface },
        mResourceProducer{ resourceProducer },
        mDescriptorAllocator{ descriptorAllocator },
        mPassExecutionGraph{ passExecutionGraph } 
    {
        // Preallocate 
        mGlobalRootConstantsBuffer = mResourceProducer->NewBuffer(
            HAL::Buffer::Properties<uint8_t>{1024, 1, HAL::ResourceState::ConstantBuffer});

        mPerFrameRootConstantsBuffer = mResourceProducer->NewBuffer(
            HAL::Buffer::Properties<uint8_t>{1024, 1, HAL::ResourceState::ConstantBuffer},
            Memory::GPUResource::UploadStrategy::DirectAccess);
    }

    const HAL::RTDescriptor* PipelineResourceStorage::GetRenderTargetDescriptor(Foundation::Name resourceName, Foundation::Name passName, uint64_t mipIndex)
    {
        const PipelineResourceStorageResource* resourceObjects = GetPerResourceData(resourceName);
        const Memory::Texture* texture = resourceObjects->Texture.get();
        assert_format(texture, "Resource ", resourceName.ToString(), " doesn't exist");

        const PipelineResourceSchedulingInfo::PassInfo* passInfo = resourceObjects->SchedulingInfo.GetInfoForPass(passName);
        assert_format(passInfo, "Resource ", resourceName.ToString(), " was not scheduled to be used as render target");

        const std::optional<PipelineResourceSchedulingInfo::SubresourceInfo>& subresourceInfo = passInfo->SubresourceInfos[mipIndex];
        assert_format(subresourceInfo != std::nullopt, "Resource ", resourceName.ToString(), ". Mip ", mipIndex, " was not scheduled to be used as render target");
        
        return texture->GetRTDescriptor(mipIndex);
    }

    const HAL::DSDescriptor* PipelineResourceStorage::GetDepthStencilDescriptor(ResourceName resourceName, Foundation::Name passName)
    {
        const PipelineResourceStorageResource* resourceObjects = GetPerResourceData(resourceName);
        const Memory::Texture* texture = resourceObjects->Texture.get();
        assert_format(texture, "Resource ", resourceName.ToString(), " doesn't exist");

        const PipelineResourceSchedulingInfo::PassInfo* passInfo = resourceObjects->SchedulingInfo.GetInfoForPass(passName);
        assert_format(passInfo && passInfo->SubresourceInfos[0], "Resource ", resourceName.ToString(), " was not scheduled to be used as depth-stencil attachment");

        return texture->GetDSDescriptor();
    }

    bool PipelineResourceStorage::HasMemoryLayoutChange() const
    {
        return mMemoryLayoutChanged;
    }

    void PipelineResourceStorage::CreatePerPassData()
    {
        for (const RenderPassGraph::Node& passNode : mPassExecutionGraph->Nodes())
        {
            CreatePerPassData(passNode.PassMetadata().Name);
        }

        CreateDebugBuffers();
    }

    void PipelineResourceStorage::StartResourceScheduling()
    {
        mPreviousFrameResources->clear();
        mPreviousFrameResourceMap->clear();
        mPreviousFrameDiffEntries->clear();
        mAllocationActions.clear();
        mSchedulingCreationRequests.clear();
        mSchedulingUsageRequests.clear();

        std::swap(mPreviousFrameDiffEntries, mCurrentFrameDiffEntries);
        std::swap(mPreviousFrameResources, mCurrentFrameResources);
        std::swap(mPreviousFrameResourceMap, mCurrentFrameResourceMap);
    }

    void PipelineResourceStorage::EndResourceScheduling()
    {
        FinalizeSchedulingInfo();

        bool memoryValid = TransferPreviousFrameResources();
        mMemoryLayoutChanged = !memoryValid;

        if (memoryValid)
        {
            return;
        }

        // Re-alias memory, then reallocate resources only if memory was invalidated
        // which can happen on first run or when resource properties were changed by the user.
        //
        if (!mRTDSMemoryAliaser.IsEmpty()) mRTDSHeap = std::make_unique<HAL::Heap>(*mDevice, mRTDSMemoryAliaser.Alias(), HAL::HeapAliasingGroup::RTDSTextures);
        if (!mNonRTDSMemoryAliaser.IsEmpty()) mNonRTDSHeap = std::make_unique<HAL::Heap>(*mDevice, mNonRTDSMemoryAliaser.Alias(), HAL::HeapAliasingGroup::NonRTDSTextures);
        if (!mBufferMemoryAliaser.IsEmpty()) mBufferHeap = std::make_unique<HAL::Heap>(*mDevice, mBufferMemoryAliaser.Alias(), HAL::HeapAliasingGroup::Buffers);
        if (!mUniversalMemoryAliaser.IsEmpty()) mUniversalHeap = std::make_unique<HAL::Heap>(*mDevice, mUniversalMemoryAliaser.Alias(), HAL::HeapAliasingGroup::Universal);

        for (auto& allocationAction : mAllocationActions)
        {
            allocationAction();
        }
    }

    void PipelineResourceStorage::QueueTextureAllocationIfNeeded(
        ResourceName resourceName,
        HAL::ResourceFormat::FormatVariant format,
        HAL::TextureKind kind,
        const Geometry::Dimensions& dimensions,
        const HAL::ClearValue& optimizedClearValue,
        uint16_t mipCount,
        const SchedulingInfoConfigurator& siConfigurator)
    {
        HAL::Texture::Properties textureProperties{ format, kind, dimensions, optimizedClearValue, HAL::ResourceState::Common, mipCount };
        HAL::ResourceFormat textureFormat = HAL::Texture::ConstructResourceFormat(mDevice, textureProperties);

        assert_format(!GetPerResourceData(resourceName), "Texture ", resourceName.ToString(), " allocation is already requested");
        CreatePerResourceData(resourceName, textureFormat);

        uint64_t resourceDataIndex = mCurrentFrameResources->size() - 1;

        auto allocationAction = [=]()
        {
            HAL::Heap* heap = nullptr;
            PipelineResourceStorageResource* resourceObjects = &mCurrentFrameResources->at(resourceDataIndex);

            switch (resourceObjects->SchedulingInfo.ResourceFormat().ResourceAliasingGroup())
            {
            case HAL::HeapAliasingGroup::RTDSTextures: heap = mRTDSHeap.get(); break;
            case HAL::HeapAliasingGroup::NonRTDSTextures: heap = mNonRTDSHeap.get(); break;
            case HAL::HeapAliasingGroup::Buffers: heap = mBufferHeap.get(); break;
            case HAL::HeapAliasingGroup::Universal: heap = mUniversalHeap.get(); break;
            }

            HAL::Texture::Properties completeProperties{
                format, kind, dimensions, optimizedClearValue, HAL::ResourceState::Common, resourceObjects->SchedulingInfo.ExpectedStates(), mipCount };

            if (resourceObjects->SchedulingInfo.CanBeAliased)
            {
                resourceObjects->Texture = mResourceProducer->NewTexture(completeProperties, *heap, resourceObjects->SchedulingInfo.HeapOffset);
            }
            else
            {
                resourceObjects->Texture = mResourceProducer->NewTexture(completeProperties);
            }

            resourceObjects->Texture->SetDebugName(resourceName.ToString());
        };

        mAllocationActions.push_back(allocationAction);
        mSchedulingCreationRequests.emplace_back(SchedulingRequest{ siConfigurator, resourceName, std::nullopt });
    }

    void PipelineResourceStorage::QueueResourceUsage(ResourceName resourceName, std::optional<ResourceName> aliasName, const SchedulingInfoConfigurator& siConfigurator)
    {
        mSchedulingUsageRequests.emplace_back(SchedulingRequest{ siConfigurator, resourceName, aliasName });
    }

    PipelineResourceStoragePass& PipelineResourceStorage::CreatePerPassData(PassName name)
    {
        auto [it, success] = mPerPassData.emplace(name, PipelineResourceStoragePass{});
        return it->second;
    }

    PipelineResourceStorageResource& PipelineResourceStorage::CreatePerResourceData(ResourceName name, const HAL::ResourceFormat& resourceFormat)
    {
        PipelineResourceStorageResource& resourceObjects = mCurrentFrameResources->emplace_back(name, resourceFormat);
        mCurrentFrameResourceMap->emplace(name, mCurrentFrameResources->size() - 1);
        return resourceObjects;
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

    void PipelineResourceStorage::FinalizeSchedulingInfo()
    {
        mRTDSMemoryAliaser = { mPassExecutionGraph };
        mNonRTDSMemoryAliaser = { mPassExecutionGraph };
        mBufferMemoryAliaser = { mPassExecutionGraph };
        mUniversalMemoryAliaser = { mPassExecutionGraph };

        auto joinAliasingLifetimes = [this](PipelineResourceStorageResource& resourceData, Foundation::Name resourceName)
        {
            const RenderPassGraph::ResourceUsageTimeline& usageTimeline = mPassExecutionGraph->GetResourceUsageTimeline(resourceName);
            uint64_t start = std::min(resourceData.SchedulingInfo.AliasingLifetime.first, usageTimeline.first);
            uint64_t end = std::max(resourceData.SchedulingInfo.AliasingLifetime.second, usageTimeline.second);
            resourceData.SchedulingInfo.AliasingLifetime = { start, end };
        };

        for (SchedulingRequest& request : mSchedulingCreationRequests)
        {
            uint64_t resourceDataIndex = mCurrentFrameResourceMap->at(request.ResourceName);
            PipelineResourceStorageResource& resourceData = mCurrentFrameResources->at(resourceDataIndex);
            request.Configurator(resourceData.SchedulingInfo);
            joinAliasingLifetimes(resourceData, request.ResourceName);
        }

        for (SchedulingRequest& request : mSchedulingUsageRequests)
        {
            auto indexIt = mCurrentFrameResourceMap->find(request.ResourceName);
            assert_format(indexIt != mCurrentFrameResourceMap->end(), "Trying to use a resource that wasn't created: ", request.ResourceName.ToString());

            PipelineResourceStorageResource& resourceData = mCurrentFrameResources->at(indexIt->second);
            request.Configurator(resourceData.SchedulingInfo);
            joinAliasingLifetimes(resourceData, request.ResourceName);

            if (request.NameAlias)
            {
                mCurrentFrameResourceMap->emplace(*request.NameAlias, indexIt->second);
                resourceData.SchedulingInfo.AddNameAlias(*request.NameAlias);
                joinAliasingLifetimes(resourceData, *request.NameAlias);
            }
        }

        for (PipelineResourceStorageResource& resourceData : *mCurrentFrameResources)
        {
            resourceData.SchedulingInfo.FinishScheduling();

            if (!resourceData.SchedulingInfo.CanBeAliased)
            {
                continue;
            }

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

    bool PipelineResourceStorage::TransferPreviousFrameResources()
    {
        for (PipelineResourceStorageResource& resourceData : *mCurrentFrameResources)
        {
            // Accumulate expected states for resource from previous frame to avoid reallocations 
            // when resource's states ping-pong between frames or change frequently for other reasons.
            auto prevResourceDataIndexIt = mPreviousFrameResourceMap->find(resourceData.ResourceName());
            if (prevResourceDataIndexIt != mPreviousFrameResourceMap->end())
            {
                PipelineResourceStorageResource& previousResourceData = mPreviousFrameResources->at(prevResourceDataIndexIt->second);
                resourceData.SchedulingInfo.AddExpectedStates(previousResourceData.SchedulingInfo.ExpectedStates());
            }

            PipelineResourceStorageResource::DiffEntry diffEntry = resourceData.GetDiffEntry();
            mCurrentFrameDiffEntries->push_back(diffEntry);
        }

        // Make diff independent from order by sorting first
        std::sort(mCurrentFrameDiffEntries->begin(), mCurrentFrameDiffEntries->end(), [](auto& first, auto& second)
        {
            return first.ResourceName.ToId() < second.ResourceName.ToId();
        });

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
                uint64_t indexInPrevFrame = mPreviousFrameResourceMap->at(diffEntry.ResourceName);
                uint64_t indexInCurrFrame = mCurrentFrameResourceMap->at(diffEntry.ResourceName);
                PipelineResourceStorageResource& prevResourceData = mPreviousFrameResources->at(indexInPrevFrame);
                PipelineResourceStorageResource& resourceData = mCurrentFrameResources->at(indexInCurrFrame);

                // Transfer GPU resources from previous frame
                resourceData.Texture = std::move(prevResourceData.Texture);
                resourceData.Buffer = std::move(prevResourceData.Buffer);
            }
                
            default:
                break;
            }
        }

        return true;
    }

    const Memory::Buffer* PipelineResourceStorage::GlobalRootConstantsBuffer() const
    {
        return mGlobalRootConstantsBuffer.get();
    }

    const Memory::Buffer* PipelineResourceStorage::PerFrameRootConstantsBuffer() const
    {
        return mPerFrameRootConstantsBuffer.get();
    }

    PipelineResourceStoragePass* PipelineResourceStorage::GetPerPassData(PassName name)
    {
        auto it = mPerPassData.find(name);
        if (it == mPerPassData.end()) return nullptr;
        return &it->second;
    }

    PipelineResourceStorageResource* PipelineResourceStorage::GetPerResourceData(ResourceName name)
    {
        auto indexIt = mCurrentFrameResourceMap->find(name);
        if (indexIt == mCurrentFrameResourceMap->end()) return nullptr;
        return &mCurrentFrameResources->at(indexIt->second);
    }

    const PipelineResourceStoragePass* PipelineResourceStorage::GetPerPassData(PassName name) const
    {
        auto it = mPerPassData.find(name);
        if (it == mPerPassData.end()) return nullptr;
        return &it->second;
    }

    const PipelineResourceStorageResource* PipelineResourceStorage::GetPerResourceData(ResourceName name) const
    {
        auto indexIt = mCurrentFrameResourceMap->find(name);
        if (indexIt == mCurrentFrameResourceMap->end()) return nullptr;
        return &mCurrentFrameResources->at(indexIt->second);
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
