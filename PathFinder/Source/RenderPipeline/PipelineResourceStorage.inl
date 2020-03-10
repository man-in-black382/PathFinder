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

        if (!mCurrentPassObjects->PassConstantBuffer || mCurrentPassObjects->PassConstantBuffer->ElementCapacity<Constants>(Alignment) < 1)
        {
            HAL::Buffer::Properties<Constants> properties{ 1, Alignment, HAL::ResourceState::ConstantBuffer };
            mCurrentPassObjects->PassConstantBuffer = mResourceProducer->NewBuffer(properties, Memory::GPUResource::UploadStrategy::DirectAccess);
            mCurrentPassObjects->PassConstantBuffer->SetDebugName(mCurrentRenderPassGraphNode.PassMetadata.Name.ToString() + " Constant Buffer");
        }

        mCurrentPassObjects->PassConstantBuffer->RequestWrite();
        mCurrentPassObjects->PassConstantBuffer->Write(&constants, 0, 1, Alignment);
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

        resourceObjects.SchedulingInfo = PipelineResourceSchedulingInfo{ HAL::Buffer::ConstructResourceFormat(mDevice, properties), resourceName };

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
            resourceObjects.Buffer = mResourceProducer->NewBuffer(finalProperties, *heap, schedulingInfo.AliasingInfo.HeapOffset);
            resourceObjects.Buffer->SetDebugName(resourceName.ToString());
        };

        return &(resourceObjects.SchedulingInfo.value());
    }

}