#include "Texture.hpp"

#include "../Foundation/Assert.hpp"

namespace HAL
{

    Texture::Texture(const Microsoft::WRL::ComPtr<ID3D12Resource>& existingResourcePtr)
        : Resource(existingResourcePtr)
    {
        mFormat = ResourceFormat::FormatFromD3DFormat(D3DDescription().Format);
        mDimensions = { D3DDescription().Width, D3DDescription().Height, D3DDescription().DepthOrArraySize };

        switch (D3DDescription().Dimension)
        {
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D: mKind = HAL::TextureKind::Texture1D; break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D: mKind = HAL::TextureKind::Texture2D; break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D: mKind = HAL::TextureKind::Texture3D; break;
        default: assert_format(false, "Existing resource is not a texture");
        }
    }

    Texture::Texture(const Device& device, const Properties& properties)
        : Resource(device, ConstructResourceFormat(
            &device, properties.Format, properties.Kind, properties.Dimensions, 
            properties.MipCount, properties.OptimizedClearValue), 
            properties.InitialStateMask, properties.ExpectedStateMask),
        mDimensions{ properties.Dimensions }, 
        mKind{ properties.Kind}, 
        mFormat{ properties.Format},
        mOptimizedClearValue{ properties.OptimizedClearValue}, 
        mMipCount{ properties.MipCount } {}

    Texture::Texture(const Device& device, const Heap& heap, uint64_t heapOffset, const Properties& properties)
        : Resource(device, heap, heapOffset, ConstructResourceFormat(
            &device, properties.Format, properties.Kind, properties.Dimensions,
            properties.MipCount, properties.OptimizedClearValue),
            properties.InitialStateMask, properties.ExpectedStateMask),
        mDimensions{ properties.Dimensions },
        mKind{ properties.Kind },
        mFormat{ properties.Format },
        mOptimizedClearValue{ properties.OptimizedClearValue },
        mMipCount{ properties.MipCount } 
    {
        assert_format(!heap.CPUAccessibleType(), "Textures cannot be accessed directly from CPU");
    }
        
    bool Texture::IsArray() const
    {
        switch (mKind)
        {
        case TextureKind::Texture1D:
        case TextureKind::Texture2D:
            return mDimensions.Depth > 1;

        default:
            return false;
        }
    }

    uint32_t Texture::SubresourceCount() const
    {
        return IsArray() ? mDimensions.Depth * mMipCount : mMipCount;
    }

    ResourceFormat Texture::ConstructResourceFormat(
        const Device* device, ResourceFormat::FormatVariant format,
        TextureKind kind, const Geometry::Dimensions& dimensions,
        uint16_t mipCount, const ClearValue& optimizedClearValue)
    {
        return { device, format, kind, dimensions, mipCount, optimizedClearValue };
    }

    Texture::Properties::Properties(
        ResourceFormat::FormatVariant format,
        TextureKind kind,
        const Geometry::Dimensions& dimensions,
        const ClearValue& optimizedClearValue,
        ResourceState initialStateMask,
        ResourceState expectedStateMask,
        uint16_t mipCount)
        :
        Format{ format },
        Kind{ kind },
        Dimensions{ dimensions },
        OptimizedClearValue{ optimizedClearValue },
        InitialStateMask{ initialStateMask },
        ExpectedStateMask{ expectedStateMask },
        MipCount{ mipCount } {}

    Texture::Properties::Properties(
        ResourceFormat::FormatVariant format,
        TextureKind kind,
        const Geometry::Dimensions& dimensions,
        const ClearValue& optimizedClearValue,
        ResourceState initialStateMask,
        uint16_t mipCount)
        :
        Texture::Properties(
            format, kind, dimensions, optimizedClearValue,
            initialStateMask, initialStateMask, mipCount) {}

    Texture::Properties::Properties(
        ResourceFormat::FormatVariant format, 
        TextureKind kind, 
        const Geometry::Dimensions& dimensions, 
        ResourceState initialStateMask, 
        uint16_t mipCount)
        :
        Texture::Properties(
            format, kind, dimensions,
            ColorClearValue{ 0, 0, 0, 0 },
            initialStateMask, initialStateMask, mipCount) {}

}
