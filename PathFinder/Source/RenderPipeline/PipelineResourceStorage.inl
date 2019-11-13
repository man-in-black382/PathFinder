#pragma once

namespace PathFinder
{

    template <class RootConstants>
    RootConstants* PipelineResourceStorage::RootConstantDataForCurrentPass() const
    {
        auto bufferIt = mPerPassConstantBuffers.find(mCurrentPassName);
        if (bufferIt == mPerPassConstantBuffers.end()) return nullptr;

        return reinterpret_cast<RootConstants *>(bufferIt->second->At(0));
    }

    template <class BufferDataT>
    void PipelineResourceStorage::AllocateRootConstantBufferIfNeeded()
    {
        auto bufferIt = mPerPassConstantBuffers.find(mCurrentPassName);
        bool alreadyAllocated = bufferIt != mPerPassConstantBuffers.end();

        if (alreadyAllocated) return;

        // Because we store complex objects in unified buffers of primitive type
        // we must alight manually beforehand and pass alignment of 1 to the buffer
        //
        auto bufferSize = Foundation::MemoryUtils::Align(sizeof(BufferDataT), 256);

        mPerPassConstantBuffers.emplace(mCurrentPassName, std::make_unique<HAL::RingBufferResource<uint8_t>>(
            *mDevice, bufferSize, mSimultaneousFramesInFlight, 1, HAL::CPUAccessibleHeapType::Upload));
    }

    template <class BufferDataT>
    PipelineResourceAllocation* PipelineResourceStorage::QueueBufferAllocationIfNeeded(ResourceName resourceName, uint64_t capacity, uint64_t perElementAlignment)
    {
        auto it = mPipelineResourceAllocations.find(resourceName);

        if (it != mPipelineResourceAllocations.end())
        {
            return &it->second;
        }
        
        auto [iter, success] = mPipelineResourceAllocations.emplace(
            resourceName, HAL::BufferResource<BufferDataT>::ConstructResourceFormat(mDevice, capacity, perElementAlignment)
        );

        PipelineResourceAllocation& allocation = iter->second;

        allocation.AllocationAction = [=, &allocation]()
        {
            HAL::Heap* heap = mBufferHeap.get();

            // Store as byte buffer and alight manually
            auto stride = Foundation::MemoryUtils::Align(sizeof(BufferDataT), 256);
            auto bufferSize = stride * capacity;

            auto buffer = std::make_unique<HAL::BufferResource<uint8_t>>(
                *mDevice, *heap, allocation.AliasingInfo.HeapOffset, bufferSize, 1,
                allocation.InitialStates(), allocation.ExpectedStates()
            );

            buffer->SetDebugName(resourceName.ToString());

            BufferPipelineResource newResource;
            CreateDescriptors(newResource, allocation, buffer.get(), stride);

            newResource.Resource = std::move(buffer);
            mPipelineBufferResources[resourceName] = std::move(newResource);
        };

        return &allocation;
    }

}