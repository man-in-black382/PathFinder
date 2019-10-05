#include "ResourceProvider.hpp"
#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    ResourceProvider::ResourceProvider(PipelineResourceStorage* storage)
        : mResourceStorage{ storage } {}

    uint32_t ResourceProvider::GetTextureDescriptorTableIndex(Foundation::Name resourceName)
    {
        const PipelineResource* resource = mResourceStorage->GetPipelineResource(resourceName);
        assert_format(resource != nullptr, "Resource ", resourceName.ToSring(), " does not exist");

        auto perPassData = resource->GetPerPassData(mResourceStorage->mCurrentPassName);
        assert_format(perPassData, "Resource ", resourceName.ToSring(), " was not scheduled to be used in this pass");

        if (perPassData->IsSRDescriptorRequested)
        {
            const HAL::SRDescriptor* srDescriptor = mResourceStorage->mDescriptorStorage.GetSRDescriptor(resource->Resource(), perPassData->ShaderVisibleFormat);
            return srDescriptor->IndexInHeapRange();
        } 

        if (perPassData->IsUADescriptorRequested)
        {
            const HAL::UADescriptor* uaDescriptor = mResourceStorage->mDescriptorStorage.GetUADescriptor(resource->Resource(), perPassData->ShaderVisibleFormat);
            return uaDescriptor->IndexInHeapRange();
        }

        assert_format(perPassData, "Resource ", resourceName.ToSring(), " was not scheduled for reading in this pass");

        return 0;
    }

}
