#include "ResourceScheduler.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    ResourceScheduler::ResourceScheduler(PipelineResourceStorage* manager)
        : mResourceStorage{ manager } {}

    void ResourceScheduler::NewRenderTarget(Foundation::Name resourceName, std::optional<NewTextureProperties> properties)
    {
        EnsureSingleSchedulingRequestForCurrentPass(resourceName);

        assert_format(!mResourceStorage->IsResourceAllocationScheduled(resourceName), "New render target has already been scheduled");

        HAL::ResourceFormat::ColorClearValue clearValue{ 0.0, 0.0, 0.0, 1.0 };
        NewTextureProperties props = FillMissingFields(properties);

        HAL::ResourceFormat::FormatVariant format = *props.ShaderVisibleFormat;

        if (props.TypelessFormat) 
        {
            format = *props.TypelessFormat;
        }

        PipelineResourceAllocator* allocator = mResourceStorage->QueueTextureAllocationIfNeeded(
            resourceName, format, *props.Kind, *props.Dimensions, clearValue
        );

        auto& passData = allocator->mPerPassData[mResourceStorage->mCurrentPassName];
        passData.RequestedState = HAL::ResourceState::RenderTarget;
        passData.RTInserter = &ResourceDescriptorStorage::EmplaceRTDescriptorIfNeeded;

        if (props.TypelessFormat)
        {
            passData.ShaderVisibleFormat = props.ShaderVisibleFormat;
        }

        allocator->mFormat = format;

        mResourceStorage->RegisterResourceNameForCurrentPass(resourceName);
    }

    void ResourceScheduler::NewDepthStencil(Foundation::Name resourceName, std::optional<NewDepthStencilProperties> properties)
    {
        EnsureSingleSchedulingRequestForCurrentPass(resourceName);

        assert_format(!mResourceStorage->IsResourceAllocationScheduled(resourceName), "New depth-stencil texture has already been scheduled");

        HAL::ResourceFormat::DepthStencilClearValue clearValue{ 1.0, 0 };
        NewDepthStencilProperties props = FillMissingFields(properties);

        PipelineResourceAllocator* allocator = mResourceStorage->QueueTextureAllocationIfNeeded(
            resourceName, *props.Format, HAL::ResourceFormat::TextureKind::Texture2D, *props.Dimensions, clearValue
        );

        auto& passData = allocator->mPerPassData[mResourceStorage->mCurrentPassName];
        passData.RequestedState = HAL::ResourceState::DepthWrite;
        passData.DSInserter = &ResourceDescriptorStorage::EmplaceDSDescriptorIfNeeded;

        allocator->mFormat = *props.Format;

        mResourceStorage->RegisterResourceNameForCurrentPass(resourceName);
    }

    void ResourceScheduler::NewTexture(Foundation::Name resourceName, std::optional<NewTextureProperties> properties)
    {
        EnsureSingleSchedulingRequestForCurrentPass(resourceName);

        assert_format(!mResourceStorage->IsResourceAllocationScheduled(resourceName), "Texture creation has already been scheduled");

        HAL::ResourceFormat::ColorClearValue clearValue{ 0.0, 0.0, 0.0, 1.0 };
        NewTextureProperties props = FillMissingFields(properties);

        HAL::ResourceFormat::FormatVariant format = *props.ShaderVisibleFormat;

        if (props.TypelessFormat)
        {
            format = *props.TypelessFormat;
        }

        PipelineResourceAllocator* allocator = mResourceStorage->QueueTextureAllocationIfNeeded(
            resourceName, format, *props.Kind, *props.Dimensions, clearValue
        );

        auto& passData = allocator->mPerPassData[mResourceStorage->mCurrentPassName];
        passData.RequestedState = HAL::ResourceState::UnorderedAccess;
        passData.UAInserter = &ResourceDescriptorStorage::EmplaceUADescriptorIfNeeded;

        if (props.TypelessFormat) 
        {
            passData.ShaderVisibleFormat = props.ShaderVisibleFormat;
        }

        allocator->mFormat = format;

        mResourceStorage->RegisterResourceNameForCurrentPass(resourceName);
    }

    void ResourceScheduler::NewBuffer()
    {

    }

    void ResourceScheduler::UseRenderTarget(Foundation::Name resourceName, std::optional<HAL::ResourceFormat::Color> concreteFormat)
    {
        EnsureSingleSchedulingRequestForCurrentPass(resourceName);

        assert_format(mResourceStorage->IsResourceAllocationScheduled(resourceName), "Cannot use non-scheduled render target");

        PipelineResourceAllocator* allocator = mResourceStorage->GetResourceAllocator(resourceName);
        bool isTypeless = std::holds_alternative<HAL::ResourceFormat::TypelessColor>(allocator->mFormat);

        assert_format(concreteFormat || !isTypeless, "Redefinition of Render target format is not allowed");
        assert_format(!concreteFormat || isTypeless, "Render target is typeless and concrete color format was not provided");

        auto& passData = allocator->mPerPassData[mResourceStorage->mCurrentPassName];
        passData.RequestedState = HAL::ResourceState::RenderTarget;
        passData.RTInserter = &ResourceDescriptorStorage::EmplaceRTDescriptorIfNeeded;

        if (isTypeless) passData.ShaderVisibleFormat = concreteFormat;

        mResourceStorage->RegisterResourceNameForCurrentPass(resourceName);
    }

    void ResourceScheduler::UseDepthStencil(Foundation::Name resourceName)
    {
        EnsureSingleSchedulingRequestForCurrentPass(resourceName);

        assert_format(mResourceStorage->IsResourceAllocationScheduled(resourceName), "Cannot reuse non-scheduled depth-stencil texture");

        PipelineResourceAllocator* allocator = mResourceStorage->GetResourceAllocator(resourceName);

        assert_format(std::holds_alternative<HAL::ResourceFormat::DepthStencil>(allocator->mFormat), "Cannot reuse non-depth-stencil texture");

        auto& passData = allocator->mPerPassData[mResourceStorage->mCurrentPassName];
        passData.RequestedState = HAL::ResourceState::DepthWrite;
        passData.DSInserter = &ResourceDescriptorStorage::EmplaceDSDescriptorIfNeeded;

        mResourceStorage->RegisterResourceNameForCurrentPass(resourceName);
    }

    void ResourceScheduler::ReadTexture(Foundation::Name resourceName, std::optional<HAL::ResourceFormat::Color> concreteFormat)
    {
        EnsureSingleSchedulingRequestForCurrentPass(resourceName);

        assert_format(mResourceStorage->IsResourceAllocationScheduled(resourceName), "Cannot read non-scheduled texture");

        PipelineResourceAllocator* allocator = mResourceStorage->GetResourceAllocator(resourceName);

        bool isTypeless = std::holds_alternative<HAL::ResourceFormat::TypelessColor>(allocator->mFormat);

        assert_format(concreteFormat || !isTypeless, "Redefinition of texture format is not allowed");
        assert_format(!concreteFormat || isTypeless, "Texture is typeless and concrete color format was not provided");

        auto& passData = allocator->mPerPassData[mResourceStorage->mCurrentPassName];
        passData.RequestedState = HAL::ResourceState::PixelShaderAccess | HAL::ResourceState::NonPixelShaderAccess;

        if (std::holds_alternative<HAL::ResourceFormat::DepthStencil>(allocator->mFormat))
        {
            passData.RequestedState |= HAL::ResourceState::DepthRead;
        } 

        if (isTypeless) passData.ShaderVisibleFormat = concreteFormat;

        passData.SRInserter = &ResourceDescriptorStorage::EmplaceSRDescriptorIfNeeded;

        mResourceStorage->RegisterResourceNameForCurrentPass(resourceName);
    }

    void ResourceScheduler::ReadBuffer()
    {

    }

    void ResourceScheduler::ReadWriteTexture(Foundation::Name resourceName, std::optional<HAL::ResourceFormat::Color> concreteFormat)
    {
        EnsureSingleSchedulingRequestForCurrentPass(resourceName);

        assert_format(mResourceStorage->IsResourceAllocationScheduled(resourceName), "Cannot read/write non-scheduled texture");

        PipelineResourceAllocator* allocator = mResourceStorage->GetResourceAllocator(resourceName);
        bool isTypeless = std::holds_alternative<HAL::ResourceFormat::TypelessColor>(allocator->mFormat);

        assert_format(concreteFormat || !isTypeless, "Redefinition of texture format is not allowed");
        assert_format(!concreteFormat || isTypeless, "Texture is typeless and concrete color format was not provided");

        auto& passData = allocator->mPerPassData[mResourceStorage->mCurrentPassName];
        passData.RequestedState = HAL::ResourceState::UnorderedAccess;
        passData.UAInserter = &ResourceDescriptorStorage::EmplaceUADescriptorIfNeeded;

        if (isTypeless) passData.ShaderVisibleFormat = concreteFormat;

        mResourceStorage->RegisterResourceNameForCurrentPass(resourceName);
    }

    void ResourceScheduler::ReadWriteBuffer()
    {

    }

    ResourceScheduler::NewTextureProperties ResourceScheduler::FillMissingFields(std::optional<NewTextureProperties> properties)
    {
        NewTextureProperties filledProperties{
            HAL::ResourceFormat::TextureKind::Texture2D,
            mResourceStorage->mDefaultRenderSurface.Dimensions(),
            mResourceStorage->mDefaultRenderSurface.RenderTargetFormat(),
            std::nullopt,
            0
        };

        if (properties)
        {
            if (properties->Kind) filledProperties.Kind = *properties->Kind;
            if (properties->Dimensions) filledProperties.Dimensions = *properties->Dimensions;
            if (properties->ShaderVisibleFormat) filledProperties.ShaderVisibleFormat = *properties->ShaderVisibleFormat;
            if (properties->TypelessFormat) filledProperties.TypelessFormat = *properties->TypelessFormat;
            if (properties->MipCount) filledProperties.MipCount = *properties->MipCount;
        }

        return filledProperties;
    }

    ResourceScheduler::NewDepthStencilProperties ResourceScheduler::FillMissingFields(std::optional<NewDepthStencilProperties> properties)
    {
        NewDepthStencilProperties filledProperties{
            mResourceStorage->mDefaultRenderSurface.DepthStencilFormat(),
            mResourceStorage->mDefaultRenderSurface.Dimensions()
        };

        if (properties)
        {
            if (properties->Format) filledProperties.Format = *properties->Format;
            if (properties->Dimensions) filledProperties.Dimensions = *properties->Dimensions;
        }

        return filledProperties;
    }

    void ResourceScheduler::EnsureSingleSchedulingRequestForCurrentPass(ResourceName resourceName)
    {
        const auto& names = mResourceStorage->ScheduledResourceNamesForCurrentPass();
        bool isResourceScheduledInCurrentPass = names.find(resourceName) != names.end();
        assert_format(!isResourceScheduledInCurrentPass, "Resource ", resourceName.ToSring(), " is already scheduled for this pass. Resources can only be scheduled once per pass");
    }

}
