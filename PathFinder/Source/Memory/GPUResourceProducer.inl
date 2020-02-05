namespace Memory
{

    template <class Element>
    GPUResourceProducer::BufferPtr GPUResourceProducer::NewBuffer(const HAL::Buffer::Properties<Element>& properties, GPUResource::UploadStrategy uploadStrategy)
    {
        Buffer* buffer = new Buffer{ properties, uploadStrategy, mStateTracker, mAllocator, mCommandList };
        auto [iter, success] = mAllocatedResources.insert(buffer);

        auto deallocationCallback = [this, iter](Buffer* buffer)
        {
            mAllocatedResources.erase(iter);
            delete buffer;
        };

        return BufferPtr{ buffer, deallocationCallback };
    }

}
