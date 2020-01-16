#pragma once

#include "Resource.hpp"

namespace HAL
{

    class Texture : public Resource
    {
    public:
        Texture(const Microsoft::WRL::ComPtr<ID3D12Resource>& existingResourcePtr);

        Texture(
            const Device& device,
            ResourceFormat::FormatVariant format,
            TextureKind kind,
            const Geometry::Dimensions& dimensions,
            const ClearValue& optimizedClearValue,
            ResourceState initialStateMask,
            ResourceState expectedStateMask,
            uint16_t mipCount = 1
        );

        Texture(
            const Device& device,
            const Heap& heap, 
            uint64_t heapOffset,
            ResourceFormat::FormatVariant format,
            TextureKind kind,
            const Geometry::Dimensions& dimensions,
            const ClearValue& optimizedClearValue,
            ResourceState initialStateMask,
            ResourceState expectedStateMask,
            uint16_t mipCount = 1
        );

        bool IsArray() const;
        virtual uint32_t SubresourceCount() const override;

        static ResourceFormat ConstructResourceFormat(
            const Device* device,
            ResourceFormat::FormatVariant format,
            TextureKind kind,
            const Geometry::Dimensions& dimensions,
            uint16_t mipCount,
            const ClearValue& optimizedClearValue);

    protected:
        Geometry::Dimensions mDimensions;
        ResourceFormat::FormatVariant mFormat;
        TextureKind mKind;
        ClearValue mOptimizedClearValue;
        uint16_t mMipCount;

    public:
        inline const Geometry::Dimensions& Dimensions() const { return mDimensions; }
        inline TextureKind Kind() const { return mKind; }
        inline ResourceFormat::FormatVariant Format() const { return mFormat; }
        inline ClearValue OptimizedClearValue() const { return mOptimizedClearValue; }
    };

}

