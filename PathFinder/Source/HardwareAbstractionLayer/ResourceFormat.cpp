#include "ResourceFormat.hpp"

#include "../Foundation/Assert.hpp"
#include "../Foundation/Visitor.hpp"

namespace HAL
{

    ResourceFormat::ResourceFormat(
        const Device* device, FormatVariant dataType, TextureKind kind, 
        const Geometry::Dimensions& dimensions, uint16_t mipCount, ClearValue optimizedClearValue)
        : mDevice{ device }, mDataType{ dataType }, mKind{ kind }
    {
        std::visit([this](auto&& t) { mDescription.Format = D3DFormat(t); }, dataType);
        
        ResolveDemensionData(kind, dimensions);

        mClearValue = D3D12_CLEAR_VALUE{};
        mClearValue->Format = mDescription.Format;

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

        mDescription.MipLevels = mipCount;

        QueryAllocationInfo();
    }

    ResourceFormat::ResourceFormat(const Device* device, std::optional<FormatVariant> dataType, BufferKind kind, const Geometry::Dimensions& dimensions)
        : mDevice{ device }, mDataType{ dataType }, mKind{ kind }
    {
        if (dataType)
        {
            std::visit([this](auto&& t) { mDescription.Format = D3DFormat(t); }, dataType.value());
        }

        ResolveDemensionData(kind, dimensions);
        QueryAllocationInfo();
    }

    void ResourceFormat::ResolveDemensionData(BufferKind kind, const Geometry::Dimensions& dimensions)
    {
        mDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        mDescription.Width = dimensions.Width;
        mDescription.Height = 1;
        mDescription.DepthOrArraySize = 1;
        mDescription.MipLevels = 1;
        mDescription.SampleDesc.Count = 1;
        mDescription.SampleDesc.Quality = 0;
        mDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    }

    void ResourceFormat::ResolveDemensionData(TextureKind kind, const Geometry::Dimensions& dimensions)
    {
        switch (kind)
        {
        case TextureKind::Texture3D: mDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D; break;
        case TextureKind::Texture2D: mDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; break;
        case TextureKind::Texture1D: mDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D; break;
        }

        mDescription.Height = (UINT)dimensions.Height;
        mDescription.Width = dimensions.Width;
        mDescription.DepthOrArraySize = (UINT16)dimensions.Depth;
        mDescription.SampleDesc.Count = 1;
        mDescription.SampleDesc.Quality = 0;
    }

    void ResourceFormat::SetExpectedStates(ResourceState expectedStates)
    {
        DetermineExpectedUsageFlags(expectedStates);
        QueryAllocationInfo();
        DetermineAliasingGroup(expectedStates);
    }

    void ResourceFormat::QueryAllocationInfo()
    {
        if (mDescription.Width == 0)
        {
            return;
        }

        UINT GPUMask = 0;
        D3D12_RESOURCE_ALLOCATION_INFO allocInfo = mDevice->D3DDevice()->GetResourceAllocationInfo(GPUMask, 1, &mDescription);

        mResourceAlignment = allocInfo.Alignment;
        mResourceSizeInBytes = allocInfo.SizeInBytes;
        mDescription.Alignment = mResourceAlignment;
    }

    void ResourceFormat::DetermineExpectedUsageFlags(ResourceState expectedStates)
    {
        if (EnumMaskBitSet(expectedStates, ResourceState::RenderTarget))
        {
            mDescription.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        }

        if (EnumMaskBitSet(expectedStates, ResourceState::DepthRead) ||
            EnumMaskBitSet(expectedStates, ResourceState::DepthWrite))
        {
            mDescription.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

            if (!EnumMaskBitSet(expectedStates, ResourceState::PixelShaderAccess) &&
                !EnumMaskBitSet(expectedStates, ResourceState::NonPixelShaderAccess))
            {
                mDescription.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
            }
        }

        if (EnumMaskBitSet(expectedStates, ResourceState::UnorderedAccess))
        {
            mDescription.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }
    }

    void ResourceFormat::DetermineAliasingGroup(ResourceState expectedStates)
    {
        if (mDevice->SupportsUniversalHeaps())
        {
            mAliasingGroup = HeapAliasingGroup::Universal;
            return;
        }

        std::visit(Foundation::MakeVisitor(
            [this](const HAL::BufferKind& kind)
            {
                mAliasingGroup = HAL::HeapAliasingGroup::Buffers;
            },
            [this, expectedStates](const HAL::TextureKind& kind)
            {
                if (EnumMaskBitSet(expectedStates, HAL::ResourceState::RenderTarget) ||
                    EnumMaskBitSet(expectedStates, HAL::ResourceState::DepthWrite) ||
                    EnumMaskBitSet(expectedStates, HAL::ResourceState::DepthRead))
                {
                    mAliasingGroup = HAL::HeapAliasingGroup::RTDSTextures;
                }
                else {
                    mAliasingGroup = HAL::HeapAliasingGroup::NonRTDSTextures;
                }
            }),
            mKind);
    }

    DXGI_FORMAT ResourceFormat::D3DFormat(TypelessColorFormat type)
    {
        switch (type)
        {
        case TypelessColorFormat::R8:                return DXGI_FORMAT_R8_TYPELESS;
        case TypelessColorFormat::RG8:               return DXGI_FORMAT_R8G8_TYPELESS;
        case TypelessColorFormat::RGBA8:             return DXGI_FORMAT_R8G8B8A8_TYPELESS;
        case TypelessColorFormat::R16:               return DXGI_FORMAT_R16_TYPELESS;
        case TypelessColorFormat::RG16:              return DXGI_FORMAT_R16G16_TYPELESS;
        case TypelessColorFormat::RGBA16:            return DXGI_FORMAT_R16G16B16A16_TYPELESS;
        case TypelessColorFormat::R32:               return DXGI_FORMAT_R32_TYPELESS;
        case TypelessColorFormat::RG32:              return DXGI_FORMAT_R32G32_TYPELESS;
        case TypelessColorFormat::RGB32:             return DXGI_FORMAT_R32G32B32_TYPELESS;
        case TypelessColorFormat::RGBA32:            return DXGI_FORMAT_R32G32B32A32_TYPELESS;
        default: assert_format("Should never be hit"); return DXGI_FORMAT_UNKNOWN;
        }
    }

    DXGI_FORMAT ResourceFormat::D3DFormat(ColorFormat type)
    {
        //DXGI_FORMAT_B8G8R8A8_UNORM = 87,

        switch (type)
        {
        case ColorFormat::R8_Usigned_Norm:     return DXGI_FORMAT_R8_UNORM;
        case ColorFormat::RG8_Usigned_Norm:    return DXGI_FORMAT_R8G8_UNORM;
        case ColorFormat::RGBA8_Usigned_Norm:  return DXGI_FORMAT_R8G8B8A8_UNORM;

        case ColorFormat::BGRA8_Unsigned_Norm: return DXGI_FORMAT_B8G8R8A8_UNORM;

        case ColorFormat::R8_Signed:         return DXGI_FORMAT_R8_SINT;
        case ColorFormat::RG8_Signed:        return DXGI_FORMAT_R8G8_SINT;
        case ColorFormat::RGBA8_Signed:      return DXGI_FORMAT_R8G8B8A8_SINT;

        case ColorFormat::R8_Unsigned:       return DXGI_FORMAT_R8_UINT;
        case ColorFormat::RG8_Unsigned:      return DXGI_FORMAT_R8G8_UINT;
        case ColorFormat::RGBA8_Unsigned:    return DXGI_FORMAT_R8G8B8A8_UINT;

        case ColorFormat::R16_Float:         return DXGI_FORMAT_R16_FLOAT;
        case ColorFormat::RG16_Float:        return DXGI_FORMAT_R16G16_FLOAT;
        case ColorFormat::RGBA16_Float:      return DXGI_FORMAT_R16G16B16A16_FLOAT;

        case ColorFormat::R16_Signed:        return DXGI_FORMAT_R16_SINT;
        case ColorFormat::RG16_Signed:       return DXGI_FORMAT_R16G16_SINT;
        case ColorFormat::RGBA16_Signed:     return DXGI_FORMAT_R16G16B16A16_SINT;

        case ColorFormat::R16_Unsigned:      return DXGI_FORMAT_R16_UINT;
        case ColorFormat::RG16_Unsigned:     return DXGI_FORMAT_R16G16_UINT;
        case ColorFormat::RGBA16_Unsigned:   return DXGI_FORMAT_R16G16B16A16_UINT;

        case ColorFormat::R32_Float:         return DXGI_FORMAT_R32_FLOAT;
        case ColorFormat::RG32_Float:        return DXGI_FORMAT_R32G32_FLOAT;
        case ColorFormat::RGB32_Float:       return DXGI_FORMAT_R32G32B32_FLOAT;
        case ColorFormat::RGBA32_Float:      return DXGI_FORMAT_R32G32B32A32_FLOAT;

        case ColorFormat::R32_Signed:        return DXGI_FORMAT_R32_SINT;
        case ColorFormat::RG32_Signed:       return DXGI_FORMAT_R32G32_SINT;
        case ColorFormat::RGB32_Signed:      return DXGI_FORMAT_R32G32B32_SINT;
        case ColorFormat::RGBA32_Signed:     return DXGI_FORMAT_R32G32B32A32_SINT;

        case ColorFormat::R32_Unsigned:      return DXGI_FORMAT_R32_UINT;
        case ColorFormat::RG32_Unsigned:     return DXGI_FORMAT_R32G32_UINT;
        case ColorFormat::RGB32_Unsigned:    return DXGI_FORMAT_R32G32B32_UINT;
        case ColorFormat::RGBA32_Unsigned:   return DXGI_FORMAT_R32G32B32A32_UINT;

        default: assert_format("Should never be hit"); return DXGI_FORMAT_UNKNOWN;
        }
    }

    DXGI_FORMAT ResourceFormat::D3DFormat(DepthStencilFormat type)
    {
        switch (type)
        {
        case DepthStencilFormat::Depth24_Float_Stencil8_Unsigned: return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case DepthStencilFormat::Depth32_Float:                   return DXGI_FORMAT_D32_FLOAT;
        default: assert_format("Should never be hit"); return DXGI_FORMAT_UNKNOWN;
        }
    }

    std::pair<DXGI_FORMAT, std::optional<DXGI_FORMAT>> ResourceFormat::D3DDepthStecilSRVFormats(DepthStencilFormat type)
    {
        switch (type)
        {
        case DepthStencilFormat::Depth24_Float_Stencil8_Unsigned: return { DXGI_FORMAT_R24_UNORM_X8_TYPELESS, DXGI_FORMAT_X24_TYPELESS_G8_UINT };
        case DepthStencilFormat::Depth32_Float:                   return { DXGI_FORMAT_R32_FLOAT, std::nullopt };
        default: assert_format("Should never be hit"); return {};
        }
    }

    ResourceFormat::FormatVariant ResourceFormat::FormatFromD3DFormat(DXGI_FORMAT format)
    {
        switch (format)
        {
        case DXGI_FORMAT_R8_TYPELESS: return TypelessColorFormat::R8;
        case DXGI_FORMAT_R8G8_TYPELESS: return TypelessColorFormat::RG8;
        case DXGI_FORMAT_R8G8B8A8_TYPELESS: return TypelessColorFormat::RGBA8;
        case DXGI_FORMAT_R16_TYPELESS: return TypelessColorFormat::R16;
        case DXGI_FORMAT_R16G16_TYPELESS: return TypelessColorFormat::RG16;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS: return TypelessColorFormat::RGBA16;
        case DXGI_FORMAT_R32_TYPELESS: return TypelessColorFormat::R32;
        case DXGI_FORMAT_R32G32_TYPELESS: return TypelessColorFormat::RG32;
        case DXGI_FORMAT_R32G32B32_TYPELESS: return TypelessColorFormat::RGB32;
        case DXGI_FORMAT_R32G32B32A32_TYPELESS: return TypelessColorFormat::RGBA32;

        case DXGI_FORMAT_B8G8R8A8_UNORM: return ColorFormat::BGRA8_Unsigned_Norm;

        case DXGI_FORMAT_R8_UNORM: return ColorFormat::R8_Usigned_Norm;
        case DXGI_FORMAT_R8G8_UNORM: return ColorFormat::RG8_Usigned_Norm;
        case DXGI_FORMAT_R8G8B8A8_UNORM: return ColorFormat::RGBA8_Usigned_Norm;

        case DXGI_FORMAT_R8_SINT: return ColorFormat::R8_Signed;
        case DXGI_FORMAT_R8G8_SINT: return ColorFormat::RG8_Signed;
        case DXGI_FORMAT_R8G8B8A8_SINT: return ColorFormat::RGBA8_Signed;

        case DXGI_FORMAT_R8_UINT: return ColorFormat::R8_Unsigned;
        case DXGI_FORMAT_R8G8_UINT: return ColorFormat::RG8_Unsigned;
        case DXGI_FORMAT_R8G8B8A8_UINT: return ColorFormat::RGBA8_Unsigned;

        case DXGI_FORMAT_R16_FLOAT: return ColorFormat::R16_Float;
        case DXGI_FORMAT_R16G16_FLOAT: return ColorFormat::RG16_Float;
        case DXGI_FORMAT_R16G16B16A16_FLOAT: return ColorFormat::RGBA16_Float;

        case DXGI_FORMAT_R16_SINT: return ColorFormat::R16_Signed;
        case DXGI_FORMAT_R16G16_SINT: return ColorFormat::RG16_Signed;
        case DXGI_FORMAT_R16G16B16A16_SINT: return ColorFormat::RGBA16_Signed;

        case DXGI_FORMAT_R16_UINT: return ColorFormat::R16_Unsigned;
        case DXGI_FORMAT_R16G16_UINT: return ColorFormat::RG16_Unsigned;
        case DXGI_FORMAT_R16G16B16A16_UINT: return ColorFormat::RGBA16_Unsigned;

        case DXGI_FORMAT_R32_FLOAT: return ColorFormat::R32_Float;
        case DXGI_FORMAT_R32G32_FLOAT: return ColorFormat::RG32_Float;
        case DXGI_FORMAT_R32G32B32_FLOAT: return ColorFormat::RGB32_Float;
        case DXGI_FORMAT_R32G32B32A32_FLOAT: return ColorFormat::RGBA32_Float;

        case DXGI_FORMAT_R32_SINT: return ColorFormat::R32_Signed;
        case DXGI_FORMAT_R32G32_SINT: return ColorFormat::RG32_Signed;
        case DXGI_FORMAT_R32G32B32_SINT: return ColorFormat::RGB32_Signed;
        case DXGI_FORMAT_R32G32B32A32_SINT: return ColorFormat::RGBA32_Signed;

        case DXGI_FORMAT_R32_UINT: return ColorFormat::R32_Unsigned;
        case DXGI_FORMAT_R32G32_UINT: return ColorFormat::RG32_Unsigned;
        case DXGI_FORMAT_R32G32B32_UINT: return ColorFormat::RGB32_Unsigned;
        case DXGI_FORMAT_R32G32B32A32_UINT: return ColorFormat::RGBA32_Unsigned;

        case DXGI_FORMAT_D24_UNORM_S8_UINT: return DepthStencilFormat::Depth24_Float_Stencil8_Unsigned;
        case DXGI_FORMAT_D32_FLOAT: return DepthStencilFormat::Depth32_Float;

        default:
            assert_format(false, "Unsupported D3D format");
            return TypelessColorFormat::R8;
        }
    }

    bool ResourceFormat::CanResourceImplicitlyPromoteFromCommonStateToState(ResourceState state) const
    {
        bool can = false;

        std::visit(Foundation::MakeVisitor(
            [&can](const BufferKind& kind)
            {
                can = true;
            },
            [&can, state](const TextureKind& kind)
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

                can = EnumMaskBitSet(compatibleStatesMask, state);
            }),
            mKind);

        return can;
    }

    bool ResourceFormat::CanResourceImplicitlyDecayToCommonStateFromState(ResourceState state) const
    {
        bool can = false;

        std::visit(Foundation::MakeVisitor(
            [&can](const BufferKind& kind)
            {
                can = true;
            },
            [&can, state](const TextureKind& kind)
            {
                // Decay rules for Simultaneous-Access Textures are not considered at the moment.
                // Decays to common from any read-only state
                //
                ResourceState compatibleStatesMask =
                    ResourceState::NonPixelShaderAccess |
                    ResourceState::PixelShaderAccess |
                    ResourceState::CopySource;

                can = EnumMaskBitSet(compatibleStatesMask, state);
            }),
            mKind);

        return can;
    }

}
