#include "TextureResource.hpp"

#include "../Foundation/Assert.hpp"

namespace HAL
{

    TextureResource::TextureResource(const Microsoft::WRL::ComPtr<ID3D12Resource>& existingResourcePtr) 
        : Resource(existingResourcePtr)
    {
        mFormat = ResourceFormat::FormatFromD3DFormat(D3DDescription().Format);
        mDimensions = { D3DDescription().Width, D3DDescription().Height, D3DDescription().DepthOrArraySize };

        switch (D3DDescription().Dimension)
        {
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D: mKind = HAL::ResourceFormat::TextureKind::Texture1D; break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D: mKind = HAL::ResourceFormat::TextureKind::Texture2D; break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D: mKind = HAL::ResourceFormat::TextureKind::Texture3D; break;
        default: assert_format(false, "Existing resource is not a texture");
        }
    }

    TextureResource::TextureResource(
        const Device& device, ResourceFormat::FormatVariant format, ResourceFormat::TextureKind kind,
        const Geometry::Dimensions& dimensions, const ResourceFormat::ClearValue& optimizedClearValue, 
        ResourceState initialStateMask, ResourceState expectedStateMask, uint16_t mipCount)
        :
        Resource(device, ResourceFormat(format, kind, dimensions, mipCount, optimizedClearValue), initialStateMask, expectedStateMask),
        mDimensions{ dimensions }, mKind{ kind }, mFormat{ format }, mOptimizedClearValue{ optimizedClearValue }, mMipCount{ mipCount } {}

    TextureResource::TextureResource(
        const Device& device, ResourceFormat::FormatVariant format, ResourceFormat::TextureKind kind, 
        const Geometry::Dimensions& dimensions, const ResourceFormat::ClearValue& optimizedClearValue, 
        CPUAccessibleHeapType heapType, uint16_t mipCount)
        :
        Resource(device, ResourceFormat(format, kind, dimensions, mipCount, optimizedClearValue), heapType),
        mDimensions{ dimensions }, mKind{ kind }, mFormat{ format }, mOptimizedClearValue{ optimizedClearValue }, mMipCount{ mipCount } {}

    bool TextureResource::IsArray() const
    {
        switch (mKind)
        {
        case ResourceFormat::TextureKind::Texture1D: 
        case ResourceFormat::TextureKind::Texture2D: 
            return mDimensions.Depth > 1;

        default:
            return false;
        }
    }

    bool TextureResource::CanImplicitlyPromoteFromCommonStateToState(HAL::ResourceState state) const
    {
        // Simultaneous-Access Textures are not considered at the moment.
        // Simultaneous-Access Textures are able to be explicitly promoted 
        // to a larger number of states.
        // Promotes from common to these states:
        //
        ResourceState compatibleStatesMask =
            ResourceState::NonPixelShaderAccess |
            ResourceState::PixelShaderAccess |
            ResourceState::CopyDestination |
            ResourceState::CopySource;

        return EnumMaskBitSet(compatibleStatesMask, state);
    }

    bool TextureResource::CanImplicitlyDecayToCommonStateFromState(HAL::ResourceState state) const
    {
        // Decay rules for Simultaneous-Access Textures are not considered at the moment.
        // Decays to common from any read-only state
        //
        ResourceState compatibleStatesMask =
            ResourceState::NonPixelShaderAccess |
            ResourceState::PixelShaderAccess |
            ResourceState::CopySource;

        return EnumMaskBitSet(compatibleStatesMask, state);
    }

}
