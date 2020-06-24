namespace PathFinder
{

    template <class T>
    void ResourceScheduler::NewBuffer(Foundation::Name resourceName, const NewBufferProperties<T>& bufferProperties)
    {
        mCurrentlySchedulingPassNode->AddWriteDependency(resourceName, 1);



        //Foundation::Name passName = mResourceStorage->CurrentPassGraphNode()->PassMetadata().Name;

        //// Wait until all allocations are requested by render passed to detach scheduling algorithm from scheduling order
        //auto delayedAllocationRequest = [=, passGraphNode = mResourceStorage->CurrentPassGraphNode()]
        //{
        //    PipelineResourceStorageResource& resourceData = mResourceStorage->QueueBuffersAllocationIfNeeded<T>(
        //        resourceName, bufferProperties.Capacity, bufferProperties.PerElementAlignment, bufferProperties.BuffersCount
        //    );

        //    for (auto bufferIdx = 0u; bufferIdx < resourceData.SchedulingInfo.ResourceCount(); ++bufferIdx)
        //    {
        //        for (auto subresourceIdx = 0u; subresourceIdx < resourceData.SchedulingInfo.SubresourceCount(); ++subresourceIdx)
        //        {
        //            PipelineResourceSchedulingInfo::PassInfo& passInfo = resourceData.SchedulingInfo.AllocateInfoForPass(
        //                passGraphNode, bufferIdx, subresourceIdx
        //            );

        //            passInfo.RequestedState = HAL::ResourceState::UnorderedAccess;
        //            passInfo.SetBufferUARequested();
        //        }
        //    }

        //    // Register resource usage for the render pass
        //    PipelineResourceStoragePass* passData = mResourceStorage->GetPerPassData(passGraphNode.PassMetadata.Name);
        //    passData->ScheduledResourceNames.insert(resourceName);
        //};

        //mResourceStorage->AddResourceCreationAction(delayedAllocationRequest, resourceName, passName);
    }

    template <class Lambda>
    void ResourceScheduler::FillCurrentPassInfo(const PipelineResourceStorageResource* resourceData, const MipList& mipList, const Lambda& lambda)
    {
        if (mipList.empty())
        {
            for (auto subresourceIdx = 0u; subresourceIdx < resourceData->SchedulingInfo.SubresourceCount(); ++subresourceIdx)
            {
                lambda(subresourceIdx);
            }
        }
        else
        {
            for (uint8_t mip : mipList)
            {
                lambda(mip);
            }
        }
    }

}

