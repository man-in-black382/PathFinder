#pragma once

#include <d3d12.h>
#include <optional>
#include <variant>
#include <array>

#include "Device.hpp"
#include "ResourceState.hpp"

#include "../Geometry/Dimensions.hpp"

namespace HAL
{
    class ResourceFormat
    {
    public:
        using ColorClearValue = std::array<float, 4>;

        struct DepthStencilClearValue
        {
            float Depth;
            uint8_t Stencil;
        };

        using ClearValue = std::variant<ColorClearValue, DepthStencilClearValue>;

        using UnderlyingType = uint8_t;

        enum class TypelessColor : UnderlyingType {
            R8 = 1, RG8 = 2, RGBA8 = 3,
            R16 = 4, RG16 = 5, RGBA16 = 6,
            R32 = 7, RG32 = 8, RGB32 = 9, RGBA32 = 10            
        };

        enum class Color : UnderlyingType {
            R8_Usigned_Norm = 11, RG8_Usigned_Norm = 12, RGBA8_Usigned_Norm = 13,

            R8_Signed = 14, RG8_Signed = 15, RGBA8_Signed = 16,
            R8_Unsigned = 17, RG8_Unsigned = 18, RGBA8_Unsigned = 19,

            R16_Float = 20, RG16_Float = 21, RGBA16_Float = 22,
            R16_Signed = 23, RG16_Signed = 24, RGBA16_Signed = 25,
            R16_Unsigned = 26, RG16_Unsigned = 27, RGBA16_Unsigned = 28,

            R32_Float = 29, RG32_Float = 30, RGB32_Float = 31, RGBA32_Float = 32,
            R32_Signed = 33, RG32_Signed = 34, RGB32_Signed = 35, RGBA32_Signed = 36,
            R32_Unsigned = 37, RG32_Unsigned = 38, RGB32_Unsigned = 39, RGBA32_Unsigned = 40
        };

        enum DepthStencil : UnderlyingType {
            Depth24_Float_Stencil8_Unsigned = 41, Depth32_Float = 42
        };

        enum class BufferKind { Buffer };

        enum class TextureKind { Texture1D, Texture2D, Texture3D };

        using FormatVariant = std::variant<TypelessColor, Color, DepthStencil>;

        ResourceFormat(const Device& device, FormatVariant dataType, TextureKind kind, const Geometry::Dimensions& dimensions, uint16_t mipCount, ClearValue optimizedClearValue);
        ResourceFormat(const Device& device, std::optional<FormatVariant> dataType, BufferKind kind, const Geometry::Dimensions& dimensions);

        static DXGI_FORMAT D3DFormat(TypelessColor type);
        static DXGI_FORMAT D3DFormat(Color type);
        static DXGI_FORMAT D3DFormat(DepthStencil type);

        static FormatVariant FormatFromD3DFormat(DXGI_FORMAT format);

        bool CanResourceImplicitlyPromoteFromCommonStateToState(ResourceState state) const;
        bool CanResourceImplicitlyDecayToCommonStateFromState(ResourceState state) const;

    private:
        using KindVariant = std::variant<BufferKind, TextureKind>;

        void ResolveDemensionData(BufferKind kind, const Geometry::Dimensions& dimensions);
        void ResolveDemensionData(TextureKind kind, const Geometry::Dimensions& dimensions);
        void QueryAllocationInfo(const Device& device);

        D3D12_RESOURCE_DESC mDesc{};
        std::optional<FormatVariant> mDataType;
        std::optional<D3D12_CLEAR_VALUE> mClearValue;
        uint64_t mResourceAlignment = 0;
        uint64_t mResourceSizeInBytes = 0;
        KindVariant mKind;

    public:
        inline const D3D12_RESOURCE_DESC& D3DResourceDescription() const { return mDesc; }
        inline const D3D12_CLEAR_VALUE* D3DOptimizedClearValue() const { return mClearValue ? &(*mClearValue) : nullptr; }
        inline auto ResourceAlighnment() const { return mResourceAlignment; }
        inline auto ResourceSizeInBytes() const { return mResourceSizeInBytes; }
        inline auto DataType() const { return mDataType; }
    };

}

