namespace Memory
{

    template <class Element>
    Buffer::Buffer(
        const HAL::Buffer::Properties<Element>& properties, 
        GPUResource::UploadStrategy uploadStrategy, 
        ResourceStateTracker* stateTracker,
        SegregatedPoolsResourceAllocator* resourceAllocator, 
        HAL::CopyCommandListBase* commandList)
        :
        GPUResource(uploadStrategy, stateTracker, resourceAllocator, commandList)
    {
        mBufferPtr = uploadStrategy == GPUResource::UploadStrategy::Automatic ? // Skip allocating default memory buffer when direct access
            resourceAllocator->AllocateBuffer(properties) : nullptr; // through persistent mapping is requested
    }

}
