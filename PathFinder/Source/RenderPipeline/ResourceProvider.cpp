#include "ResourceProvider.hpp"
#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    ResourceProvider::ResourceProvider(ResourceStorage* storage)
        : mResourceStorage{ storage } {}

    uint32_t ResourceProvider::GetTextureDescriptorTableIndex(Foundation::Name resourceName)
    {
        const auto& names = mResourceStorage->ScheduledResourceNamesForCurrentPass();
        
        bool resourceScheduled = names.find(resourceName) != names.end();
        assert_format(resourceScheduled, "Resource ", resourceName.ToSring(), " was not scheduled to be used in this pass");

        PipelineResource& resource = mResourceStorage->GetPipelineResource(resourceName);

        auto perPassData = resource.GetPerPassData(mResourceStorage->mCurrentPassName);

        std::optional<HAL::ResourceFormat::Color> format;

        if (perPassData) format = perPassData->ShaderVisibleFormat;

        if (const HAL::SRDescriptor* srDescriptor = mResourceStorage->mDescriptorStorage.GetSRDescriptor(resourceName, format))
        {
            return srDescriptor->IndexInHeapRange();
        } 

        if (const HAL::UADescriptor* uaDescriptor = mResourceStorage->mDescriptorStorage.GetUADescriptor(resourceName, format))
        {
            return uaDescriptor->IndexInHeapRange();
        }

        assert_format(false, "Cannot find descriptor for resource ", resourceName.ToSring(), ". This is a bug.");

        return 0;
    }

}
