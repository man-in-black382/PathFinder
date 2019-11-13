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

            auto buffer = std::make_unique<HAL::BufferResource<BufferDataT>>(
                *mDevice, *heap, allocation.AliasingInfo.HeapOffset, capacity, 
                perElementAlignment, allocation.InitialStates(), allocation.ExpectedStates()
            );

            buffer->SetDebugName(resourceName.ToString());

            PipelineResource newResource;
            CreateDescriptors(resourceName, newResource, allocation, buffer.get());

            newResource.Resource = std::move(buffer);
            mPipelineResources[resourceName] = std::move(newResource);
        };

        return &allocation;
    }

    template <class BufferDataT>
    void PathFinder::PipelineResourceStorage::CreateDescriptors(ResourceName resourceName, PipelineResource& resource, const PipelineResourceAllocation& allocator, const HAL::BufferResource<BufferDataT>* buffer)
    {
        for (const auto& [passName, passMetadata] : allocator.AllPassesMetadata())
        {
            PipelineResource::PassMetadata& newResourcePerPassData = resource.AllocateMetadateForPass(passName);

            if (passMetadata.CreateBufferCBDescriptor)
            {
                newResourcePerPassData.IsCBDescriptorRequested = true;
                mDescriptorStorage->EmplaceCBDescriptorIfNeeded(buffer);
            }

            if (passMetadata.CreateBufferSRDescriptor)
            {
                newResourcePerPassData.IsSRDescriptorRequested = true;
                mDescriptorStorage->EmplaceSRDescriptorIfNeeded(buffer);
            }

            if (passMetadata.CreateBufferUADescriptor)
            {
                newResourcePerPassData.IsUADescriptorRequested = true;
                mDescriptorStorage->EmplaceUADescriptorIfNeeded(buffer);
            }
        }
    }

}