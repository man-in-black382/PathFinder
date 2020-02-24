namespace PathFinder
{

    template <class T>
    void ResourceScheduler::NewBuffer(Foundation::Name resourceName, const NewBufferProperties<T>& bufferProperties)
    {
        EnsureSingleSchedulingRequestForCurrentPass(resourceName);

        assert_format(!mResourceStorage->IsResourceAllocationScheduled(resourceName), "Buffer creation has already been scheduled");

        PipelineResourceSchedulingInfo* schedulingInfo = mResourceStorage->QueueBufferAllocationIfNeeded<T>(
            resourceName, bufferProperties.Capacity, bufferProperties.PerElementAlignment
        );

        auto& passData = schedulingInfo->AllocateMetadataForPass(mResourceStorage->CurrentPassGraphNode());
        passData.RequestedState = HAL::ResourceState::UnorderedAccess;
        passData.CreateBufferUADescriptor = true;

        mResourceStorage->RegisterResourceNameForCurrentPass(resourceName);
    }

}

