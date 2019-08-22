#pragma once

#include "Resource.hpp"

namespace HAL
{

    class TextureResource : public Resource
    {
    public:
        TextureResource(const Microsoft::WRL::ComPtr<ID3D12Resource>& existingResourcePtr);

        TextureResource(
            const Device& device,
            ResourceFormat::FormatVariant format,
            ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions,
            const ResourceFormat::ClearValue& optimizedClearValue,
            ResourceState initialStateMask,
            ResourceState expectedStateMask,
            std::optional<CPUAccessibleHeapType> heapType = std::nullopt
        );

        bool IsArray() const;

    protected:
        Geometry::Dimensions mDimensions;
        ResourceFormat::FormatVariant mFormat;
        ResourceFormat::TextureKind mKind;

    public:
        inline const Geometry::Dimensions& Dimensions() const { return mDimensions; }
        inline ResourceFormat::TextureKind Kind() const { return mKind; }
        inline ResourceFormat::FormatVariant Format() const { return mFormat; }
    };

}

