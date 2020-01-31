namespace Memory
{

    template <class Element>
    Buffer::Buffer(
        const HAL::Buffer::Properties<Element>& properties, 
        GPUResource::UploadStrategy uploadStrategy, 
        SegregatedPoolsResourceAllocator* resourceAllocator, 
        HAL::CopyCommandListBase* commandList)
        :
        GPUResource(
            uploadStrategy, 
            uploadStrategy == GPUResource::UploadStrategy::Automatic ? // Skip allocating default memory buffer when direct access
                resourceAllocator->AllocateTexture(properties) : nullptr, // through persistent mapping is requested
            resourceAllocator, 
            commandList) 
    {

    }

}
