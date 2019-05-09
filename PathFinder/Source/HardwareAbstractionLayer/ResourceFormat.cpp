#include "ResourceFormat.hpp"

namespace HAL
{

    ResourceFormat::ResourceFormat(TypelessColor dataType, BufferKind kind, const Geometry::Dimensions& dimensions)
    {
        mDesc.Format = D3DFormat(dataType);
        ResolveDemensionData(kind, dimensions);
    }

    ResourceFormat::ResourceFormat(Color dataType, BufferKind kind, const Geometry::Dimensions& dimensions)
    {
        mDesc.Format = D3DFormat(dataType);
        ResolveDemensionData(kind, dimensions);
    }

    ResourceFormat::ResourceFormat(DepthStencil dataType, BufferKind kind, const Geometry::Dimensions& dimensions)
    {
        mDesc.Format = D3DFormat(dataType);
        ResolveDemensionData(kind, dimensions);
    }

    ResourceFormat::ResourceFormat(TypelessColor dataType, TextureKind kind, const Geometry::Dimensions& dimensions)
    {
        mDesc.Format = D3DFormat(dataType);
        ResolveDemensionData(kind, dimensions);
    }

    ResourceFormat::ResourceFormat(Color dataType, TextureKind kind, const Geometry::Dimensions& dimensions)
    {
        mDesc.Format = D3DFormat(dataType);
        ResolveDemensionData(kind, dimensions);
    }

    ResourceFormat::ResourceFormat(DepthStencil dataType, TextureKind kind, const Geometry::Dimensions& dimensions)
    {
        mDesc.Format = D3DFormat(dataType);
        ResolveDemensionData(kind, dimensions);
    }

    void ResourceFormat::ResolveDemensionData(BufferKind kind, const Geometry::Dimensions& dimensions)
    {
        mDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        mDesc.Width = dimensions.Width;
    }

    void ResourceFormat::ResolveDemensionData(TextureKind kind, const Geometry::Dimensions& dimensions)
    {
        switch (kind)
        {
        case TextureKind::Texture3D: mDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D; break;
        case TextureKind::Texture2D: mDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; break;
        case TextureKind::Texture1D: mDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D; break;
        }

        mDesc.Height = dimensions.Height;
        mDesc.Width = dimensions.Width;
        mDesc.DepthOrArraySize = dimensions.Depth;
    }

    constexpr DXGI_FORMAT ResourceFormat::D3DFormat(TypelessColor type)
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

    constexpr DXGI_FORMAT ResourceFormat::D3DFormat(Color type)
    {
        switch (type)
        {
        case Color::R8_Signed:         return DXGI_FORMAT_R8_SINT;
        case Color::RG8_Signed:        return DXGI_FORMAT_R8G8_SINT;
        case Color::RGBA8_Signed:      return DXGI_FORMAT_R8G8B8A8_SINT;

        case Color::R8_Unsigned:       return DXGI_FORMAT_R8_UINT;
        case Color::RG8_Unsigned:      return DXGI_FORMAT_R8G8_UINT;
        case Color::RGBA8_Unsigned:    return DXGI_FORMAT_R8G8B8A8_UINT;

        case Color::R16_Float:         return DXGI_FORMAT_R16_FLOAT;
        case Color::RG16_Float:        return DXGI_FORMAT_R16G16_FLOAT;
        case Color::RGBA16_Float:      return DXGI_FORMAT_R16G16B16A16_TYPELESS;

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

    constexpr DXGI_FORMAT ResourceFormat::D3DFormat(DepthStencil type)
    {
        switch (type)
        {
        case DepthStencil::Depth24_Float_Stencil8_Unsigned: return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case DepthStencil::Depth32_Float:                   return DXGI_FORMAT_D32_FLOAT;
        }
    }

}
