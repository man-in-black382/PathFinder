#pragma once

#include "Resource.hpp"

namespace HAL
{

    class Texture : public Resource
    {
    public:
        struct Properties
        {
            ResourceFormat::FormatVariant Format;
            TextureKind Kind;
            Geometry::Dimensions Dimensions;
            ClearValue OptimizedClearValue;
            ResourceState InitialStateMask;
            ResourceState ExpectedStateMask;
            uint16_t MipCount;

            Properties(HAL::ResourceFormat::FormatVariant format, HAL::TextureKind kind, const Geometry::Dimensions& dimensions,
                const HAL::ClearValue& optimizedClearValue, HAL::ResourceState initialStateMask, HAL::ResourceState expectedStateMask, uint16_t mipCount = 1);

            Properties(HAL::ResourceFormat::FormatVariant format, HAL::TextureKind kind, const Geometry::Dimensions& dimensions,
                const HAL::ClearValue& optimizedClearValue, HAL::ResourceState initialStateMask, uint16_t mipCount = 1);

            Properties(HAL::ResourceFormat::FormatVariant format, HAL::TextureKind kind, const Geometry::Dimensions& dimensions,
                HAL::ResourceState initialStateMask, uint16_t mipCount = 1);
        };

        Texture(const Microsoft::WRL::ComPtr<ID3D12Resource>& existingResourcePtr);
        Texture(const Device& device, const Properties& properties);
        Texture(const Device& device, const Heap& heap, uint64_t heapOffset, const Properties& properties);

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

