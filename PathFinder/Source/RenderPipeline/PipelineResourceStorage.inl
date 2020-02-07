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

    template <class BufferDataT>
    void PipelineResourceStorage::AllocateRootConstantBufferIfNeeded()
    {
        //PerPassObjects& passObjects = GetPerPassObjects(mCurrentPassName);

        //if (passObjects.PassConstantBuffer) return;

        //// Because we store complex objects in unified buffers of primitive type
        //// we must alight manually beforehand and pass alignment of 1 to the buffer
        ////
        //auto bufferSize = Foundation::MemoryUtils::Align(sizeof(BufferDataT), 256);

        //passObjects.PassConstantBuffer = std::make_unique<HAL::RingBufferResource<uint8_t>>(
        //    *mDevice, bufferSize, mSimultaneousFramesInFlight, 1, HAL::CPUAccessibleHeapType::Upload);
    }

    template <class BufferDataT>
    PipelineResourceSchedulingInfo* PipelineResourceStorage::QueueBufferAllocationIfNeeded(ResourceName resourceName, uint64_t capacity, uint64_t perElementAlignment)
    {
        PerResourceObjects& resourceObjects = GetPerResourceObjects(resourceName);

        if (resourceObjects.SchedulingInfo)
        {
            return resourceObjects.SchedulingInfo.get();
        }
        
        HAL::Buffer::Properties<BufferDataT> properties{ capacity, perElementAlignment };

        resourceObjects.SchedulingInfo = std::make_unique<PipelineResourceSchedulingInfo>(
            HAL::Buffer::ConstructResourceFormat(mDevice, properties));

        resourceObjects.SchedulingInfo->AllocationAction = [=, &resourceObjects]()
        {
            PipelineResourceSchedulingInfo* schedulingInfo = resourceObjects.SchedulingInfo.get();
            HAL::Heap* heap = nullptr;

            switch (schedulingInfo->ResourceFormat().ResourceAliasingGroup())
            { 
            case HAL::HeapAliasingGroup::Buffers: heap = mBufferHeap.get(); break;
            case HAL::HeapAliasingGroup::Universal: heap = mUniversalHeap.get(); break;
            case HAL::HeapAliasingGroup::RTDSTextures:
            case HAL::HeapAliasingGroup::NonRTDSTextures:
                assert_format(false, "Should never be hit");
            }

            HAL::Buffer::Properties<BufferDataT> finalProperties{ capacity, perElementAlignment, schedulingInfo->InitialStates(), schedulingInfo->ExpectedStates() };
            resourceObjects.Buffer->Resource = mResourceProducer->NewBuffer(finalProperties, *heap, schedulingInfo->AliasingInfo.HeapOffset);
            resourceObjects.Buffer->Resource->SetDebugName(resourceName.ToString());

            for (const auto& [passName, passMetadata] : schedulingInfo->AllPassesMetadata())
            {
                BufferPipelineResource::PassMetadata& newResourcePerPassData = resourceObjects.Buffer->AllocateMetadateForPass(passName);
                newResourcePerPassData.IsCBDescriptorRequested = passMetadata.CreateBufferCBDescriptor;
                newResourcePerPassData.IsSRDescriptorRequested = passMetadata.CreateBufferSRDescriptor;
                newResourcePerPassData.IsUADescriptorRequested = passMetadata.CreateBufferUADescriptor;
                newResourcePerPassData.RequiredState = passMetadata.OptimizedState;
                newResourcePerPassData.ShaderVisibleFormat = passMetadata.ShaderVisibleFormat;
            }
        };

        return resourceObjects.SchedulingInfo.get();
    }

}