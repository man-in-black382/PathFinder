#include "ResourceProvider.hpp"
#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    ResourceProvider::ResourceProvider(const PipelineResourceStorage* storage, const RenderPassGraph::Node* passNode)
        : mResourceStorage{ storage }, mPassNode{ passNode } {}

    uint32_t ResourceProvider::GetUATextureIndex(Foundation::Name textureName, uint8_t mipLevel)
    {
        const PipelineResourceStorageResource* resourceObjects = mResourceStorage->GetPerResourceData(textureName);
        assert_format(resourceObjects && resourceObjects->Texture, "Resource ", textureName.ToString(), " does not exist");

        Foundation::Name passName = mPassNode->PassMetadata().Name;
        const PipelineResourceSchedulingInfo::PassInfo* passInfo = resourceObjects->SchedulingInfo.GetInfoForPass(passName);

        assert_format(passInfo && passInfo->SubresourceInfos[mipLevel],
            "Resource ",
            textureName.ToString(),
            " was not scheduled for usage in ",
            passName.ToString());

        assert_format(passInfo->SubresourceInfos[mipLevel]->AccessValidationFlag == PipelineResourceSchedulingInfo::SubresourceInfo::AccessFlag::TextureUA,
            "Resource ",
            textureName.ToString(),
            " was not scheduled to be accessed as Unordered Access resource in ",
            passName.ToString());

        return resourceObjects->Texture->GetUADescriptor(mipLevel)->IndexInHeapRange();
    }

    uint32_t ResourceProvider::GetSRTextureIndex(Foundation::Name textureName, uint8_t mipLevel)
    {
        const PipelineResourceStorageResource* resourceObjects = mResourceStorage->GetPerResourceData(textureName);
        assert_format(resourceObjects && resourceObjects->Texture, "Resource ", textureName.ToString(), " does not exist");

        Foundation::Name passName = mPassNode->PassMetadata().Name;

        // Mip level only used for sanity check. It doesn't alter SRV in any way in current implementation.
        const PipelineResourceSchedulingInfo::PassInfo* passInfo = resourceObjects->SchedulingInfo.GetInfoForPass(passName);

        assert_format(passInfo && passInfo->SubresourceInfos[mipLevel],
            "Resource ",
            textureName.ToString(),
            " was not scheduled for usage in ",
            passName.ToString());

        assert_format(passInfo->SubresourceInfos[mipLevel]->AccessValidationFlag == PipelineResourceSchedulingInfo::SubresourceInfo::AccessFlag::TextureSR,
            "Resource ", 
            textureName.ToString(),
            " was not scheduled to be accessed as Shader Resource in ",
            passName.ToString());

        return resourceObjects->Texture->GetSRDescriptor()->IndexInHeapRange();
    }

    const HAL::Texture::Properties& ResourceProvider::GetTextureProperties(Foundation::Name resourceName)
    {
        const PipelineResourceStorageResource* resourceObjects = mResourceStorage->GetPerResourceData(resourceName);
        assert_format(resourceObjects && resourceObjects->Texture, "Resource ", resourceName.ToString(), " does not exist");
        return resourceObjects->Texture->Properties();
    }

}
