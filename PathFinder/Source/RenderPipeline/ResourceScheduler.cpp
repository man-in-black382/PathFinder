#include "ResourceScheduler.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    ResourceScheduler::ResourceScheduler(PipelineResourceStorage* manager, RenderPassUtilityProvider* utilityProvider)
        : mResourceStorage{ manager }, mUtilityProvider{ utilityProvider } {}

    void ResourceScheduler::NewRenderTarget(Foundation::Name resourceName, std::optional<NewTextureProperties> properties)
    {
        NewTextureProperties props = FillMissingFields(properties);
        Foundation::Name passName = mResourceStorage->CurrentPassGraphNode().PassMetadata.Name;

        // Wait until all allocations are requested by render passed to detach scheduling algorithm from scheduling order
        auto delayedAllocationRequest = [this, props, resourceName, passGraphNode = mResourceStorage->CurrentPassGraphNode()]
        {
            HAL::ResourceFormat::FormatVariant format = *props.ShaderVisibleFormat;

            if (props.TypelessFormat) format = *props.TypelessFormat;

            PipelineResourceStorageResource& resourceData = mResourceStorage->QueueTexturesAllocationIfNeeded(
                resourceName, format, *props.Kind, *props.Dimensions, *props.ClearValues, *props.MipCount, props.TextureCount
            );

            for (auto textureIdx = 0u; textureIdx < resourceData.SchedulingInfo.ResourceCount(); ++textureIdx)
            {
                for (auto subresourceIdx = 0u; subresourceIdx < resourceData.SchedulingInfo.SubresourceCount(); ++subresourceIdx)
                {
                    PipelineResourceSchedulingInfo::PassInfo& passInfo = resourceData.SchedulingInfo.AllocateInfoForPass(passGraphNode, textureIdx, subresourceIdx);
                    passInfo.RequestedState = HAL::ResourceState::RenderTarget;
                    passInfo.SetTextureRTRequested();

                    if (props.TypelessFormat)
                    {
                        passInfo.ShaderVisibleFormat = props.ShaderVisibleFormat;
                    }
                }
            }

            // Register resource usage for the render pass
            PipelineResourceStoragePass* passData = mResourceStorage->GetPerPassData(passGraphNode.PassMetadata.Name);
            passData->ScheduledResourceNames.insert(resourceName);
        };

        mResourceStorage->AddResourceCreationAction(delayedAllocationRequest, resourceName, passName);
    }

    void ResourceScheduler::NewDepthStencil(Foundation::Name resourceName, std::optional<NewDepthStencilProperties> properties)
    {
        NewDepthStencilProperties props = FillMissingFields(properties);
        Foundation::Name passName = mResourceStorage->CurrentPassGraphNode().PassMetadata.Name;

        // Wait until all allocations are requested by render passed to detach scheduling algorithm from scheduling order
        auto delayedAllocationRequest = [this, props, resourceName, passGraphNode = mResourceStorage->CurrentPassGraphNode()]
        {
            HAL::DepthStencilClearValue clearValue{ 1.0, 0 };
            
            PipelineResourceStorageResource& resourceData = mResourceStorage->QueueTexturesAllocationIfNeeded(
                resourceName, *props.Format, HAL::TextureKind::Texture2D, *props.Dimensions, clearValue, 1, props.TextureCount
            );

            for (auto textureIdx = 0u; textureIdx < resourceData.SchedulingInfo.ResourceCount(); ++textureIdx)
            {
                for (auto subresourceIdx = 0u; subresourceIdx < resourceData.SchedulingInfo.SubresourceCount(); ++subresourceIdx)
                {
                    PipelineResourceSchedulingInfo::PassInfo& passInfo = resourceData.SchedulingInfo.AllocateInfoForPass(passGraphNode, textureIdx, subresourceIdx);
                    passInfo.RequestedState = HAL::ResourceState::DepthWrite;
                    passInfo.SetTextureDSRequested();
                }
            }

            // Register resource usage for the render pass
            PipelineResourceStoragePass* passData = mResourceStorage->GetPerPassData(passGraphNode.PassMetadata.Name);
            passData->ScheduledResourceNames.insert(resourceName);
        };

        mResourceStorage->AddResourceCreationAction(delayedAllocationRequest, resourceName, passName);
    }

    void ResourceScheduler::NewTexture(Foundation::Name resourceName, std::optional<NewTextureProperties> properties)
    {
        NewTextureProperties props = FillMissingFields(properties);
        Foundation::Name passName = mResourceStorage->CurrentPassGraphNode().PassMetadata.Name;

        // Wait until all allocations are requested by render passed to detach scheduling algorithm from scheduling order
        auto delayedAllocationRequest = [this, props, resourceName, passGraphNode = mResourceStorage->CurrentPassGraphNode()]
        {
            HAL::ResourceFormat::FormatVariant format = *props.ShaderVisibleFormat;

            if (props.TypelessFormat)
            {
                format = *props.TypelessFormat;
            }

            PipelineResourceStorageResource& resourceData = mResourceStorage->QueueTexturesAllocationIfNeeded(
                resourceName, format, *props.Kind, *props.Dimensions, *props.ClearValues, *props.MipCount, props.TextureCount
            );

            for (auto textureIdx = 0u; textureIdx < resourceData.SchedulingInfo.ResourceCount(); ++textureIdx)
            {
                for (auto subresourceIdx = 0u; subresourceIdx < resourceData.SchedulingInfo.SubresourceCount(); ++subresourceIdx)
                {
                    PipelineResourceSchedulingInfo::PassInfo& passInfo = resourceData.SchedulingInfo.AllocateInfoForPass(passGraphNode, textureIdx, subresourceIdx);
                    passInfo.RequestedState = HAL::ResourceState::UnorderedAccess;
                    passInfo.SetTextureUARequested();

                    if (props.TypelessFormat)
                    {
                        passInfo.ShaderVisibleFormat = props.ShaderVisibleFormat;
                    }
                }
            }

            // Register resource usage for the render pass
            PipelineResourceStoragePass* passData = mResourceStorage->GetPerPassData(passGraphNode.PassMetadata.Name);
            passData->ScheduledResourceNames.insert(resourceName);
        };

        mResourceStorage->AddResourceCreationAction(delayedAllocationRequest, resourceName, passName);
    }

    void ResourceScheduler::UseRenderTarget(const ResourceKey& resourceKey, const MipList& mips, std::optional<HAL::ColorFormat> concreteFormat)
    {
        Foundation::Name passName = mResourceStorage->CurrentPassGraphNode().PassMetadata.Name;

        auto delayedUsageRequest = [=, passGraphNode = mResourceStorage->CurrentPassGraphNode()]
        {
            PipelineResourceStorageResource* resourceData = mResourceStorage->GetPerResourceData(resourceKey.ResourceName());

            assert_format(resourceData, "Cannot use non-scheduled render target");

            bool isTypeless = std::holds_alternative<HAL::TypelessColorFormat>(*resourceData->SchedulingInfo.ResourceFormat().DataType());

            assert_format(concreteFormat || !isTypeless, "Redefinition of Render target format is not allowed");
            assert_format(!concreteFormat || isTypeless, "Render target is typeless and concrete color format was not provided");

            auto fillInfoForCurrentPass = [&](uint64_t subresourceIdx)
            {
                PipelineResourceSchedulingInfo::PassInfo& passInfo = resourceData->SchedulingInfo.AllocateInfoForPass(
                    passGraphNode, resourceKey.IndexInArray(), subresourceIdx
                );

                passInfo.RequestedState = HAL::ResourceState::RenderTarget;
                passInfo.SetTextureRTRequested();

                if (isTypeless)
                {
                    passInfo.ShaderVisibleFormat = concreteFormat;
                }
            };

            FillCurrentPassInfo(resourceData, mips, fillInfoForCurrentPass);

            // Register resource usage for the render pass
            PipelineResourceStoragePass* passData = mResourceStorage->GetPerPassData(passGraphNode.PassMetadata.Name);
            passData->ScheduledResourceNames.insert(resourceKey.ResourceName());
        };

        mResourceStorage->AddResourceUsageAction(delayedUsageRequest);
    }

    void ResourceScheduler::UseDepthStencil(const ResourceKey& resourceKey)
    {
        Foundation::Name passName = mResourceStorage->CurrentPassGraphNode().PassMetadata.Name;

        auto delayedUsageRequest = [=, passGraphNode = mResourceStorage->CurrentPassGraphNode()]
        {
            PipelineResourceStorageResource* resourceData = mResourceStorage->GetPerResourceData(resourceKey.ResourceName());

            assert_format(resourceData, "Cannot reuse non-scheduled depth-stencil texture");
            assert_format(std::holds_alternative<HAL::DepthStencilFormat>(*resourceData->SchedulingInfo.ResourceFormat().DataType()), "Cannot reuse non-depth-stencil texture");

            for (auto subresourceIdx = 0u; subresourceIdx < resourceData->SchedulingInfo.SubresourceCount(); ++subresourceIdx)
            {
                PipelineResourceSchedulingInfo::PassInfo& passInfo = resourceData->SchedulingInfo.AllocateInfoForPass(
                    passGraphNode, resourceKey.IndexInArray(), subresourceIdx
                );

                passInfo.RequestedState = HAL::ResourceState::DepthWrite;
                passInfo.SetTextureDSRequested();
            }

            // Register resource usage for the render pass
            PipelineResourceStoragePass* passData = mResourceStorage->GetPerPassData(passGraphNode.PassMetadata.Name);
            passData->ScheduledResourceNames.insert(resourceKey.ResourceName());
        };

        mResourceStorage->AddResourceUsageAction(delayedUsageRequest);
    }

    void ResourceScheduler::ReadTexture(const ResourceKey& resourceKey, const MipList& mips, std::optional<HAL::ColorFormat> concreteFormat)
    {
        Foundation::Name passName = mResourceStorage->CurrentPassGraphNode().PassMetadata.Name;

        auto delayedUsageRequest = [=, passGraphNode = mResourceStorage->CurrentPassGraphNode()]
        {
            PipelineResourceStorageResource* resourceData = mResourceStorage->GetPerResourceData(resourceKey.ResourceName());

            assert_format(resourceData, "Cannot read non-scheduled texture");

            bool isTypeless = std::holds_alternative<HAL::TypelessColorFormat>(*resourceData->SchedulingInfo.ResourceFormat().DataType());

            assert_format(concreteFormat || !isTypeless, "Redefinition of texture format is not allowed");
            assert_format(!concreteFormat || isTypeless, "Texture is typeless and concrete color format was not provided");

            auto fillInfoForCurrentPass = [&](uint64_t subresourceIdx)
            {
                PipelineResourceSchedulingInfo::PassInfo& passInfo = resourceData->SchedulingInfo.AllocateInfoForPass(
                    passGraphNode, resourceKey.IndexInArray(), subresourceIdx
                );

                passInfo.RequestedState = HAL::ResourceState::PixelShaderAccess | HAL::ResourceState::NonPixelShaderAccess;

                if (std::holds_alternative<HAL::DepthStencilFormat>(*resourceData->SchedulingInfo.ResourceFormat().DataType()))
                {
                    passInfo.RequestedState |= HAL::ResourceState::DepthRead;
                }

                if (isTypeless)
                {
                    passInfo.ShaderVisibleFormat = concreteFormat;
                }

                passInfo.SetTextureSRRequested();
            };

            FillCurrentPassInfo(resourceData, mips, fillInfoForCurrentPass);

            // Register resource usage for the render pass
            PipelineResourceStoragePass* passData = mResourceStorage->GetPerPassData(passGraphNode.PassMetadata.Name);
            passData->ScheduledResourceNames.insert(resourceKey.ResourceName());
        };

        mResourceStorage->AddResourceUsageAction(delayedUsageRequest);
    }

    void ResourceScheduler::ReadWriteTexture(const ResourceKey& resourceKey, const MipList& mips, std::optional<HAL::ColorFormat> concreteFormat)
    {
        Foundation::Name passName = mResourceStorage->CurrentPassGraphNode().PassMetadata.Name;

        auto delayedUsageRequest = [=, passGraphNode = mResourceStorage->CurrentPassGraphNode()]
        {
            PipelineResourceStorageResource* resourceData = mResourceStorage->GetPerResourceData(resourceKey.ResourceName());

            assert_format(resourceData, "Cannot read/write non-scheduled texture");

            bool isTypeless = std::holds_alternative<HAL::TypelessColorFormat>(*resourceData->SchedulingInfo.ResourceFormat().DataType());

            assert_format(concreteFormat || !isTypeless, "Redefinition of texture format is not allowed");
            assert_format(!concreteFormat || isTypeless, "Texture is typeless and concrete color format was not provided");

            auto fillInfoForCurrentPass = [&](uint64_t subresourceIdx)
            {
                PipelineResourceSchedulingInfo::PassInfo& passInfo = resourceData->SchedulingInfo.AllocateInfoForPass(
                    passGraphNode, resourceKey.IndexInArray(), subresourceIdx
                );

                passInfo.RequestedState = HAL::ResourceState::UnorderedAccess;
                passInfo.SetTextureUARequested();

                if (isTypeless)
                {
                    passInfo.ShaderVisibleFormat = concreteFormat;
                }
            };

            FillCurrentPassInfo(resourceData, mips, fillInfoForCurrentPass);

            // Register resource usage for the render pass
            PipelineResourceStoragePass* passData = mResourceStorage->GetPerPassData(passGraphNode.PassMetadata.Name);
            passData->ScheduledResourceNames.insert(resourceKey.ResourceName());
        };

        mResourceStorage->AddResourceUsageAction(delayedUsageRequest);
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
            mUtilityProvider->DefaultRenderSurfaceDescription.RenderTargetFormat(),
            HAL::TextureKind::Texture2D,
            mUtilityProvider->DefaultRenderSurfaceDescription.Dimensions(),
            std::nullopt,
            HAL::ColorClearValue{ 0, 0, 0, 1 },
            1
        };

        if (properties)
        {
            if (properties->Kind) filledProperties.Kind = properties->Kind;
            if (properties->Dimensions) filledProperties.Dimensions = properties->Dimensions;
            if (properties->ShaderVisibleFormat) filledProperties.ShaderVisibleFormat = properties->ShaderVisibleFormat;
            if (properties->TypelessFormat) filledProperties.TypelessFormat = properties->TypelessFormat;
            if (properties->ClearValues) filledProperties.ClearValues = properties->ClearValues;
            if (properties->MipCount) filledProperties.MipCount = properties->MipCount;
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
            if (properties->Format) filledProperties.Format = properties->Format;
            if (properties->Dimensions) filledProperties.Dimensions = properties->Dimensions;
            if (properties->MipCount) filledProperties.MipCount = properties->MipCount;
        }

        filledProperties.TextureCount = std::max(properties->TextureCount, 1ull);

        return filledProperties;
    }

}
