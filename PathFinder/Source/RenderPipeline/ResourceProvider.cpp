#include "ResourceProvider.hpp"
#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    ResourceProvider::ResourceProvider(const PipelineResourceStorage* storage, const ResourceDescriptorStorage* descriptorStorage)
        : mResourceStorage{ storage }, mDescriptorStorage{ descriptorStorage } {}

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

    uint32_t ResourceProvider::GetExternalTextureDescriptorTableIndex(const HAL::Texture* texture, HAL::ShaderRegister registerType)
    {
        switch (registerType)
        {
        case HAL::ShaderRegister::ShaderResource:
        {
            auto descriptor = mDescriptorStorage->GetSRDescriptor(texture);
            assert_format(descriptor, "Descriptor for texture does not exist");
            return descriptor->IndexInHeapRange();
        }
            
        case HAL::ShaderRegister::UnorderedAccess:
        {
            auto descriptor = mDescriptorStorage->GetUADescriptor(texture);
            assert_format(descriptor, "Descriptor for texture does not exist");
            return descriptor->IndexInHeapRange();
        }

        default:
            assert_format(false, "Invalid register type");
            return 0;
        }
    }

}
