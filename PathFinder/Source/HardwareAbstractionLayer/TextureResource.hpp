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
            ResourceFormat::FormatVariant format,
            ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions,
            const ResourceFormat::ClearValue& optimizedClearValue,
            CPUAccessibleHeapType heapType,
            uint16_t mipCount = 1
        );

        bool IsArray() const;

        virtual bool CanImplicitlyPromoteFromCommonStateToState(HAL::ResourceState state) const override;
        virtual bool CanImplicitlyDecayToCommonStateFromState(HAL::ResourceState state) const override;

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

