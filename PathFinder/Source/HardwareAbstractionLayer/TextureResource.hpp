#pragma once

#include "Resource.hpp"

namespace HAL
{

    class ColorTextureResource : public Resource
    {
    public:
        using Resource::Resource;
        ColorTextureResource(
            const Device& device,
            ResourceFormat::Color dataType,
            ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions,
            ResourceState initialStateMask,
            ResourceState expectedStateMask,
            HeapType heapType = HeapType::Default
        );
    };

    class TypelessTextureResource : public Resource
    {
    public:
        using Resource::Resource;
        TypelessTextureResource(
            const Device& device,
            ResourceFormat::TypelessColor dataType,
            ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions,
            ResourceState initialStateMask,
            ResourceState expectedStateMask,
            HeapType heapType = HeapType::Default
        );
    };

    class DepthStencilTextureResource : public Resource
    {
    public:
          using Resource::Resource;
          DepthStencilTextureResource(
              const Device& device,
              ResourceFormat::DepthStencil dataType,
              const Geometry::Dimensions& dimensions,
              ResourceState initialStateMask,
              ResourceState expectedStateMask,
              HeapType heapType = HeapType::Default
          );
    };

}

