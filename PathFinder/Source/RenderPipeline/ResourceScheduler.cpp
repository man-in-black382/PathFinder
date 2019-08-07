#include "ResourceScheduler.hpp"

namespace PathFinder
{

    ResourceScheduler::ResourceScheduler(ResourceStorage* manager)
        : mResourceStorage{ manager } {}

    void ResourceScheduler::WillRenderToRenderTarget(
        Foundation::Name resourceName, HAL::ResourceFormat::Color dataFormat,
        HAL::ResourceFormat::TextureKind kind, const Geometry::Dimensions& dimensions)
    {
        mResourceStorage->MarkResourceNameAsScheduled(resourceName);
        mResourceStorage->RegisterStateForResource(resourceName, HAL::ResourceState::RenderTarget);

        mResourceStorage->QueueTextureAllocationIfNeeded<HAL::ColorTextureResource>(
            resourceName, 
            [this, resourceName](const HAL::ColorTextureResource& resource) 
        { 
            mResourceStorage->mDescriptorStorage.EmplaceRTDescriptorIfNeeded(resourceName, resource);
        }, 
            *mResourceStorage->mDevice,
            dataFormat, 
            kind, 
            dimensions,
            HAL::Resource::ColorClearValue{ 0.f, 0.f, 0.f, 0.f });
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

        mResourceStorage->QueueTextureAllocationIfNeeded<HAL::TypelessTextureResource>(
            resourceName, 
            [this, resourceName, shaderVisisbleFormat](const HAL::TypelessTextureResource& resource)
        {
            mResourceStorage->mDescriptorStorage.EmplaceRTDescriptorIfNeeded(resourceName, resource, shaderVisisbleFormat);
        }, 
            *mResourceStorage->mDevice,
            dataFormat, 
            kind, 
            dimensions,
            HAL::Resource::ColorClearValue{ 0.f, 0.f, 0.f, 0.f });
    }

    void ResourceScheduler::WillRenderToRenderTarget(Foundation::Name resourceName)
    {
        mResourceStorage->MarkResourceNameAsScheduled(resourceName);
        mResourceStorage->RegisterStateForResource(resourceName, HAL::ResourceState::RenderTarget);

        mResourceStorage->QueueTextureAllocationIfNeeded<HAL::ColorTextureResource>(
            resourceName, 
            [this, resourceName](const HAL::ColorTextureResource& resource)
        {
            mResourceStorage->mDescriptorStorage.EmplaceRTDescriptorIfNeeded(resourceName, resource);
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

}
