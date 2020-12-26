#pragma once

namespace PathFinder
{

    template <class Constants>
    void PipelineResourceStorage::UpdateFrameRootConstants(const Constants& constants)
    {
        constexpr uint64_t Alignment = 256;

        if (!mPerFrameRootConstantsBuffer || mPerFrameRootConstantsBuffer->Capacity<Constants>(Alignment) < 1)
        {
            auto properties = HAL::BufferProperties::Create<Constants>(1, Alignment, HAL::ResourceState::ConstantBuffer);
            mPerFrameRootConstantsBuffer = mResourceProducer->NewBuffer(properties, Memory::GPUResource::AccessStrategy::DirectUpload);
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
            auto properties = HAL::BufferProperties::Create<Constants>(1, Alignment, HAL::ResourceState::ConstantBuffer);
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
            auto properties = HAL::BufferProperties::Create<uint8_t>(grownBufferSize, 1, HAL::ResourceState::ConstantBuffer);

            passData->PassConstantBuffer = mResourceProducer->NewBuffer(properties, Memory::GPUResource::AccessStrategy::DirectUpload);
            passData->PassConstantBuffer->SetDebugName(passNode.PassMetadata().Name.ToString() + " Constant Buffer");
            passData->PassConstantData.resize(grownBufferSize);
        }

        passData->PassConstantBuffer->RequestWrite();

        // Store data in CPU storage 
        const uint8_t* data = reinterpret_cast<const uint8_t*>(&constants);
        std::copy(data, data + sizeof(Constants), passData->PassConstantData.begin() + passData->PassConstantBufferMemoryOffset);
    }

}