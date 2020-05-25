#pragma once

namespace PathFinder
{

    template <class Constants>
    void PipelineResourceStorage::UpdateFrameRootConstants(const Constants& constants)
    {
        constexpr uint64_t Alignment = 256;

        if (!mPerFrameRootConstantsBuffer || mPerFrameRootConstantsBuffer->ElementCapacity<Constants>(Alignment) < 1)
        {
            HAL::Buffer::Properties<Constants> properties{ 1, Alignment, HAL::ResourceState::ConstantBuffer };
            mPerFrameRootConstantsBuffer = mResourceProducer->NewBuffer(properties, Memory::GPUResource::UploadStrategy::DirectAccess);
            mPerFrameRootConstantsBuffer->SetDebugName("Frame Constant Buffer");
        }

        mPerFrameRootConstantsBuffer->RequestWrite();
        mPerFrameRootConstantsBuffer->Write(&constants, 0, 1, Alignment);
    }

    template <class Constants>
    void PipelineResourceStorage::UpdateGlobalRootConstants(const Constants& constants)
    {
        constexpr uint64_t Alignment = 256;

        if (!mGlobalRootConstantsBuffer || mGlobalRootConstantsBuffer->ElementCapacity<Constants>(Alignment) < 1)
        {
            HAL::Buffer::Properties<Constants> properties{ 1, Alignment, HAL::ResourceState::ConstantBuffer };
            mGlobalRootConstantsBuffer = mResourceProducer->NewBuffer(properties);
            mGlobalRootConstantsBuffer->SetDebugName("Global Constant Buffer");
        }

        mGlobalRootConstantsBuffer->RequestWrite();
        mGlobalRootConstantsBuffer->Write(&constants, 0, 1, Alignment);
    }

    template <class Constants>
    void PipelineResourceStorage::UpdateCurrentPassRootConstants(const Constants& constants)
    {
        constexpr uint64_t Alignment = 256;

        if (!mCurrentPassData->PassConstantBuffer || mCurrentPassData->PassConstantBuffer->ElementCapacity<Constants>(Alignment) < 1)
        {
            HAL::Buffer::Properties<Constants> properties{ 1024, Alignment, HAL::ResourceState::ConstantBuffer };
            mCurrentPassData->PassConstantBuffer = mResourceProducer->NewBuffer(properties, Memory::GPUResource::UploadStrategy::DirectAccess);
            mCurrentPassData->PassConstantBuffer->SetDebugName(mCurrentRenderPassGraphNode.PassMetadata.Name.ToString() + " Constant Buffer");
        }

        // Advance offset once if allowed and transition to non-allowed state
        if (mCurrentPassData->IsAllowedToAdvanceConstantBufferOffset)
        {
            mCurrentPassData->PassConstantBufferMemoryOffset += mCurrentPassData->LastSetConstantBufferDataSize;
            mCurrentPassData->IsAllowedToAdvanceConstantBufferOffset = false;
        }

        mCurrentPassData->LastSetConstantBufferDataSize = Foundation::MemoryUtils::Align(sizeof(Constants), Alignment);

        // Interpret as raw bytes since one render pass can request to upload constants of different types
        mCurrentPassData->PassConstantBuffer->RequestWrite();
        mCurrentPassData->PassConstantBuffer->Write(
            reinterpret_cast<const uint8_t*>(&constants), mCurrentPassData->PassConstantBufferMemoryOffset, sizeof(Constants)
        );
    }

    template <class BufferDataT>
    PipelineResourceStorageResource& PipelineResourceStorage::QueueBuffersAllocationIfNeeded(ResourceName resourceName, uint64_t capacity, uint64_t perElementAlignment, uint64_t buffersCount)
    {
        HAL::Buffer::Properties<BufferDataT> properties{ capacity, perElementAlignment };
        HAL::ResourceFormat bufferFormat = HAL::Buffer::ConstructResourceFormat(mDevice, properties);

        PipelineResourceStorageResource* resourceObjects = GetPerResourceData(resourceName);

        if (resourceObjects)
        {
            return *resourceObjects;
        }

        resourceObjects = &CreatePerResourceData(resourceName, bufferFormat, buffersCount);

        resourceObjects->SchedulingInfo.AllocationAction = [=]()
        {
            HAL::Heap* heap = nullptr;

            switch (resourceObjects->SchedulingInfo.ResourceFormat().ResourceAliasingGroup())
            {
            case HAL::HeapAliasingGroup::Buffers: heap = mBufferHeap.get(); break;
            case HAL::HeapAliasingGroup::Universal: heap = mUniversalHeap.get(); break;
            case HAL::HeapAliasingGroup::RTDSTextures:
            case HAL::HeapAliasingGroup::NonRTDSTextures:
                assert_format(false, "Should never be hit");
            }

            HAL::Buffer::Properties<BufferDataT> finalProperties{ 
                capacity, perElementAlignment, resourceObjects->SchedulingInfo.InitialStates(), resourceObjects->SchedulingInfo.ExpectedStates() 
            };

            for (auto bufferIdx = 0u; bufferIdx < buffersCount; ++bufferIdx)
            {
                if (resourceObjects->SchedulingInfo.MemoryAliasingInfo.IsAliased)
                {
                    resourceObjects->Buffers.emplace_back(mResourceProducer->NewBuffer(finalProperties, *heap, resourceObjects->SchedulingInfo.MemoryAliasingInfo.HeapOffset));
                }
                else
                {
                    resourceObjects->Buffers.emplace_back(mResourceProducer->NewBuffer(finalProperties));
                }

                std::string debugName = resourceName.ToString() + (buffersCount > 1 ? ("[" + std::to_string(bufferIdx) + "]") : "");
                resourceObjects->Buffers.back()->SetDebugName(debugName);
            }
        };
        
        return *resourceObjects;
    }

}