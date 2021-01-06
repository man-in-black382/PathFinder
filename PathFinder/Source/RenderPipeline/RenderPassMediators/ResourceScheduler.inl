namespace PathFinder
{

    template <class T>
    void ResourceScheduler::NewBuffer(Foundation::Name resourceName, const NewBufferProperties<T>& bufferProperties)
    {
        bool canBeReadAcrossFrames = EnumMaskEquals(bufferProperties.Flags, Flags::CrossFrameRead);

        mResourceStorage->QueueResourceAllocationIfNeeded(
            resourceName,
            HAL::BufferProperties::Create<T>(bufferProperties.Capacity, bufferProperties.PerElementAlignment),
            bufferProperties.BufferToCopyPropertiesFrom,

            [this, resourceName, canBeReadAcrossFrames, node = mCurrentlySchedulingPassNode](PipelineResourceSchedulingInfo& schedulingInfo)
            {
                RegisterGraphDependency(*node, MipSet::FirstMip(), resourceName, {}, 1, true);

                schedulingInfo.SetSubresourceInfo(
                    node->PassMetadata().Name,
                    0, 
                    HAL::ResourceState::UnorderedAccess,
                    PipelineResourceSchedulingInfo::SubresourceInfo::AccessFlag::BufferUA,
                    std::nullopt);

                schedulingInfo.CanBeAliased = !canBeReadAcrossFrames && mResourceStorage->IsMemoryAliasingEnabled();
            }
        );
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

