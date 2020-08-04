#pragma once

#include "Resource.hpp"

namespace HAL
{

    class Texture : public Resource
    {
    public:
        Texture(const Microsoft::WRL::ComPtr<ID3D12Resource>& existingResourcePtr);
        Texture(const Device& device, const TextureProperties& properties);
        Texture(const Device& device, const Heap& heap, uint64_t heapOffset, const TextureProperties& properties);

        bool IsArray() const;
        virtual uint32_t SubresourceCount() const override;

        bool CanImplicitlyPromoteFromCommonStateToState(ResourceState state) const override;
        bool CanImplicitlyDecayToCommonStateFromState(ResourceState state) const override;

    protected:
        TextureProperties mProperties;

    private:
        TextureProperties ConstructProperties() const;

    public:
        inline const TextureProperties& Properties() const { return mProperties; }
        inline const Geometry::Dimensions& Dimensions() const { return mProperties.Dimensions; }
        inline TextureKind Kind() const { return mProperties.Kind; }
        inline FormatVariant Format() const { return mProperties.Format; }
        inline ClearValue OptimizedClearValue() const { return mProperties.OptimizedClearValue; }
    };

}

