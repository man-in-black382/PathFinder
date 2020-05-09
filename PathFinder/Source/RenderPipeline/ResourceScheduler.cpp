#include "ResourceScheduler.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    ResourceScheduler::ResourceScheduler(PipelineResourceStorage* manager, RenderPassUtilityProvider* utilityProvider)
        : mResourceStorage{ manager }, mUtilityProvider{ utilityProvider } {}

    void ResourceScheduler::NewRenderTarget(Foundation::Name resourceName, std::optional<NewTextureProperties> properties)
    {
        EnsureSingleSchedulingRequestForCurrentPass(resourceName);

        assert_format(!mResourceStorage->IsResourceAllocationScheduled(resourceName), "New render target has already been scheduled");

        HAL::ColorClearValue clearValue{ 0.0, 0.0, 0.0, 1.0 };
        NewTextureProperties props = FillMissingFields(properties);
        HAL::ResourceFormat::FormatVariant format = *props.ShaderVisibleFormat;

        if (props.TypelessFormat) 
        {
            format = *props.TypelessFormat;
        }

        PipelineResourceSchedulingInfo* schedulingInfo = mResourceStorage->QueueTexturesAllocationIfNeeded(
            resourceName, format, *props.Kind, *props.Dimensions, clearValue, *props.MipCount, props.TextureCount
        );

        for (auto textureIdx = 0u; textureIdx < schedulingInfo->ResourceCount(); ++textureIdx)
        {
            auto& passData = schedulingInfo->AllocateMetadataForPass(mResourceStorage->CurrentPassGraphNode(), textureIdx);
            passData.RequestedState = HAL::ResourceState::RenderTarget;
            passData.CreateTextureRTDescriptor = true;

            if (props.TypelessFormat)
            {
                passData.ShaderVisibleFormat = props.ShaderVisibleFormat;
            }
        }

        mResourceStorage->RegisterResourceNameForCurrentPass(resourceName);
    }

    void ResourceScheduler::NewDepthStencil(Foundation::Name resourceName, std::optional<NewDepthStencilProperties> properties)
    {
        EnsureSingleSchedulingRequestForCurrentPass(resourceName);

        assert_format(!mResourceStorage->IsResourceAllocationScheduled(resourceName), "New depth-stencil texture has already been scheduled");

        HAL::DepthStencilClearValue clearValue{ 1.0, 0 };
        NewDepthStencilProperties props = FillMissingFields(properties);

        PipelineResourceSchedulingInfo* schedulingInfo = mResourceStorage->QueueTexturesAllocationIfNeeded(
            resourceName, *props.Format, HAL::TextureKind::Texture2D, *props.Dimensions, clearValue, 1, props.TextureCount
        );

        for (auto textureIdx = 0u; textureIdx < schedulingInfo->ResourceCount(); ++textureIdx)
        {
            auto& passData = schedulingInfo->AllocateMetadataForPass(mResourceStorage->CurrentPassGraphNode(), textureIdx);
            passData.RequestedState = HAL::ResourceState::DepthWrite;
            passData.CreateTextureDSDescriptor = true;
        }

        mResourceStorage->RegisterResourceNameForCurrentPass(resourceName);
    }

    void ResourceScheduler::NewTexture(Foundation::Name resourceName, std::optional<NewTextureProperties> properties)
    {
        EnsureSingleSchedulingRequestForCurrentPass(resourceName);

        assert_format(!mResourceStorage->IsResourceAllocationScheduled(resourceName), "Texture creation has already been scheduled");

        HAL::ColorClearValue clearValue{ 0.0, 0.0, 0.0, 1.0 };
        NewTextureProperties props = FillMissingFields(properties);

        HAL::ResourceFormat::FormatVariant format = *props.ShaderVisibleFormat;

        if (props.TypelessFormat)
        {
            format = *props.TypelessFormat;
        }

        PipelineResourceSchedulingInfo* schedulingInfo = mResourceStorage->QueueTexturesAllocationIfNeeded(
            resourceName, format, *props.Kind, *props.Dimensions, clearValue, *props.MipCount, props.TextureCount
        );

        for (auto textureIdx = 0u; textureIdx < schedulingInfo->ResourceCount(); ++textureIdx)
        {
            auto& passData = schedulingInfo->AllocateMetadataForPass(mResourceStorage->CurrentPassGraphNode(), textureIdx);
            passData.RequestedState = HAL::ResourceState::UnorderedAccess;
            passData.CreateTextureUADescriptor = true;

            if (props.TypelessFormat)
            {
                passData.ShaderVisibleFormat = props.ShaderVisibleFormat;
            }
        }

        mResourceStorage->RegisterResourceNameForCurrentPass(resourceName);
    }

    void ResourceScheduler::UseRenderTarget(const ResourceKey& resourceKey, std::optional<HAL::ColorFormat> concreteFormat)
    {
        EnsureSingleSchedulingRequestForCurrentPass(resourceKey.ResourceName());

        assert_format(mResourceStorage->IsResourceAllocationScheduled(resourceKey.ResourceName()), "Cannot use non-scheduled render target");

        PipelineResourceStorageResource* resourceData = mResourceStorage->GetPerResourceData(resourceKey.ResourceName());
        bool isTypeless = std::holds_alternative<HAL::TypelessColorFormat>(*resourceData->SchedulingInfo.ResourceFormat().DataType());

        assert_format(concreteFormat || !isTypeless, "Redefinition of Render target format is not allowed");
        assert_format(!concreteFormat || isTypeless, "Render target is typeless and concrete color format was not provided");

        auto& passData = resourceData->SchedulingInfo.AllocateMetadataForPass(mResourceStorage->CurrentPassGraphNode(), resourceKey.IndexInArray());
        passData.RequestedState = HAL::ResourceState::RenderTarget;
        passData.CreateTextureRTDescriptor = true;

        if (isTypeless) passData.ShaderVisibleFormat = concreteFormat;

        mResourceStorage->RegisterResourceNameForCurrentPass(resourceKey.ResourceName());
    }

    void ResourceScheduler::UseDepthStencil(const ResourceKey& resourceKey)
    {
        assert_format(mResourceStorage->IsResourceAllocationScheduled(resourceKey.ResourceName()), "Cannot reuse non-scheduled depth-stencil texture");

        PipelineResourceStorageResource* resourceData = mResourceStorage->GetPerResourceData(resourceKey.ResourceName());

        assert_format(std::holds_alternative<HAL::DepthStencilFormat>(*resourceData->SchedulingInfo.ResourceFormat().DataType()), "Cannot reuse non-depth-stencil texture");

        auto& passData = resourceData->SchedulingInfo.AllocateMetadataForPass(mResourceStorage->CurrentPassGraphNode(), resourceKey.IndexInArray());
        passData.RequestedState = HAL::ResourceState::DepthWrite;
        passData.CreateTextureDSDescriptor = true;

        mResourceStorage->RegisterResourceNameForCurrentPass(resourceKey.ResourceName());
    }

    void ResourceScheduler::ReadTexture(const ResourceKey& resourceKey, std::optional<HAL::ColorFormat> concreteFormat)
    {
        assert_format(mResourceStorage->IsResourceAllocationScheduled(resourceKey.ResourceName()), "Cannot read non-scheduled texture");

        PipelineResourceStorageResource* resourceData = mResourceStorage->GetPerResourceData(resourceKey.ResourceName());

        bool isTypeless = std::holds_alternative<HAL::TypelessColorFormat>(*resourceData->SchedulingInfo.ResourceFormat().DataType());

        assert_format(concreteFormat || !isTypeless, "Redefinition of texture format is not allowed");
        assert_format(!concreteFormat || isTypeless, "Texture is typeless and concrete color format was not provided");

        auto& passData = resourceData->SchedulingInfo.AllocateMetadataForPass(mResourceStorage->CurrentPassGraphNode(), resourceKey.IndexInArray());
        passData.RequestedState = HAL::ResourceState::PixelShaderAccess | HAL::ResourceState::NonPixelShaderAccess;

        if (std::holds_alternative<HAL::DepthStencilFormat>(*resourceData->SchedulingInfo.ResourceFormat().DataType()))
        {
            passData.RequestedState |= HAL::ResourceState::DepthRead;
        } 

        if (isTypeless) passData.ShaderVisibleFormat = concreteFormat;

        passData.CreateTextureSRDescriptor = true;

        mResourceStorage->RegisterResourceNameForCurrentPass(resourceKey.ResourceName());
    }

    void ResourceScheduler::ReadWriteTexture(const ResourceKey& resourceKey, std::optional<HAL::ColorFormat> concreteFormat)
    {
        assert_format(mResourceStorage->IsResourceAllocationScheduled(resourceKey.ResourceName()), "Cannot read/write non-scheduled texture");

        PipelineResourceStorageResource* resourceData = mResourceStorage->GetPerResourceData(resourceKey.ResourceName());
        bool isTypeless = std::holds_alternative<HAL::TypelessColorFormat>(*resourceData->SchedulingInfo.ResourceFormat().DataType());

        assert_format(concreteFormat || !isTypeless, "Redefinition of texture format is not allowed");
        assert_format(!concreteFormat || isTypeless, "Texture is typeless and concrete color format was not provided");

        auto& passData = resourceData->SchedulingInfo.AllocateMetadataForPass(mResourceStorage->CurrentPassGraphNode(), resourceKey.IndexInArray());
        passData.RequestedState = HAL::ResourceState::UnorderedAccess;
        passData.CreateTextureUADescriptor = true;

        if (isTypeless) passData.ShaderVisibleFormat = concreteFormat;

        mResourceStorage->RegisterResourceNameForCurrentPass(resourceKey.ResourceName());
    }

    void ResourceScheduler::ReadBuffer(const ResourceKey& resourceKey, BufferReadContext readContext)
    {
        assert_format(false, "Not implemented");
    }

    void ResourceScheduler::ReadWriteBuffer(const ResourceKey& resourceKey)
    {
        assert_format(false, "Not implemented");
    }

    ResourceScheduler::NewTextureProperties ResourceScheduler::FillMissingFields(std::optional<NewTextureProperties> properties)
    {
        NewTextureProperties filledProperties{
            HAL::TextureKind::Texture2D,
            mUtilityProvider->DefaultRenderSurfaceDescription.Dimensions(),
            mUtilityProvider->DefaultRenderSurfaceDescription.RenderTargetFormat(),
            std::nullopt,
            1
        };

        if (properties)
        {
            if (properties->Kind) filledProperties.Kind = *properties->Kind;
            if (properties->Dimensions) filledProperties.Dimensions = *properties->Dimensions;
            if (properties->ShaderVisibleFormat) filledProperties.ShaderVisibleFormat = *properties->ShaderVisibleFormat;
            if (properties->TypelessFormat) filledProperties.TypelessFormat = *properties->TypelessFormat;
            if (properties->MipCount) filledProperties.MipCount = *properties->MipCount;
        }

        filledProperties.TextureCount = std::max(properties->TextureCount, 1ull);

        return filledProperties;
    }

    ResourceScheduler::NewDepthStencilProperties ResourceScheduler::FillMissingFields(std::optional<NewDepthStencilProperties> properties)
    {
        NewDepthStencilProperties filledProperties{
            mUtilityProvider->DefaultRenderSurfaceDescription.DepthStencilFormat(),
            mUtilityProvider->DefaultRenderSurfaceDescription.Dimensions(),
            1
        };

        if (properties)
        {
            if (properties->Format) filledProperties.Format = *properties->Format;
            if (properties->Dimensions) filledProperties.Dimensions = *properties->Dimensions;
            if (properties->MipCount) filledProperties.MipCount = *properties->MipCount;
        }

        filledProperties.TextureCount = std::max(properties->TextureCount, 1ull);

        return filledProperties;
    }

    void ResourceScheduler::EnsureSingleSchedulingRequestForCurrentPass(Foundation::Name resourceName) const
    {
        const auto& names = mResourceStorage->ScheduledResourceNamesForCurrentPass();
        bool isResourceScheduledInCurrentPass = names.find(resourceName) != names.end();
        assert_format(!isResourceScheduledInCurrentPass, "Resource ", resourceName.ToString(), " is already scheduled for this pass. Resources can only be scheduled once per pass");
    }

}
