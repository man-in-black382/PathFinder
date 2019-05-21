#pragma once

#include <d3d12.h>
#include <optional>
#include <variant>

#include "../Geometry/Dimensions.hpp"

namespace HAL
{
    class ResourceFormat
    {
    public:
        enum class TypelessColor {
            R8, RG8, RGBA8,
            R16, RG16, RGBA16,
            R32, RG32, RGB32, RGBA32            
        };

        enum class Color {
            R8_Usigned_Norm, RG8_Usigned_Norm, RGBA8_Usigned_Norm,

            R8_Signed, RG8_Signed, RGBA8_Signed,
            R8_Unsigned, RG8_Unsigned, RGBA8_Unsigned,

            R16_Float, RG16_Float, RGBA16_Float,
            R16_Signed, RG16_Signed, RGBA16_Signed,
            R16_Unsigned, RG16_Unsigned, RGBA16_Unsigned,

            R32_Float, RG32_Float, RGB32_Float, RGBA32_Float,
            R32_Signed, RG32_Signed, RGB32_Signed, RGBA32_Signed,
            R32_Unsigned, RG32_Unsigned, RGB32_Unsigned, RGBA32_Unsigned
        };

        enum DepthStencil {
            Depth24_Float_Stencil8_Unsigned, Depth32_Float
        };

        enum class BufferKind { Buffer };

        enum class TextureKind { Texture1D, Texture2D, Texture3D };

        using FormatVariant = std::variant<TypelessColor, Color, DepthStencil>;

        ResourceFormat(std::optional<FormatVariant> dataType, TextureKind kind, const Geometry::Dimensions& dimensions);
        ResourceFormat(std::optional<FormatVariant> dataType, BufferKind , const Geometry::Dimensions& dimensions);

        static constexpr DXGI_FORMAT D3DFormat(TypelessColor type);
        static constexpr DXGI_FORMAT D3DFormat(Color type);
        static constexpr DXGI_FORMAT D3DFormat(DepthStencil type);

    private:
        void ResolveDemensionData(BufferKind kind, const Geometry::Dimensions& dimensions);
        void ResolveDemensionData(TextureKind kind, const Geometry::Dimensions& dimensions);

        D3D12_RESOURCE_DESC mDesc{};

    public:
        inline const auto& D3DResourceDescription() const { return mDesc; }
    };

}

