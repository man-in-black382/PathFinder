#pragma once

#include "Resource.hpp"

namespace HAL
{

    class TextureResource : public Resource
    {
    public:
        using Resource::Resource;

    protected:
        Geometry::Dimensions mDimensions;

    public:
        inline Geometry::Dimensions& Dimensions() { return mDimensions; }
    };



    class ColorTextureResource : public TextureResource
    {
    public:
        using TextureResource::TextureResource;

        ColorTextureResource(
            const Device& device,
            ResourceFormat::Color dataType,
            ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions,
            ResourceState initialStateMask,
            ResourceState expectedStateMask,
            HeapType heapType = HeapType::Default
        );

        ~ColorTextureResource() = default;
    };

    class TypelessTextureResource : public TextureResource
    {
    public:
        using TextureResource::TextureResource;

        TypelessTextureResource(
            const Device& device,
            ResourceFormat::TypelessColor dataType,
            ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions,
            ResourceState initialStateMask,
            ResourceState expectedStateMask,
            HeapType heapType = HeapType::Default
        );

        ~TypelessTextureResource() = default;
    };

    class DepthStencilTextureResource : public TextureResource
    {
    public:
        using TextureResource::TextureResource;

        DepthStencilTextureResource(
            const Device& device,
            ResourceFormat::DepthStencil dataType,
            const Geometry::Dimensions& dimensions,
            ResourceState initialStateMask,
            ResourceState expectedStateMask,
            HeapType heapType = HeapType::Default
        );

        ~DepthStencilTextureResource() = default;
    };

}

