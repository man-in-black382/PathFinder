#include "ResourceFormat.hpp"

#include "../Foundation/Assert.hpp"
#include "../Foundation/Visitor.hpp"

namespace HAL
{

    ResourceFormat::ResourceFormat(std::optional<FormatVariant> dataType, TextureKind kind, const Geometry::Dimensions& dimensions, ClearValue optimizedClearValue)
    {
        if (dataType) 
        {
            std::visit([this](auto&& t) { mDesc.Format = D3DFormat(t); }, dataType.value());
        }
        
        ResolveDemensionData(kind, dimensions);

        mClearValue = D3D12_CLEAR_VALUE{};
        mClearValue->Format = mDesc.Format;

        std::visit(Foundation::MakeVisitor(
            [this](const ColorClearValue& value)
        {
            mClearValue->Color[0] = value[0];
            mClearValue->Color[1] = value[1];
            mClearValue->Color[2] = value[2];
            mClearValue->Color[3] = value[3];
        },
            [this](const DepthStencilClearValue& value)
        {
            mClearValue->DepthStencil.Depth = value.Depth;
            mClearValue->DepthStencil.Stencil = value.Stencil;
        }),
            optimizedClearValue);

        mDesc.MipLevels = 1;
    }

    ResourceFormat::ResourceFormat(std::optional<FormatVariant> dataType, BufferKind kind, const Geometry::Dimensions& dimensions)
    {
        if (dataType)
        {
            std::visit([this](auto&& t) { mDesc.Format = D3DFormat(t); }, dataType.value());
        }

        ResolveDemensionData(kind, dimensions);
    }

    void ResourceFormat::ResolveDemensionData(BufferKind kind, const Geometry::Dimensions& dimensions)
    {
        mDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        mDesc.Width = dimensions.Width;
        mDesc.Height = 1;
        mDesc.DepthOrArraySize = 1;
        mDesc.MipLevels = 1;
        mDesc.SampleDesc.Count = 1;
        mDesc.SampleDesc.Quality = 0;
        mDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    }

    void ResourceFormat::ResolveDemensionData(TextureKind kind, const Geometry::Dimensions& dimensions)
    {
        switch (kind)
        {
        case TextureKind::Texture3D: mDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D; break;
        case TextureKind::Texture2D: mDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; break;
        case TextureKind::Texture1D: mDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D; break;
        }

        mDesc.Height = (UINT)dimensions.Height;
        mDesc.Width = dimensions.Width;
        mDesc.DepthOrArraySize = (UINT16)dimensions.Depth;
        mDesc.SampleDesc.Count = 1;
        mDesc.SampleDesc.Quality = 0;
    }

    DXGI_FORMAT ResourceFormat::D3DFormat(TypelessColor type)
    {
        switch (type)
        {
        case TypelessColor::R8:                return DXGI_FORMAT_R8_TYPELESS;
        case TypelessColor::RG8:               return DXGI_FORMAT_R8G8_TYPELESS;
        case TypelessColor::RGBA8:             return DXGI_FORMAT_R8G8B8A8_TYPELESS;
        case TypelessColor::R16:               return DXGI_FORMAT_R16_TYPELESS;
        case TypelessColor::RG16:              return DXGI_FORMAT_R16G16_TYPELESS;
        case TypelessColor::RGBA16:            return DXGI_FORMAT_R16G16B16A16_TYPELESS;
        case TypelessColor::R32:               return DXGI_FORMAT_R32_TYPELESS;
        case TypelessColor::RG32:              return DXGI_FORMAT_R32G32_TYPELESS;
        case TypelessColor::RGB32:             return DXGI_FORMAT_R32G32B32_TYPELESS;
        case TypelessColor::RGBA32:            return DXGI_FORMAT_R32G32B32A32_TYPELESS;
        }
    }

    DXGI_FORMAT ResourceFormat::D3DFormat(Color type)
    {
        switch (type)
        {
        case Color::R8_Usigned_Norm:        return DXGI_FORMAT_R8_UNORM;
        case Color::RG8_Usigned_Norm:      return DXGI_FORMAT_R8G8_UNORM;
        case Color::RGBA8_Usigned_Norm:  return DXGI_FORMAT_R8G8B8A8_UNORM;

        case Color::R8_Signed:         return DXGI_FORMAT_R8_SINT;
        case Color::RG8_Signed:        return DXGI_FORMAT_R8G8_SINT;
        case Color::RGBA8_Signed:      return DXGI_FORMAT_R8G8B8A8_SINT;

        case Color::R8_Unsigned:       return DXGI_FORMAT_R8_UINT;
        case Color::RG8_Unsigned:      return DXGI_FORMAT_R8G8_UINT;
        case Color::RGBA8_Unsigned:    return DXGI_FORMAT_R8G8B8A8_UINT;

        case Color::R16_Float:         return DXGI_FORMAT_R16_FLOAT;
        case Color::RG16_Float:        return DXGI_FORMAT_R16G16_FLOAT;
        case Color::RGBA16_Float:      return DXGI_FORMAT_R16G16B16A16_FLOAT;

        case Color::R16_Signed:        return DXGI_FORMAT_R16_SINT;
        case Color::RG16_Signed:       return DXGI_FORMAT_R16G16_SINT;
        case Color::RGBA16_Signed:     return DXGI_FORMAT_R16G16B16A16_SINT;

        case Color::R16_Unsigned:      return DXGI_FORMAT_R16_UINT;
        case Color::RG16_Unsigned:     return DXGI_FORMAT_R16G16_UINT;
        case Color::RGBA16_Unsigned:   return DXGI_FORMAT_R16G16B16A16_UINT;

        case Color::R32_Float:         return DXGI_FORMAT_R32_FLOAT;
        case Color::RG32_Float:        return DXGI_FORMAT_R32G32_FLOAT;
        case Color::RGB32_Float:       return DXGI_FORMAT_R32G32B32_FLOAT;
        case Color::RGBA32_Float:      return DXGI_FORMAT_R32G32B32A32_FLOAT;

        case Color::R32_Signed:        return DXGI_FORMAT_R32_SINT;
        case Color::RG32_Signed:       return DXGI_FORMAT_R32G32_SINT;
        case Color::RGB32_Signed:      return DXGI_FORMAT_R32G32B32_SINT;
        case Color::RGBA32_Signed:     return DXGI_FORMAT_R32G32B32A32_SINT;

        case Color::R32_Unsigned:      return DXGI_FORMAT_R32_UINT;
        case Color::RG32_Unsigned:     return DXGI_FORMAT_R32G32_UINT;
        case Color::RGB32_Unsigned:    return DXGI_FORMAT_R32G32B32_UINT;
        case Color::RGBA32_Unsigned:   return DXGI_FORMAT_R32G32B32A32_UINT;
        }
    }

    DXGI_FORMAT ResourceFormat::D3DFormat(DepthStencil type)
    {
        switch (type)
        {
        case DepthStencil::Depth24_Float_Stencil8_Unsigned: return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case DepthStencil::Depth32_Float:                   return DXGI_FORMAT_D32_FLOAT;
        }
    }

    ResourceFormat::FormatVariant ResourceFormat::FormatFromD3DFormat(DXGI_FORMAT format)
    {
        switch (format)
        {
        case DXGI_FORMAT_R8_TYPELESS: return TypelessColor::R8;
        case DXGI_FORMAT_R8G8_TYPELESS: return TypelessColor::RG8;
        case DXGI_FORMAT_R8G8B8A8_TYPELESS: return TypelessColor::RGBA8;
        case DXGI_FORMAT_R16_TYPELESS: return TypelessColor::R16;
        case DXGI_FORMAT_R16G16_TYPELESS: return TypelessColor::RG16;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS: return TypelessColor::RGBA16;
        case DXGI_FORMAT_R32_TYPELESS: return TypelessColor::R32;
        case DXGI_FORMAT_R32G32_TYPELESS: return TypelessColor::RG32;
        case DXGI_FORMAT_R32G32B32_TYPELESS: return TypelessColor::RGB32;
        case DXGI_FORMAT_R32G32B32A32_TYPELESS: return TypelessColor::RGBA32;

        case DXGI_FORMAT_R8_UNORM: return Color::R8_Usigned_Norm;
        case DXGI_FORMAT_R8G8_UNORM: return Color::RG8_Usigned_Norm;
        case DXGI_FORMAT_R8G8B8A8_UNORM: return Color::RGBA8_Usigned_Norm;

        case DXGI_FORMAT_R8_SINT: return Color::R8_Signed;
        case DXGI_FORMAT_R8G8_SINT: return Color::RG8_Signed;
        case DXGI_FORMAT_R8G8B8A8_SINT: return Color::RGBA8_Signed;

        case DXGI_FORMAT_R8_UINT: return Color::R8_Unsigned;
        case DXGI_FORMAT_R8G8_UINT: return Color::RG8_Unsigned;
        case DXGI_FORMAT_R8G8B8A8_UINT: return Color::RGBA8_Unsigned;

        case DXGI_FORMAT_R16_FLOAT: return Color::R16_Float;
        case DXGI_FORMAT_R16G16_FLOAT: return Color::RG16_Float;
        case DXGI_FORMAT_R16G16B16A16_FLOAT: return Color::RGBA16_Float;

        case DXGI_FORMAT_R16_SINT: return Color::R16_Signed;
        case DXGI_FORMAT_R16G16_SINT: return Color::RG16_Signed;
        case DXGI_FORMAT_R16G16B16A16_SINT: return Color::RGBA16_Signed;

        case DXGI_FORMAT_R16_UINT: return Color::R16_Unsigned;
        case DXGI_FORMAT_R16G16_UINT: return Color::RG16_Unsigned;
        case DXGI_FORMAT_R16G16B16A16_UINT: return Color::RGBA16_Unsigned;

        case DXGI_FORMAT_R32_FLOAT: return Color::R32_Float;
        case DXGI_FORMAT_R32G32_FLOAT: return Color::RG32_Float;
        case DXGI_FORMAT_R32G32B32_FLOAT: return Color::RGB32_Float;
        case DXGI_FORMAT_R32G32B32A32_FLOAT: return Color::RGBA32_Float;

        case DXGI_FORMAT_R32_SINT: return Color::R32_Signed;
        case DXGI_FORMAT_R32G32_SINT: return Color::RG32_Signed;
        case DXGI_FORMAT_R32G32B32_SINT: return Color::RGB32_Signed;
        case DXGI_FORMAT_R32G32B32A32_SINT: return Color::RGBA32_Signed;

        case DXGI_FORMAT_R32_UINT: return Color::R32_Unsigned;
        case DXGI_FORMAT_R32G32_UINT: return Color::RG32_Unsigned;
        case DXGI_FORMAT_R32G32B32_UINT: return Color::RGB32_Unsigned;
        case DXGI_FORMAT_R32G32B32A32_UINT: return Color::RGBA32_Unsigned;

        case DXGI_FORMAT_D24_UNORM_S8_UINT: return DepthStencil::Depth24_Float_Stencil8_Unsigned;
        case DXGI_FORMAT_D32_FLOAT: return DepthStencil::Depth32_Float;

        default:
            assert_format(false, "Unsupported D3D format");
            return TypelessColor::R8;
        }
    }

}
