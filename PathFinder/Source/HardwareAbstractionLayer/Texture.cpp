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

    Texture::Texture(
        const Device& device, ResourceFormat::FormatVariant format, TextureKind kind,
        const Geometry::Dimensions& dimensions, const ClearValue& optimizedClearValue, 
        ResourceState initialStateMask, ResourceState expectedStateMask, uint16_t mipCount)
        :
        Resource(device, ConstructResourceFormat(&device, format, kind, dimensions, mipCount, optimizedClearValue), initialStateMask, expectedStateMask),
        mDimensions{ dimensions }, mKind{ kind }, mFormat{ format }, mOptimizedClearValue{ optimizedClearValue }, mMipCount{ mipCount } {}

    Texture::Texture(
        const Device& device, const Heap& heap, uint64_t heapOffset, 
        ResourceFormat::FormatVariant format, TextureKind kind, 
        const Geometry::Dimensions& dimensions, 
        const ClearValue& optimizedClearValue, 
        ResourceState initialStateMask, ResourceState expectedStateMask, 
        uint16_t mipCount)
        :
        Resource(device, heap, heapOffset, ConstructResourceFormat(&device, format, kind, dimensions, mipCount, optimizedClearValue), initialStateMask, expectedStateMask),
        mDimensions{ dimensions }, mKind{ kind }, mFormat{ format }, mOptimizedClearValue{ optimizedClearValue }, mMipCount{ mipCount } {}

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

}
