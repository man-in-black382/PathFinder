#include "ResourceProvider.hpp"
#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    ResourceProvider::ResourceProvider(const PipelineResourceStorage* storage)
        : mResourceStorage{ storage } {}

    uint32_t ResourceProvider::GetTextureDescriptorTableIndex(Foundation::Name resourceName) const
    {
        const TexturePipelineResource* resource = mResourceStorage->GetPipelineTextureResource(resourceName);
        assert_format(resource != nullptr, "Resource ", resourceName.ToString(), " does not exist");

        auto perPassData = resource->GetMetadataForPass(mResourceStorage->CurrentPassName());
        assert_format(perPassData, "Resource ", resourceName.ToString(), " was not scheduled to be used in this pass");

        if (perPassData->IsSRDescriptorRequested)
        {
            auto srDescriptor = mResourceStorage->DescriptorStorage()->GetSRDescriptor(resource->Resource.get(), perPassData->ShaderVisibleFormat);
            return srDescriptor->IndexInHeapRange();
        } 

        if (perPassData->IsUADescriptorRequested)
        {
            auto uaDescriptor = mResourceStorage->DescriptorStorage()->GetUADescriptor(resource->Resource.get(), perPassData->ShaderVisibleFormat);
            return uaDescriptor->IndexInHeapRange();
        }

        assert_format(perPassData, "Resource ", resourceName.ToString(), " was not scheduled for reading in this pass");

        return 0;
    }

    uint32_t ResourceProvider::GetExternalTextureDescriptorTableIndex(const HAL::TextureResource* texture, HAL::ShaderRegister registerType)
    {
        assert_format(false, "not implemented");
        return 0;
    }

}
