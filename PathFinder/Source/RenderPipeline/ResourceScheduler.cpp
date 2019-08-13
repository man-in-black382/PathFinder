#include "ResourceScheduler.hpp"

namespace PathFinder
{

    ResourceScheduler::ResourceScheduler(ResourceStorage* manager)
        : mResourceStorage{ manager } {}

    void ResourceScheduler::NewRenderTarget(Foundation::Name resourceName, std::optional<NewTextureProperties> properties)
    {
       /* if (mResourceStorage->IsResourceScheduled(resourceName)) throw std::invalid_argument("New render target has already been scheduled");

        mResourceStorage->MarkResourceNameAsScheduled(resourceName);
        mResourceStorage->RegisterStateForResource(resourceName, HAL::ResourceState::RenderTarget);

        HAL::Resource::ColorClearValue clearValue{ 0.0, 0.0, 0.0, 1.0 };
        NewTextureProperties props = FillMissingFields(properties);

        if (props.TypelessFormat)
        {
            auto descriptorCreationAction = [this, resourceName, props](const HAL::TypelessTexture& resource)
            {
                mResourceStorage->mDescriptorStorage.EmplaceRTDescriptorIfNeeded(resourceName, resource, *props.ShaderVisibleFormat);
                mResourceStorage->RegisterColorFormatForResource(resourceName, *props.ShaderVisibleFormat);
            };

            mResourceStorage->QueueTextureAllocationIfNeeded<HAL::TypelessTexture>(resourceName, descriptorCreationAction, *properties->TypelessFormat, *props.Kind, *props.Dimensions, clearValue);
        } 
        else {
            auto descriptorCreationAction = [this, resourceName](const HAL::ColorTexture& resource)
            {
                mResourceStorage->mDescriptorStorage.EmplaceRTDescriptorIfNeeded(resourceName, resource);
                mResourceStorage->RegisterColorFormatForResource(resourceName, resource.DataFormat());
            };

            mResourceStorage->QueueTextureAllocationIfNeeded<HAL::ColorTexture>(resourceName, descriptorCreationAction, *props.ShaderVisibleFormat, *props.Kind, *props.Dimensions, clearValue);
        }*/
    }

    void ResourceScheduler::NewDepthStencil(Foundation::Name resourceName, std::optional<NewDepthStencilProperties> properties)
    {
      /*  if (mResourceStorage->IsResourceScheduled(resourceName)) throw std::invalid_argument("New depth-stencil texture has already been scheduled");

        mResourceStorage->MarkResourceNameAsScheduled(resourceName);
        mResourceStorage->RegisterStateForResource(resourceName, HAL::ResourceState::DepthWrite);

        HAL::Resource::DepthStencilClearValue clearValue{ 1.0, 0 };
        NewDepthStencilProperties props = FillMissingFields(properties);

        auto descriptorCreationAction = [this, resourceName](const HAL::DepthStencilTexture& resource)
        {
            mResourceStorage->mDescriptorStorage.EmplaceDSDescriptorIfNeeded(resourceName, resource);
        };

        mResourceStorage->QueueTextureAllocationIfNeeded<HAL::DepthStencilTexture>(resourceName, descriptorCreationAction, *props.Format, *props.Dimensions, clearValue);*/
    }

    void ResourceScheduler::NewTexture(Foundation::Name resourceName, std::optional<NewTextureProperties> properties)
    {
      /*  if (mResourceStorage->IsResourceScheduled(resourceName)) throw std::invalid_argument("Texture creation has already been scheduled");

        mResourceStorage->MarkResourceNameAsScheduled(resourceName);
        mResourceStorage->RegisterStateForResource(resourceName, HAL::ResourceState::UnorderedAccess);

        HAL::Resource::ColorClearValue clearValue{ 0.0, 0.0, 0.0, 1.0 };
        NewTextureProperties props = FillMissingFields(properties);

        if (props.TypelessFormat)
        {
            auto descriptorCreationAction = [this, resourceName, props](const HAL::TypelessTexture& resource)
            {
                mResourceStorage->mDescriptorStorage.EmplaceUADescriptorIfNeeded(resourceName, resource, *props.ShaderVisibleFormat);
                mResourceStorage->RegisterColorFormatForResource(resourceName, *props.ShaderVisibleFormat);
            };

            mResourceStorage->QueueTextureAllocationIfNeeded<HAL::TypelessTexture>(resourceName, descriptorCreationAction, *properties->TypelessFormat, *props.Kind, *props.Dimensions, clearValue);
        }
        else {
            auto descriptorCreationAction = [this, resourceName](const HAL::ColorTexture& resource)
            {
                mResourceStorage->mDescriptorStorage.EmplaceUADescriptorIfNeeded(resourceName, resource);
                mResourceStorage->RegisterColorFormatForResource(resourceName, resource.DataFormat());
            };

            mResourceStorage->QueueTextureAllocationIfNeeded<HAL::ColorTexture>(resourceName, descriptorCreationAction, *props.ShaderVisibleFormat, *props.Kind, *props.Dimensions, clearValue);
        }*/
    }

    void ResourceScheduler::NewBuffer()
    {

    }

    void ResourceScheduler::UseRenderTarget(Foundation::Name resourceName, std::optional<HAL::ResourceFormat::Color> concreteFormat)
    {
       /* HAL::Resource* resource = mResourceStorage->GetResource(resourceName);

        if (!resource) throw std::invalid_argument("Cannot use render target that does not exist");

        if (concreteFormat)
        {
            if (HAL::TypelessTexture* existingRT = dynamic_cast<HAL::TypelessTexture *>(resource))
            {
                mResourceStorage->MarkResourceNameAsScheduled(resourceName);
                mResourceStorage->RegisterStateForResource(resourceName, HAL::ResourceState::RenderTarget);
                mResourceStorage->RegisterColorFormatForResource(resourceName, *concreteFormat)
            }
            else {
                throw std::invalid_argument("Redefinition of render target format for typed render target is not allowed");
            }
        }
        else if (HAL::ColorTexture* existingRT = dynamic_cast<HAL::ColorTexture *>(resource))
        {
            mResourceStorage->MarkResourceNameAsScheduled(resourceName);
            mResourceStorage->RegisterStateForResource(resourceName, HAL::ResourceState::RenderTarget);
            mResourceStorage->RegisterColorFormatForResource(resourceName, existingRT->DataFormat())
        } 
        else {
            throw std::invalid_argument("Resource name does not belong to a render target");
        }*/
    }

    void ResourceScheduler::UseDepthStencil(Foundation::Name resourceName)
    {
       /* HAL::Resource* resource = mResourceStorage->GetResource(resourceName);

        if (!resource) throw std::invalid_argument("Cannot use depth stencil that does not exist");

        if (HAL::DepthStencilTexture* existingRT = dynamic_cast<HAL::DepthStencilTexture *>(resource))
        {
            mResourceStorage->MarkResourceNameAsScheduled(resourceName);
            mResourceStorage->RegisterStateForResource(resourceName, HAL::ResourceState::DepthWrite);
        }
        else {
            throw std::invalid_argument("Resource name does not belong to a depth stencil texture");
        }        */
    }

    void ResourceScheduler::ReadTexture(Foundation::Name resourceName, std::optional<HAL::ResourceFormat::Color> concreteFormat)
    {

    }

    void ResourceScheduler::ReadBuffer()
    {

    }

    void ResourceScheduler::ReadWriteTexture(Foundation::Name resourceName, std::optional<HAL::ResourceFormat::Color> concreteFormat)
    {

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

    /* void ResourceScheduler::WillRenderToRenderTarget(
        Foundation::Name resourceName, HAL::ResourceFormat::Color dataFormat,
        HAL::ResourceFormat::TextureKind kind, const Geometry::Dimensions& dimensions)
    {
        
    }

    void ResourceScheduler::WillRenderToRenderTarget(
        Foundation::Name resourceName,
        HAL::ResourceFormat::TypelessColor dataFormat,
        HAL::ResourceFormat::Color shaderVisisbleFormat,
        HAL::ResourceFormat::TextureKind kind, 
        const Geometry::Dimensions& dimensions)
    {
        mResourceStorage->MarkResourceNameAsScheduled(resourceName);
        mResourceStorage->RegisterStateForResource(resourceName, HAL::ResourceState::RenderTarget);

        auto passName = mResourceStorage->mCurrentPassName;

       
    }

    void ResourceScheduler::WillRenderToRenderTarget(Foundation::Name resourceName)
    {
        mResourceStorage->MarkResourceNameAsScheduled(resourceName);
        mResourceStorage->RegisterStateForResource(resourceName, HAL::ResourceState::RenderTarget);

        auto passName = mResourceStorage->mCurrentPassName;

        mResourceStorage->QueueTextureAllocationIfNeeded<HAL::ColorTextureResource>(
            resourceName, 
            [this, resourceName, passName](const HAL::ColorTextureResource& resource)
        {
            mResourceStorage->mDescriptorStorage.EmplaceRTDescriptorIfNeeded(resourceName, resource);
            mResourceStorage->mResourceShaderVisibleFormatMap[passName][resourceName] = resource.DataFormat();
        }, 
            *mResourceStorage->mDevice,
            mResourceStorage->mDefaultRenderSurface.RenderTargetFormat(),
            HAL::ResourceFormat::TextureKind::Texture2D, 
            mResourceStorage->mDefaultRenderSurface.Dimensions(),
            HAL::Resource::ColorClearValue{ 0.f, 0.f, 0.f, 0.f });
    }

    void ResourceScheduler::WillRenderToDepthStencil(
        Foundation::Name resourceName,
        HAL::ResourceFormat::DepthStencil dataFormat,
        const Geometry::Dimensions& dimensions)
    {
        mResourceStorage->MarkResourceNameAsScheduled(resourceName);
        mResourceStorage->RegisterStateForResource(resourceName, HAL::ResourceState::DepthWrite);

        mResourceStorage->QueueTextureAllocationIfNeeded<HAL::DepthStencilTextureResource>(
            resourceName,
            [this, resourceName](const HAL::DepthStencilTextureResource& resource)
        {
            mResourceStorage->mDescriptorStorage.EmplaceDSDescriptorIfNeeded(resourceName, resource);
        }, 
            *mResourceStorage->mDevice,
            dataFormat, 
            dimensions,
            HAL::Resource::DepthStencilClearValue{ 1.f, 0 });
    }

    void ResourceScheduler::WillRenderToDepthStencil(Foundation::Name resourceName)
    {
        mResourceStorage->MarkResourceNameAsScheduled(resourceName);
        mResourceStorage->RegisterStateForResource(resourceName, HAL::ResourceState::DepthWrite);

        mResourceStorage->QueueTextureAllocationIfNeeded<HAL::DepthStencilTextureResource>(
            resourceName,
            [this, resourceName](const HAL::DepthStencilTextureResource& resource)
        {
            mResourceStorage->mDescriptorStorage.EmplaceDSDescriptorIfNeeded(resourceName, resource);
        },
            *mResourceStorage->mDevice,
            mResourceStorage->mDefaultRenderSurface.DepthStencilFormat(),
            mResourceStorage->mDefaultRenderSurface.Dimensions(),
            HAL::Resource::DepthStencilClearValue{ 1.f, 0 });
    }
*/
}
