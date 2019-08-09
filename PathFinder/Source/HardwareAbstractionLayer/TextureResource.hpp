#pragma once

#include "Resource.hpp"

namespace HAL
{

    class TextureResource : public Resource
    {
    public:
        using Resource::Resource;

        bool IsArray() const;

    protected:
        Geometry::Dimensions mDimensions;
        ResourceFormat::TextureKind mKind;

    public:
        inline const Geometry::Dimensions& Dimensions() const { return mDimensions; }
        inline ResourceFormat::TextureKind Kind() const { return mKind; }
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
            const ClearValue& optimizedClearValue,
            ResourceState initialStateMask,
            ResourceState expectedStateMask,
            std::optional<CPUAccessibleHeapType> heapType = std::nullopt
        );

        ~ColorTextureResource() = default;

    private:
        ResourceFormat::Color mDataFormat;

    public:
        inline ResourceFormat::Color DataFormat() const { return mDataFormat; };
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
            const ClearValue& optimizedClearValue,
            ResourceState initialStateMask,
            ResourceState expectedStateMask,
            std::optional<CPUAccessibleHeapType> heapType = std::nullopt
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
            const ClearValue& optimizedClearValue,
            ResourceState initialStateMask,
            ResourceState expectedStateMask,
            std::optional<CPUAccessibleHeapType> heapType = std::nullopt
        );

        ~DepthStencilTextureResource() = default;
    };

}

