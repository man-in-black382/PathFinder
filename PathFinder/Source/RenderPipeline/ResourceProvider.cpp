#include "ResourceProvider.hpp"
#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    ResourceProvider::ResourceProvider(const PipelineResourceStorage* storage)
        : mResourceStorage{ storage } {}

    uint32_t ResourceProvider::GetUATextureIndex(const ResourceKey& textureKey, uint8_t mipLevel)
    {
        const PipelineResourceStorageResource* resourceObjects = mResourceStorage->GetPerResourceData(textureKey.ResourceName());
        const Memory::Texture* resource = resourceObjects->GetTexture(textureKey.IndexInArray());
        assert_format(resource, "Resource ", textureKey.ResourceName().ToString(), " does not exist");

        Foundation::Name passName = mResourceStorage->CurrentPassGraphNode().PassMetadata.Name;
        const PipelineResourceSchedulingInfo::PassInfo* perPassData = resourceObjects->SchedulingInfo.GetInfoForPass(passName, textureKey.IndexInArray(), mipLevel);

        assert_format(perPassData,
            "Resource ",
            textureKey.ResourceName().ToString(),
            " at index ",
            std::to_string(textureKey.IndexInArray()),
            " was not scheduled for usage in ",
            passName.ToString());

        assert_format(perPassData->IsTextureUARequested(),
            "Resource ", 
            textureKey.ResourceName().ToString(),
            " was not scheduled to be accessed as Unordered Access resource in ",
            passName.ToString());

        return resource->GetUADescriptor(mipLevel)->IndexInHeapRange();
    }

    uint32_t ResourceProvider::GetSRTextureIndex(const ResourceKey& textureKey, uint8_t mipLevel)
    {
        const PipelineResourceStorageResource* resourceObjects = mResourceStorage->GetPerResourceData(textureKey.ResourceName());
        const Memory::Texture* resource = resourceObjects->GetTexture(textureKey.IndexInArray());
        assert_format(resource, "Resource ", textureKey.ResourceName().ToString(), " does not exist");

        Foundation::Name passName = mResourceStorage->CurrentPassGraphNode().PassMetadata.Name;

        // Mip level only used for sanity check. It doesn't alter SRV in any way in current implementation.
        const PipelineResourceSchedulingInfo::PassInfo* perPassData = resourceObjects->SchedulingInfo.GetInfoForPass(passName, textureKey.IndexInArray(), mipLevel);

        assert_format(perPassData,
            "Resource ",
            textureKey.ResourceName().ToString(),
            " at index ",
            std::to_string(textureKey.IndexInArray()),
            " was not scheduled for usage in ",
            passName.ToString());

        assert_format(perPassData->IsTextureSRRequested(),
            "Resource ", 
            textureKey.ResourceName().ToString(),
            " was not scheduled to be accessed as Shader Resource in ",
            passName.ToString());

        return resource->GetSRDescriptor()->IndexInHeapRange();
    }

    const HAL::Texture::Properties& ResourceProvider::GetTextureProperties(Foundation::Name resourceName)
    {
        const PipelineResourceStorageResource* resourceObjects = mResourceStorage->GetPerResourceData(resourceName);
        // Since all textures in the array have identical properties, get first 
        const Memory::Texture* resource = resourceObjects->GetTexture();
        assert_format(resource, "Resource ", resourceName.ToString(), " does not exist");

        return resource->Properties();
    }

}
