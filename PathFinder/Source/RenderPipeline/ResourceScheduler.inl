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

        /* assert_format(!mResourceStorage->IsResourceAllocationScheduled(resourceName), "Texture creation has already been scheduled");

         HAL::ResourceFormat::ColorClearValue clearValue{ 0.0, 0.0, 0.0, 1.0 };
         NewTextureProperties props = FillMissingFields(properties);

         HAL::ResourceFormat::FormatVariant format = *props.ShaderVisibleFormat;

         if (props.TypelessFormat)
         {
             format = *props.TypelessFormat;
         }

         PipelineResourceAllocation* allocator = mResourceStorage->QueueTextureAllocationIfNeeded(
             resourceName, format, *props.Kind, *props.Dimensions, clearValue
         );

         auto& passData = allocator->AllocateMetadataForPass(mResourceStorage->CurrentPassName());
         passData.RequestedState = HAL::ResourceState::UnorderedAccess;
         passData.UAInserter = &ResourceDescriptorStorage::EmplaceUADescriptorIfNeeded;

         if (props.TypelessFormat)
         {
             passData.ShaderVisibleFormat = props.ShaderVisibleFormat;
         }

         mResourceStorage->RegisterResourceNameForCurrentPass(resourceName);*/
    }

}

