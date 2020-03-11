#include "ResourceProvider.hpp"
#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    ResourceProvider::ResourceProvider(const PipelineResourceStorage* storage)
        : mResourceStorage{ storage } {}

    uint32_t ResourceProvider::GetUATextureIndex(Foundation::Name resourceName, uint8_t mipLevel)
    {
        auto* resourceObjects = mResourceStorage->GetPerResourceObjects(resourceName);
        Memory::Texture* resource = resourceObjects->Texture.get();
        assert_format(resource, "Resource ", resourceName.ToString(), " does not exist");

        const auto* perPassData = resourceObjects->SchedulingInfo->GetMetadataForPass(mResourceStorage->CurrentPassGraphNode().PassMetadata.Name);
        assert_format(perPassData && perPassData->CreateTextureUADescriptor, "Resource ", resourceName.ToString(), " was not scheduled to be accessed as Unordered Access resource");

        return resource->GetOrCreateUADescriptor(mipLevel)->IndexInHeapRange();
    }

    uint32_t ResourceProvider::GetSRTextureIndex(Foundation::Name resourceName)
    {
        auto* resourceObjects = mResourceStorage->GetPerResourceObjects(resourceName);
        Memory::Texture* resource = resourceObjects->Texture.get();
        assert_format(resource, "Resource ", resourceName.ToString(), " does not exist");

        const auto* perPassData = resourceObjects->SchedulingInfo->GetMetadataForPass(mResourceStorage->CurrentPassGraphNode().PassMetadata.Name);
        assert_format(perPassData && perPassData->CreateTextureSRDescriptor, "Resource ", resourceName.ToString(), " was not scheduled to be accessed as Shader Resource");

        return resource->GetOrCreateSRDescriptor()->IndexInHeapRange();
    }

    const HAL::Texture::Properties& ResourceProvider::GetTextureProperties(Foundation::Name resourceName)
    {
        auto* resourceObjects = mResourceStorage->GetPerResourceObjects(resourceName);
        Memory::Texture* resource = resourceObjects->Texture.get();
        assert_format(resource, "Resource ", resourceName.ToString(), " does not exist");

        return resource->Properties();
    }

}
