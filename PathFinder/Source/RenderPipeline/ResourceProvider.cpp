#include "ResourceProvider.hpp"
#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    ResourceProvider::ResourceProvider(const PipelineResourceStorage* storage)
        : mResourceStorage{ storage } {}

    uint32_t ResourceProvider::GetUATextureIndex(const ResourceKey& textureKey, uint8_t mipLevel)
    {
        const auto* resourceObjects = mResourceStorage->GetPerResourceObjects(textureKey.ResourceName());
        const Memory::Texture* resource = resourceObjects->GetTexture(textureKey.IndexInArray());
        assert_format(resource, "Resource ", textureKey.ResourceName().ToString(), " does not exist");

        const auto* perPassData = resourceObjects->SchedulingInfo->GetMetadataForPass(mResourceStorage->CurrentPassGraphNode().PassMetadata.Name);
        assert_format(perPassData && perPassData->CreateTextureUADescriptor, "Resource ", textureKey.ResourceName().ToString(), " was not scheduled to be accessed as Unordered Access resource");

        return resource->GetUADescriptor(mipLevel)->IndexInHeapRange();
    }

    uint32_t ResourceProvider::GetSRTextureIndex(const ResourceKey& textureKey)
    {
        const auto* resourceObjects = mResourceStorage->GetPerResourceObjects(textureKey.ResourceName());
        const Memory::Texture* resource = resourceObjects->GetTexture(textureKey.IndexInArray());
        assert_format(resource, "Resource ", textureKey.ResourceName().ToString(), " does not exist");

        const auto* perPassData = resourceObjects->SchedulingInfo->GetMetadataForPass(mResourceStorage->CurrentPassGraphNode().PassMetadata.Name);
        assert_format(perPassData && perPassData->CreateTextureSRDescriptor, "Resource ", textureKey.ResourceName().ToString(), " was not scheduled to be accessed as Shader Resource");

        return resource->GetSRDescriptor()->IndexInHeapRange();
    }

    const HAL::Texture::Properties& ResourceProvider::GetTextureProperties(Foundation::Name resourceName)
    {
        const auto* resourceObjects = mResourceStorage->GetPerResourceObjects(resourceName);
        // Since all textures in the array have identical properties, get first 
        const Memory::Texture* resource = resourceObjects->GetTexture();
        assert_format(resource, "Resource ", resourceName.ToString(), " does not exist");

        return resource->Properties();
    }

}
