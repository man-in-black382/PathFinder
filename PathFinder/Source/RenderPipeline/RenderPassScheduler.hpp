#pragma once

#include "BufferResource.hpp"
#include "TextureResource.hpp"

namespace PathFinder
{

    class RenderPassScheduler
    {
    public:
        template <class BufferContentT>
        PrepareNewBufferResource(uint64_t capacity, ResourceState state);

        ColorTextureResource* GetNewColorTextureResource(
            ResourceFormat::Color dataType, ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions, ResourceState state
        );

        TypelessTextureResource* GetNewTypelessTextureResource(
            ResourceFormat::TypelessColor dataType, ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions, ResourceState state
        );

        DepthStencilTextureResource* GetNewDepthStencilTextureResource(
            ResourceFormat::DepthStencil dataType,
            const Geometry::Dimensions& dimensions,
            ResourceState state
        );
    };
   
}
