namespace PathFinder
{

    template <class T>
    void ResourceScheduler::NewBuffer(Foundation::Name resourceName, const NewBufferProperties<T>& bufferProperties)
    {
        EnsureSingleSchedulingRequestForCurrentPass(resourceName);

        assert_format(!mResourceStorage->IsResourceAllocationScheduled(resourceName), "Buffer creation has already been scheduled");

        PipelineResourceSchedulingInfo* schedulingInfo = mResourceStorage->QueueBuffersAllocationIfNeeded<T>(
            resourceName, bufferProperties.Capacity, bufferProperties.PerElementAlignment, bufferProperties.BuffersCount
        );

        for (auto bufferIdx = 0u; bufferIdx < schedulingInfo->ResourceCount(); ++bufferIdx)
        {
            auto& passData = schedulingInfo->AllocateMetadataForPass(mResourceStorage->CurrentPassGraphNode(), bufferIdx);
            passData.RequestedState = HAL::ResourceState::UnorderedAccess;
            passData.CreateBufferUADescriptor = true;
        }

        mResourceStorage->RegisterResourceNameForCurrentPass(resourceName);
    }

}

