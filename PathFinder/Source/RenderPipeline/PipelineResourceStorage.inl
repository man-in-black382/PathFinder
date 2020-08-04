#pragma once

namespace PathFinder
{

    template <class Constants>
    void PipelineResourceStorage::UpdateFrameRootConstants(const Constants& constants)
    {
        constexpr uint64_t Alignment = 256;

        if (!mPerFrameRootConstantsBuffer || mPerFrameRootConstantsBuffer->Capacity<Constants>(Alignment) < 1)
        {
            HAL::BufferProperties<Constants> properties{ 1, Alignment, HAL::ResourceState::ConstantBuffer };
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

        if (!mGlobalRootConstantsBuffer || mGlobalRootConstantsBuffer->Capacity<Constants>(Alignment) < 1)
        {
            HAL::BufferProperties<Constants> properties{ 1, Alignment, HAL::ResourceState::ConstantBuffer };
            mGlobalRootConstantsBuffer = mResourceProducer->NewBuffer(properties);
            mGlobalRootConstantsBuffer->SetDebugName("Global Constant Buffer");
        }

        mGlobalRootConstantsBuffer->RequestWrite();
        mGlobalRootConstantsBuffer->Write(&constants, 0, 1, Alignment);
    }

    template <class Constants>
    void PipelineResourceStorage::UpdatePassRootConstants(const Constants& constants, const RenderPassGraph::Node& passNode)
    {
        constexpr uint64_t Alignment = 256;
        constexpr uint64_t GrowAlignment = 4096;

        PipelineResourceStoragePass* passData = GetPerPassData(passNode.PassMetadata().Name);

        uint64_t alignedBytesToWrite = Foundation::MemoryUtils::Align(sizeof(Constants), Alignment);
        uint64_t alreadyWrittenBytes = passData->PassConstantBufferMemoryOffset;
        uint64_t newBufferSize = alignedBytesToWrite + alreadyWrittenBytes;

        // Advance offset once if allowed and transition to non-allowed state
        if (passData->IsAllowedToAdvanceConstantBufferOffset)
        {
            passData->PassConstantBufferMemoryOffset += passData->LastSetConstantBufferDataSize;
            passData->IsAllowedToAdvanceConstantBufferOffset = false;
        }

        passData->LastSetConstantBufferDataSize = alignedBytesToWrite;

        // Allocate on demand
        if (!passData->PassConstantBuffer || passData->PassConstantBuffer->Capacity() < newBufferSize)
        {
            uint64_t grownBufferSize = Foundation::MemoryUtils::Align(newBufferSize, GrowAlignment);
            HAL::BufferProperties properties{ grownBufferSize, 1, HAL::ResourceState::ConstantBuffer };
            passData->PassConstantBuffer = mResourceProducer->NewBuffer(properties, Memory::GPUResource::UploadStrategy::DirectAccess);
            passData->PassConstantBuffer->SetDebugName(passNode.PassMetadata().Name.ToString() + " Constant Buffer");
            passData->PassConstantData.resize(grownBufferSize);
        }

        passData->PassConstantBuffer->RequestWrite();

        // Store data in CPU storage 
        const uint8_t* data = reinterpret_cast<const uint8_t*>(&constants);
        std::copy(data, data + sizeof(Constants), passData->PassConstantData.begin() + passData->PassConstantBufferMemoryOffset);
    }

    template <class BufferDataT>
    void PipelineResourceStorage::QueueBufferAllocationIfNeeded(ResourceName resourceName, uint64_t capacity, uint64_t perElementAlignment, const SchedulingInfoConfigurator& siConfigurator)
    {
        HAL::BufferProperties<BufferDataT> properties{ capacity, perElementAlignment };
        HAL::ResourceFormat bufferFormat = HAL::Buffer::ConstructResourceFormat(mDevice, properties);

        PipelineResourceStorageResource* resourceObjects = GetPerResourceData(resourceName);
        assert_format(!resourceObjects, "Buffer ", resourceName.ToString(), " allocation is already requested");
        resourceObjects = &CreatePerResourceData(resourceName, bufferFormat);

        auto allocationAction = [=]()
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

            HAL::BufferProperties<BufferDataT> finalProperties{
                capacity, perElementAlignment, HAL::ResourceState::Common, resourceObjects->SchedulingInfo.ExpectedStates()
            };

            if (resourceObjects->SchedulingInfo.CanBeAliased)
            {
                resourceObjects->Buffer = mResourceProducer->NewBuffer(finalProperties, *heap, resourceObjects->SchedulingInfo.HeapOffset);
            }
            else
            {
                resourceObjects->Buffer = mResourceProducer->NewBuffer(finalProperties);
            }

            resourceObjects->Buffer->SetDebugName(resourceName.ToString());
        };

        mAllocationActions.push_back(allocationAction);
        mSchedulingCreationRequests.emplace_back(siConfigurator, resourceName, std::nullopt);
    }

}