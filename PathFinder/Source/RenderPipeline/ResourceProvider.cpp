#include "ResourceProvider.hpp"
#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    ResourceProvider::ResourceProvider(const PipelineResourceStorage* storage)
        : mResourceStorage{ storage } {}

    uint32_t ResourceProvider::GetTextureDescriptorTableIndex(Foundation::Name resourceName)
    {
        auto* resourceObjects = mResourceStorage->GetPerResourceObjects(resourceName);
        Memory::Texture* resource = resourceObjects->Texture.get();
        assert_format(resource, "Resource ", resourceName.ToString(), " does not exist");

        const auto* perPassData = resourceObjects->SchedulingInfo->GetMetadataForPass(mResourceStorage->CurrentPassName());
        assert_format(perPassData, "Resource ", resourceName.ToString(), " was not scheduled to be used in this pass");

        if (perPassData->CreateTextureSRDescriptor)
        {
            return resource->GetOrCreateSRDescriptor()->IndexInHeapRange();
        } 

        if (perPassData->CreateTextureUADescriptor)
        {
            return resource->GetOrCreateUADescriptor()->IndexInHeapRange();
        }

        assert_format(perPassData, "Resource ", resourceName.ToString(), " was not scheduled for reading in this pass");

        return 0;
    }

}
