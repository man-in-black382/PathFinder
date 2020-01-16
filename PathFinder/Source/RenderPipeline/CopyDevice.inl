namespace PathFinder
{

    template <class T>
    void CopyDevice::QueueBufferToTextureCopy(
        const HAL::Buffer<T>& buffer,
        const HAL::Texture& texture, 
        const HAL::ResourceFootprint& footprint)
    {
        for (const HAL::SubresourceFootprint& subresourceFootprint : footprint.SubresourceFootprints())
        {
            mRingCommandList.CurrentCommandList().CopyBufferToTexture(buffer, texture, subresourceFootprint);
        }
    }

    template <class T>
    void CopyDevice::QueueBufferToBufferCopy(
        const HAL::Buffer<T>& source,
        const HAL::Buffer<T>& destination,
        uint64_t sourceOffset, uint64_t objectCount, uint64_t destinationOffset)
    {
        mRingCommandList.CurrentCommandList().CopyBufferRegion(source, destination, sourceOffset, objectCount, destinationOffset);
    }

    template <class T>
    void CopyDevice::QueueBufferToTextureCopy(
        std::shared_ptr<HAL::Buffer<T>> buffer, 
        std::shared_ptr<HAL::Texture> texture, 
        const HAL::ResourceFootprint& footprint)
    {
        QueueBufferToTextureCopy(*buffer, *texture, footprint);

        // Hold in RAM until actual copy is performed
        mResourcesToCopy[mCurrentFrameIndex].push_back(buffer);
        mResourcesToCopy[mCurrentFrameIndex].push_back(texture);
    }

    template <class T>
    void CopyDevice::QueueBufferToBufferCopy(
        std::shared_ptr<HAL::Buffer<T>> source,
        std::shared_ptr<HAL::Buffer<T>> destination,
        uint64_t sourceOffset, uint64_t objectCount, uint64_t destinationOffset)
    {
        QueueBufferToBufferCopy(*source, *destination, sourceOffset, objectCount, destinationOffset);

        // Hold in RAM until actual copy is performed
        mResourcesToCopy[mCurrentFrameIndex].push_back(source);
        mResourcesToCopy[mCurrentFrameIndex].push_back(destination);
    }

    template <class T>
    std::shared_ptr<HAL::Buffer<T>> CopyDevice::QueueResourceCopyToDefaultMemory(std::shared_ptr<HAL::Buffer<T>> buffer)
    {
        auto emptyClone = std::make_shared<HAL::Buffer<T>>(
            *mDevice, buffer->Capacity(), buffer->PerElementAlignment(), HAL::ResourceState::CopyDestination, buffer->ExpectedStates());

        mRingCommandList.CurrentCommandList().CopyResource(*buffer, *emptyClone);

        // Hold in RAM until actual copy is performed
        mResourcesToCopy[mCurrentFrameIndex].push_back(buffer);
        mResourcesToCopy[mCurrentFrameIndex].push_back(emptyClone);

        return emptyClone;
    }

    template <class T>
    std::shared_ptr<HAL::Buffer<T>> CopyDevice::QueueResourceCopyToReadbackMemory(std::shared_ptr<HAL::Buffer<T>> buffer)
    {
        auto emptyClone = std::make_shared<HAL::Buffer<T>>(
            *mDevice, buffer->Capacity(), buffer->PerElementAlignment(), HAL::CPUAccessibleHeapType::Readback);

        mRingCommandList.CurrentCommandList().CopyResource(*buffer, *emptyClone);

        // Hold in RAM until actual copy is performed
        mResourcesToCopy[mCurrentFrameIndex].push_back(buffer);
        mResourcesToCopy[mCurrentFrameIndex].push_back(emptyClone);

        return emptyClone;
    }

}

