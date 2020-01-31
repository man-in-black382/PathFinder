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
        
      /*  resourceObjects.SchedulingInfo = std::make_unique<PipelineResourceSchedulingInfo>(
            HAL::Buffer<BufferDataT>::ConstructResourceFormat(mDevice, capacity, perElementAlignment));*/

        resourceObjects.SchedulingInfo->AllocationAction = [=, &resourceObjects]()
        {
            PipelineResourceSchedulingInfo* allocation = resourceObjects.SchedulingInfo.get();
            HAL::Heap* heap = mBufferHeap.get();

            // Store as byte buffer and alight manually
            auto stride = Foundation::MemoryUtils::Align(sizeof(BufferDataT), perElementAlignment);
            auto bufferSize = stride * capacity;

            resourceObjects.Buffer = std::make_unique<BufferPipelineResource>();

            /*resourceObjects.Buffer->Resource = std::make_unique<HAL::Buffer<uint8_t>>(
                *mDevice, *heap, allocation->AliasingInfo.HeapOffset, bufferSize, 1,
                allocation->InitialStates(), allocation->ExpectedStates()
            );*/

            resourceObjects.Buffer->Resource->SetDebugName(resourceName.ToString());

            CreateDescriptors(*resourceObjects.Buffer, *resourceObjects.SchedulingInfo, stride);
        };

        return resourceObjects.SchedulingInfo.get();
    }

}