namespace Memory
{

    template <class Element>
    GPUResourceProducer::BufferPtr GPUResourceProducer::NewBuffer(const HAL::Buffer::Properties<Element>& properties, GPUResource::UploadStrategy uploadStrategy)
    {
        Buffer* buffer = new Buffer{ properties, uploadStrategy, mStateTracker, mResourceAllocator, mDescriptorAllocator, mCommandList };
        auto [iter, success] = mAllocatedResources.insert(buffer);

        auto deallocationCallback = [this, iter](Buffer* buffer)
        {
            mAllocatedResources.erase(iter);
            delete buffer;
        };

        return BufferPtr{ buffer, deallocationCallback };
    }

    template <class Element>
    GPUResourceProducer::BufferPtr GPUResourceProducer::NewBuffer(const HAL::Buffer::Properties<Element>& properties, const HAL::Heap& explicitHeap, uint64_t heapOffset)
    {
        Buffer* buffer = new Buffer{ 
            properties, mStateTracker, mResourceAllocator, 
            mDescriptorAllocator, mCommandList, *mDevice, explicitHeap, heapOffset 
        };

        auto [iter, success] = mAllocatedResources.insert(buffer);

        auto deallocationCallback = [this, iter](Buffer* buffer)
        {
            mAllocatedResources.erase(iter);
            delete buffer;
        };

        return BufferPtr{ buffer, deallocationCallback };
    }

}
