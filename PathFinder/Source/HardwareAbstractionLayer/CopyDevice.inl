namespace HAL
{

    template <class T>
    std::unique_ptr<BufferResource<T>> CopyDevice::QueueResourceCopyToDefaultHeap(std::shared_ptr<BufferResource<T>> buffer)
    {
        auto emptyClone = std::make_unique<BufferResource<T>>(
            *mDevice, buffer->Capacity(), buffer->PaddedElementSize(), buffer->InitialStates(), buffer->ExpectedStates());

        mCommandList.CopyResource(*buffer, *emptyClone);

        // Hold in RAM until actual copy is performed
        mResourcesToCopy.push_back(buffer);

        return std::move(emptyClone);
    }

}

