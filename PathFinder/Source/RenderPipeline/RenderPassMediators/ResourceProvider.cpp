#include "ResourceProvider.hpp"
#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    ResourceProvider::ResourceProvider(const PipelineResourceStorage* storage, const RenderPassGraph* passGraph, uint64_t graphNodeIndex)
        : mResourceStorage{ storage }, mPassGraph{ passGraph }, mGraphNodeIndex{ graphNodeIndex } {}

    uint32_t ResourceProvider::GetUATextureIndex(Foundation::Name textureName, uint8_t mipLevel) const
    {
        const PipelineResourceStorageResource* resourceObjects = mResourceStorage->GetPerResourceData(textureName);
        assert_format(resourceObjects && resourceObjects->Texture, "Resource ", textureName.ToString(), " does not exist");

        Foundation::Name passName = mPassGraph->Nodes()[mGraphNodeIndex].PassMetadata().Name;
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

    uint32_t ResourceProvider::GetSRTextureIndex(Foundation::Name textureName, uint8_t mipLevel) const
    {
        const PipelineResourceStorageResource* resourceObjects = mResourceStorage->GetPerResourceData(textureName);
        assert_format(resourceObjects && resourceObjects->Texture, "Resource ", textureName.ToString(), " does not exist");

        Foundation::Name passName = mPassGraph->Nodes()[mGraphNodeIndex].PassMetadata().Name;

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

    uint32_t ResourceProvider::GetSamplerIndex(Foundation::Name samplerName) const
    {
        const HAL::SamplerDescriptor* descriptor = mResourceStorage->GetSamplerDescriptor(samplerName);
        assert_format(descriptor, "Sampler ", samplerName.ToString(), " does not exist");
        return descriptor->IndexInHeapRange();
    }

    const HAL::TextureProperties& ResourceProvider::GetTextureProperties(Foundation::Name resourceName) const
    {
        const PipelineResourceStorageResource* resourceObjects = mResourceStorage->GetPerResourceData(resourceName);
        assert_format(resourceObjects && resourceObjects->Texture, "Resource ", resourceName.ToString(), " does not exist");
        return resourceObjects->Texture->Properties();
    }

}
