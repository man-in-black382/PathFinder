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
        mPreviousFrameDiffEntries->clear();
        mAllocationActions.clear();
        mSchedulingInfoCreationConfiguators.clear();
        mSchedulingInfoUsageConfiguators.clear();

        std::swap(mPreviousFrameDiffEntries, mCurrentFrameDiffEntries);
        std::swap(mPreviousFrameResources, mCurrentFrameResources);
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

    void PipelineResourceStorage::QueueTexturesAllocationIfNeeded(
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

        PipelineResourceStorageResource* resourceObjects = GetPerResourceData(resourceName);
        assert_format(!resourceObjects, "Texture ", resourceName.ToString(), " allocation is already requested");
        resourceObjects = &CreatePerResourceData(resourceName, textureFormat);

        auto allocationAction = [=]()
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
        mSchedulingInfoCreationConfiguators.emplace_back(siConfigurator, resourceName);
    }

    void PipelineResourceStorage::QueueResourceUsage(ResourceName resourceName, const SchedulingInfoConfigurator& siConfigurator)
    {
        mSchedulingInfoUsageConfiguators.emplace_back(siConfigurator, resourceName);
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

    PipelineResourceStorageResource& PipelineResourceStorage::CreatePerResourceData(ResourceName name, const HAL::ResourceFormat& resourceFormat)
    {
        auto [it, success] = mCurrentFrameResources->emplace(name, PipelineResourceStorageResource{ name, resourceFormat });
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
            // Accumulate expected states for resource from previous frame to avoid reallocations 
            // when resource's states ping-pong between frames or change frequently for other reasons.
            auto prevResourceDataIt = mPreviousFrameResources->find(resourceName);
            if (prevResourceDataIt != mPreviousFrameResources->end())
            {
                resourceData.SchedulingInfo.AddExpectedStates(prevResourceDataIt->second.SchedulingInfo.ExpectedStates());
            }

            PipelineResourceStorageResource::DiffEntry diffEntry = resourceData.GetDiffEntry();
            const RenderPassGraph::ResourceUsageTimeline& usageTimeline = mPassExecutionGraph->GetResourceUsageTimeline(resourceName);

            diffEntry.LifetimeStart = usageTimeline.first;
            diffEntry.LifetimeEnd = usageTimeline.second;
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
                PipelineResourceStorageResource& resourceData = mCurrentFrameResources->at(diffEntry.ResourceName);
                PipelineResourceStorageResource& prevResourceData = mPreviousFrameResources->at(diffEntry.ResourceName);

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

    void PipelineResourceStorage::FinalizeSchedulingInfo()
    {
        mRTDSMemoryAliaser = { mPassExecutionGraph };
        mNonRTDSMemoryAliaser = { mPassExecutionGraph };
        mBufferMemoryAliaser = { mPassExecutionGraph };
        mUniversalMemoryAliaser = { mPassExecutionGraph };

        for (auto& [configurator, resourceName] : mSchedulingInfoCreationConfiguators)
        {
            PipelineResourceStorageResource* resourceData = GetPerResourceData(resourceName);
            configurator(resourceData->SchedulingInfo);
        }

        for (auto& [configurator, resourceName] : mSchedulingInfoUsageConfiguators)
        {
            PipelineResourceStorageResource* resourceData = GetPerResourceData(resourceName);
            assert_format(resourceData, "Trying to use a resource that wasn't created: ", resourceName.ToString());
            configurator(resourceData->SchedulingInfo);
        }

        for (auto& [resourceName, resourceData] : *mCurrentFrameResources)
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

    const Memory::Buffer* PipelineResourceStorage::GlobalRootConstantsBuffer() const
    {
        return mGlobalRootConstantsBuffer.get();
    }

    const Memory::Buffer* PipelineResourceStorage::PerFrameRootConstantsBuffer() const
    {
        return mPerFrameRootConstantsBuffer.get();
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
