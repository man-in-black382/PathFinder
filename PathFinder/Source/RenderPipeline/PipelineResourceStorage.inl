#pragma once

namespace PathFinder
{

    template <class RootConstants>
    RootConstants* PipelineResourceStorage::RootConstantDataForCurrentPass()
    {
        /*PerPassObjects& passObjects = GetPerPassObjects(mCurrentPassName);

        if (!passObjects.PassConstantBuffer)
        {
            return nullptr;
        }

        return reinterpret_cast<RootConstants *>(passObjects.PassConstantBuffer->At(0));*/

        return nullptr;
    }

    template <class Constants>
    void PipelineResourceStorage::UpdateFrameRootConstants(const Constants& constants)
    {
        constexpr uint64_t Alignment = 256;

        if (!mPerFrameRootConstantsBuffer || mPerFrameRootConstantsBuffer->HALBuffer()->ElementCapacity<Constants>(Alignment) < 1)
        {
            HAL::Buffer::Properties<Constants> properties{ 1, Alignment, HAL::ResourceState::ConstantBuffer };
            mPerFrameRootConstantsBuffer = mResourceProducer->NewBuffer(properties, Memory::GPUResource::UploadStrategy::DirectAccess);
        }

        mPerFrameRootConstantsBuffer->RequestWrite();
        mPerFrameRootConstantsBuffer->Write(&constants, 0, 1, Alignment);
    }

    template <class Constants>
    void PipelineResourceStorage::UpdateGlobalRootConstants(const Constants& constants)
    {
        constexpr uint64_t Alignment = 256;

        if (!mGlobalRootConstantsBuffer || mGlobalRootConstantsBuffer->HALBuffer()->ElementCapacity<Constants>(Alignment) < 1)
        {
            HAL::Buffer::Properties<Constants> properties{ 1, Alignment, HAL::ResourceState::ConstantBuffer };
            mGlobalRootConstantsBuffer = mResourceProducer->NewBuffer(properties);
        }

        mGlobalRootConstantsBuffer->RequestWrite();
        mGlobalRootConstantsBuffer->Write(&constants, 0, 1, Alignment);
    }

    template <class Constants>
    void PipelineResourceStorage::UpdateCurrentPassRootConstants(const Constants& constants)
    {
        PerPassObjects& passObjects = GetPerPassObjects(mCurrentPassName);

        constexpr uint64_t Alignment = 256;

        if (!passObjects.PassConstantBuffer || passObjects.PassConstantBuffer->HALBuffer()->ElementCapacity<Constants>(Alignment) < 1)
        {
            HAL::Buffer::Properties<Constants> properties{ 1, Alignment, HAL::ResourceState::ConstantBuffer };
            passObjects.PassConstantBuffer = mResourceProducer->NewBuffer(properties, Memory::GPUResource::UploadStrategy::DirectAccess);
        }

        mGlobalRootConstantsBuffer->RequestWrite();
        mGlobalRootConstantsBuffer->Write(&constants, 0, 1, Alignment);
    }

    template <class BufferDataT>
    PipelineResourceSchedulingInfo* PipelineResourceStorage::QueueBufferAllocationIfNeeded(ResourceName resourceName, uint64_t capacity, uint64_t perElementAlignment)
    {
        PerResourceObjects& resourceObjects = GetPerResourceObjects(resourceName);

        if (resourceObjects.SchedulingInfo)
        {
            return &(resourceObjects.SchedulingInfo.value());
        }
        
        HAL::Buffer::Properties<BufferDataT> properties{ capacity, perElementAlignment };

        resourceObjects.SchedulingInfo = PipelineResourceSchedulingInfo{ HAL::Buffer::ConstructResourceFormat(mDevice, properties) };

        resourceObjects.SchedulingInfo->AllocationAction = [=, &resourceObjects]()
        {
            PipelineResourceSchedulingInfo& schedulingInfo = *resourceObjects.SchedulingInfo;
            HAL::Heap* heap = nullptr;

            switch (schedulingInfo.ResourceFormat().ResourceAliasingGroup())
            { 
            case HAL::HeapAliasingGroup::Buffers: heap = mBufferHeap.get(); break;
            case HAL::HeapAliasingGroup::Universal: heap = mUniversalHeap.get(); break;
            case HAL::HeapAliasingGroup::RTDSTextures:
            case HAL::HeapAliasingGroup::NonRTDSTextures:
                assert_format(false, "Should never be hit");
            }

            HAL::Buffer::Properties<BufferDataT> finalProperties{ capacity, perElementAlignment, schedulingInfo.InitialStates(), schedulingInfo.ExpectedStates() };
            resourceObjects.PipelineBuffer.Resource = mResourceProducer->NewBuffer(finalProperties, *heap, schedulingInfo.AliasingInfo.HeapOffset);
            resourceObjects.PipelineBuffer.Resource->SetDebugName(resourceName.ToString());

            for (const auto& [passName, passMetadata] : schedulingInfo.AllPassesMetadata())
            {
                BufferPipelineResource::PassMetadata& newResourcePerPassData = resourceObjects.PipelineBuffer.AllocateMetadateForPass(passName);
                newResourcePerPassData.IsCBDescriptorRequested = passMetadata.CreateBufferCBDescriptor;
                newResourcePerPassData.IsSRDescriptorRequested = passMetadata.CreateBufferSRDescriptor;
                newResourcePerPassData.IsUADescriptorRequested = passMetadata.CreateBufferUADescriptor;
                newResourcePerPassData.RequiredState = passMetadata.OptimizedState;
                newResourcePerPassData.ShaderVisibleFormat = passMetadata.ShaderVisibleFormat;
            }
        };

        return &(resourceObjects.SchedulingInfo.value());
    }

}