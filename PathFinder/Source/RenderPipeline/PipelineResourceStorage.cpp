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
            HAL::BufferProperties<uint8_t>{1024, 1, HAL::ResourceState::ConstantBuffer});

        mPerFrameRootConstantsBuffer = mResourceProducer->NewBuffer(
            HAL::BufferProperties<uint8_t>{1024, 1, HAL::ResourceState::ConstantBuffer},
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

    void PipelineResourceStorage::BeginFrame()
    {
        mPreviousFrameResources->clear();
        mPreviousFrameResourceMap->clear();
        mPreviousFrameDiffEntries->clear();
        mAliasMap.clear();

        std::swap(mPreviousFrameDiffEntries, mCurrentFrameDiffEntries);
        std::swap(mPreviousFrameResources, mCurrentFrameResources);
        std::swap(mPreviousFrameResourceMap, mCurrentFrameResourceMap);
    }

    void PipelineResourceStorage::EndFrame()
    {

    }

    bool PipelineResourceStorage::HasMemoryLayoutChange() const
    {
        return mMemoryLayoutChanged;
    }

    void PipelineResourceStorage::StartResourceScheduling()
    {
        mSchedulingCreationRequests.clear();
        mSchedulingUsageRequests.clear();
    }

    void PipelineResourceStorage::EndResourceScheduling()
    {
        for (SchedulingRequest& request : mSchedulingCreationRequests)
        {
            uint64_t resourceDataIndex = mCurrentFrameResourceMap->at(request.ResourceName);
            PipelineResourceStorageResource& resourceData = mCurrentFrameResources->at(resourceDataIndex);
            request.Configurator(resourceData.SchedulingInfo);
        }

        std::vector<ResourceName> aliases;

        for (SchedulingRequest& request : mSchedulingUsageRequests)
        {
            aliases.clear();

            ResourceName resourceName = request.ResourceName;

            // If resource name is an alias
            auto aliasIt = mAliasMap.find(resourceName);

            while (aliasIt != mAliasMap.end())
            {
                aliases.push_back(resourceName);
                // Take next name in chain
                resourceName = aliasIt->second;
                // See whether that name is also an alias
                aliasIt = mAliasMap.find(resourceName);
            }

            auto indexIt = mCurrentFrameResourceMap->find(resourceName);
            assert_format(indexIt != mCurrentFrameResourceMap->end(), "Trying to use a resource that wasn't created: ", resourceName.ToString());

            PipelineResourceStorageResource& resourceData = mCurrentFrameResources->at(indexIt->second);
            request.Configurator(resourceData.SchedulingInfo);

            // Associate aliases with original resource
            for (ResourceName alias : aliases)
            {
                mCurrentFrameResourceMap->emplace(alias, indexIt->second);
                resourceData.SchedulingInfo.AddNameAlias(alias);
            }
        }
    }

    void PipelineResourceStorage::AllocateScheduledResources()
    {
        mRTDSMemoryAliaser = { mPassExecutionGraph };
        mNonRTDSMemoryAliaser = { mPassExecutionGraph };
        mBufferMemoryAliaser = { mPassExecutionGraph };
        mUniversalMemoryAliaser = { mPassExecutionGraph };

        // Determine resource effective lifetimes
        auto joinAliasingLifetimes = [this](PipelineResourceStorageResource& resourceData, Foundation::Name resourceName)
        {
            const RenderPassGraph::ResourceUsageTimeline& usageTimeline = mPassExecutionGraph->GetResourceUsageTimeline(resourceName);
            uint64_t start = std::min(resourceData.SchedulingInfo.AliasingLifetime.first, usageTimeline.first);
            uint64_t end = std::max(resourceData.SchedulingInfo.AliasingLifetime.second, usageTimeline.second);
            resourceData.SchedulingInfo.AliasingLifetime = { start, end };
        };

        for (PipelineResourceStorageResource& resourceData : *mCurrentFrameResources)
        {
            joinAliasingLifetimes(resourceData, resourceData.SchedulingInfo.ResourceName());

            for (Foundation::Name alias : resourceData.SchedulingInfo.Aliases())
            {
                joinAliasingLifetimes(resourceData, alias);
            }

            if (resourceData.SchedulingInfo.CanBeAliased)
            {
                switch (resourceData.SchedulingInfo.ResourceFormat().ResourceAliasingGroup())
                {
                case HAL::HeapAliasingGroup::RTDSTextures: mRTDSMemoryAliaser.AddSchedulingInfo(&resourceData.SchedulingInfo); break;
                case HAL::HeapAliasingGroup::NonRTDSTextures: mNonRTDSMemoryAliaser.AddSchedulingInfo(&resourceData.SchedulingInfo); break;
                case HAL::HeapAliasingGroup::Buffers: mBufferMemoryAliaser.AddSchedulingInfo(&resourceData.SchedulingInfo); break;
                case HAL::HeapAliasingGroup::Universal: mUniversalMemoryAliaser.AddSchedulingInfo(&resourceData.SchedulingInfo); break;
                }
            }
        }

        // See whether resource reallocation and therefore memory layout invalidation is required
        mMemoryLayoutChanged = !TransferPreviousFrameResources();

        if (mMemoryLayoutChanged)
        {
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

        mAllocationActions.clear();
    }

    void PipelineResourceStorage::QueueTextureAllocationIfNeeded(
        ResourceName resourceName,
        HAL::FormatVariant format,
        HAL::TextureKind kind,
        const Geometry::Dimensions& dimensions,
        const HAL::ClearValue& optimizedClearValue,
        uint16_t mipCount,
        const SchedulingInfoConfigurator& siConfigurator)
    {
        HAL::TextureProperties textureProperties{ format, kind, dimensions, optimizedClearValue, HAL::ResourceState::Common, mipCount };
        HAL::ResourceFormat textureFormat{ mDevice, textureProperties };

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

            HAL::TextureProperties completeProperties{
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
        mSchedulingCreationRequests.emplace_back(SchedulingRequest{ siConfigurator, resourceName });
    }

    void PipelineResourceStorage::QueueResourceUsage(ResourceName resourceName, std::optional<ResourceName> aliasName, const SchedulingInfoConfigurator& siConfigurator)
    {
        mSchedulingUsageRequests.emplace_back(SchedulingRequest{ siConfigurator, resourceName });

        if (aliasName)
        {
            mAliasMap[*aliasName] = resourceName;
        }
    }

    PipelineResourceStoragePass& PipelineResourceStorage::CreatePerPassData(PassName name)
    {
        auto [it, success] = mPerPassData.emplace(name, PipelineResourceStoragePass{});

        HAL::BufferProperties<float> properties{ 1024 };
        it->second.PassDebugBuffer = mResourceProducer->NewBuffer(properties);
        it->second.PassDebugBuffer->SetDebugName(name.ToString() + " Debug Constant Buffer");
        it->second.PassDebugBuffer->RequestWrite();

        // Avoid garbage on first use
        uint8_t* uploadMemory = it->second.PassDebugBuffer->WriteOnlyPtr();
        memset(uploadMemory, 0, sizeof(float) * 1024);

        return it->second;
    }

    PipelineResourceStorageResource& PipelineResourceStorage::CreatePerResourceData(ResourceName name, const HAL::ResourceFormat& resourceFormat)
    {
        PipelineResourceStorageResource& resourceObjects = mCurrentFrameResources->emplace_back(name, resourceFormat);
        mCurrentFrameResourceMap->emplace(name, mCurrentFrameResources->size() - 1);
        return resourceObjects;
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

            resourceData.SchedulingInfo.ApplyExpectedStates();
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
