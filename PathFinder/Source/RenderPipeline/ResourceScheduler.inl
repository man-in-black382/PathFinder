namespace PathFinder
{

    template <class BufferDataT>
    void ResourceScheduler::WillUseRootConstantBuffer()
    {
        mResourceStorage->AllocateRootConstantBufferIfNeeded<BufferDataT>();
    }

    template <class T>
    void ResourceScheduler::NewBuffer(Foundation::Name resourceName, const NewBufferProperties<T>& bufferProperties)
    {
        EnsureSingleSchedulingRequestForCurrentPass(resourceName);

        assert_format(!mResourceStorage->IsResourceAllocationScheduled(resourceName), "Buffer creation has already been scheduled");

        PipelineResourceSchedulingInfo* schedulingInfo = mResourceStorage->QueueBufferAllocationIfNeeded<T>(
            resourceName, bufferProperties.Capacity, bufferProperties.PerElementAlignment
        );

        auto& passData = schedulingInfo->AllocateMetadataForPass(mResourceStorage->CurrentPassName());
        passData.RequestedState = HAL::ResourceState::UnorderedAccess;
        passData.CreateBufferUADescriptor = true;

        mResourceStorage->RegisterResourceNameForCurrentPass(resourceName);
    }

}

