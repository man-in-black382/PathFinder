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
            for (auto subresourceIdx = 0u; subresourceIdx < schedulingInfo->SubresourceCount(); ++subresourceIdx)
            {
                PipelineResourceSchedulingInfo::PassInfo& passInfo = schedulingInfo->AllocateInfoForPass(
                    mResourceStorage->CurrentPassGraphNode(), bufferIdx, subresourceIdx
                );

                passInfo.RequestedState = HAL::ResourceState::UnorderedAccess;
                passInfo.CreateBufferUADescriptor = true;
            }            
        }

        mResourceStorage->RegisterResourceNameForCurrentPass(resourceName);
    }

}

