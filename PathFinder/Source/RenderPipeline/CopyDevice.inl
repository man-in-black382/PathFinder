namespace PathFinder
{

    template <class T>
    void CopyDevice::QueueBufferToTextureCopy(std::shared_ptr<HAL::BufferResource<T>> buffer, const HAL::TextureResource& texture, const ResourceFootprint& footprint)
    {
        assert_format(IsCopyableState(buffer->InitialStates()), "Resource must be in a copyable state");

        for (const SubresourceFootprint& subresourceFootprint : footprint.SubresourceFootprints())
        {
            mCommandList.CopyBufferToTexture(*buffer, texture, subresourceFootprint);
        }

        mResourcesToCopy.push_back(buffer);
    }

    template <class T>
    std::unique_ptr<HAL::BufferResource<T>> CopyDevice::QueueResourceCopyToDefaultHeap(std::shared_ptr<HAL::BufferResource<T>> buffer)
    {
        assert_format(IsCopyableState(buffer->InitialStates()), "Resource must be in a copyable state");

        auto emptyClone = std::make_unique<HAL::BufferResource<T>>(
            *mDevice, buffer->Capacity(), buffer->PerElementAlignment(), HAL::ResourceState::CopyDestination, buffer->ExpectedStates());

        mCommandList.CopyResource(*buffer, *emptyClone);

        // Hold in RAM until actual copy is performed
        mResourcesToCopy.push_back(buffer);

        return std::move(emptyClone);
    }

}

