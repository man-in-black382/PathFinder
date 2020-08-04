#include "Texture.hpp"

#include "../Foundation/Assert.hpp"

namespace HAL
{

    Texture::Texture(const Microsoft::WRL::ComPtr<ID3D12Resource>& existingResourcePtr)
        : Resource(existingResourcePtr), mProperties{ ConstructProperties() } {}

    Texture::Texture(const Device& device, const TextureProperties& properties)
        : Resource(device, ResourceFormat{ &device, properties }),
        mProperties{ properties } {}

    Texture::Texture(const Device& device, const Heap& heap, uint64_t heapOffset, const TextureProperties& properties)
        : Resource(device, heap, heapOffset, ResourceFormat{ &device, properties }),
        mProperties{ properties }
    {
        assert_format(!heap.CPUAccessibleType(), "Textures cannot be accessed directly from CPU");
    }
        
    bool Texture::IsArray() const
    {
        switch (mProperties.Kind)
        {
        case TextureKind::Texture1D:
        case TextureKind::Texture2D:
            return mProperties.Dimensions.Depth > 1;

        default:
            return false;
        }
    }

    uint32_t Texture::SubresourceCount() const
    {
        return IsArray() ? mProperties.Dimensions.Depth * mProperties.MipCount : mProperties.MipCount;
    }

    bool Texture::CanImplicitlyPromoteFromCommonStateToState(ResourceState state) const
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

    bool Texture::CanImplicitlyDecayToCommonStateFromState(ResourceState state) const
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

    TextureProperties Texture::ConstructProperties() const
    {
        TextureKind kind{};

        switch (D3DDescription().Dimension)
        {
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D: kind = HAL::TextureKind::Texture1D; break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D: kind = HAL::TextureKind::Texture2D; break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D: kind = HAL::TextureKind::Texture3D; break;
        default: assert_format(false, "Resource is not a texture");
        }

        return {
            ResourceFormat::FormatFromD3DFormat(D3DDescription().Format),
            kind,
            Geometry::Dimensions{ D3DDescription().Width, D3DDescription().Height, D3DDescription().DepthOrArraySize },
            HAL::ResourceState::Common,
            D3DDescription().MipLevels
        };
    }

}
