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
            uint16_t mipCount = 1
        );

        TextureResource(
            const Device& device,
            const Heap& heap, 
            uint64_t heapOffset,
            ResourceFormat::FormatVariant format,
            ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions,
            const ResourceFormat::ClearValue& optimizedClearValue,
            ResourceState initialStateMask,
            ResourceState expectedStateMask,
            uint16_t mipCount = 1
        );

        bool IsArray() const;
        virtual uint32_t SubresourceCount() const override;

        static ResourceFormat ConstructResourceFormat(
            const Device* device,
            ResourceFormat::FormatVariant format,
            ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions,
            uint16_t mipCount,
            const ResourceFormat::ClearValue& optimizedClearValue);

    protected:
        Geometry::Dimensions mDimensions;
        ResourceFormat::FormatVariant mFormat;
        ResourceFormat::TextureKind mKind;
        ResourceFormat::ClearValue mOptimizedClearValue;
        uint16_t mMipCount;

    public:
        inline const Geometry::Dimensions& Dimensions() const { return mDimensions; }
        inline ResourceFormat::TextureKind Kind() const { return mKind; }
        inline ResourceFormat::FormatVariant Format() const { return mFormat; }
        inline ResourceFormat::ClearValue OptimizedClearValue() const { return mOptimizedClearValue; }
    };

}

