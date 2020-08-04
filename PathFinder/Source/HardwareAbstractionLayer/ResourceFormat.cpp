#include "ResourceFormat.hpp"

#include "../Foundation/Assert.hpp"
#include "../Foundation/Visitor.hpp"

namespace HAL
{

    TextureProperties::TextureProperties(
        FormatVariant format,
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

    TextureProperties::TextureProperties(
        FormatVariant format,
        TextureKind kind,
        const Geometry::Dimensions& dimensions,
        ResourceState initialStateMask,
        ResourceState expectedStateMask,
        uint16_t mipCount)
        :
        TextureProperties(
            format, kind, dimensions,
            ColorClearValue{ 0, 0, 0, 0 },
            initialStateMask, expectedStateMask, mipCount) {}

    TextureProperties::TextureProperties(
        FormatVariant format,
        TextureKind kind,
        const Geometry::Dimensions& dimensions,
        const ClearValue& optimizedClearValue,
        ResourceState initialStateMask,
        uint16_t mipCount)
        :
        TextureProperties(
            format, kind, dimensions, optimizedClearValue,
            initialStateMask, initialStateMask, mipCount) {}

    TextureProperties::TextureProperties(
        FormatVariant format,
        TextureKind kind,
        const Geometry::Dimensions& dimensions,
        ResourceState initialStateMask,
        uint16_t mipCount)
        :
        TextureProperties(
            format, kind, dimensions,
            ColorClearValue{ 0, 0, 0, 0 },
            initialStateMask, initialStateMask, mipCount) {}

    Geometry::Dimensions TextureProperties::MipSize(uint8_t mip) const
    {
        assert_format(mip <= MipCount, "Mip ", mip, " is out of bounds");
        Geometry::Dimensions mipDimensions = Dimensions.XYZMultiplied(1.0f / powf(2.0f, mip));
        mipDimensions.Depth = std::max(mipDimensions.Depth, 1ull);
        return mipDimensions;
    }



    ResourceFormat::ResourceFormat(const Device* device, const TextureProperties& textureProperties)
        : mDevice{ device }, mResourceProperties{ textureProperties }
    {
        std::visit([this](auto&& t) { mDescription.Format = D3DFormat(t); }, textureProperties.Format);

        ResolveTextureDemensionData(textureProperties.Kind, textureProperties.Dimensions, textureProperties.MipCount);
        SetExpectedStates(textureProperties.InitialStateMask | textureProperties.ExpectedStateMask);
    }

    void ResourceFormat::ResolveBufferDemensionData(uint64_t byteCount)
    {
        mSubresourceCount = 1;

        mDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        mDescription.Width = byteCount;
        mDescription.Height = 1;
        mDescription.DepthOrArraySize = 1;
        mDescription.MipLevels = 1;
        mDescription.SampleDesc.Count = 1;
        mDescription.SampleDesc.Quality = 0;
        mDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    }

    void ResourceFormat::ResolveTextureDemensionData(TextureKind kind, const Geometry::Dimensions& dimensions, uint8_t mipCount)
    {
        switch (kind)
        {
        case TextureKind::Texture3D: mDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D; break;
        case TextureKind::Texture2D: mDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; break;
        case TextureKind::Texture1D: mDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D; break;
        }

        bool isArray = (kind == TextureKind::Texture1D || kind == TextureKind::Texture2D) && dimensions.Depth > 1;
        mSubresourceCount = isArray ? dimensions.Depth * mipCount : mipCount;

        assert_format(!isArray, "Texture arrays are not supported currently. Add proper handling of texture array scheduling first, then remove this assert.");

        mDescription.Height = (UINT)dimensions.Height;
        mDescription.Width = dimensions.Width;
        mDescription.DepthOrArraySize = (UINT16)dimensions.Depth;
        mDescription.SampleDesc.Count = 1;
        mDescription.SampleDesc.Quality = 0;
        mDescription.MipLevels = mipCount;
    }

    void ResourceFormat::SetExpectedStates(ResourceState expectedStates)
    {
        std::visit([expectedStates](auto&& resourceProperties) { resourceProperties.ExpectedStateMask = expectedStates; }, mResourceProperties);

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
            [this](const BufferProperties<uint8_t>& bufferProperties)
            {
                mAliasingGroup = HAL::HeapAliasingGroup::Buffers;
            },
            [this, expectedStates](const TextureProperties& textureProperties)
            {
                if (EnumMaskBitSet(textureProperties.ExpectedStateMask, HAL::ResourceState::RenderTarget) ||
                    EnumMaskBitSet(textureProperties.ExpectedStateMask, HAL::ResourceState::DepthWrite) ||
                    EnumMaskBitSet(textureProperties.ExpectedStateMask, HAL::ResourceState::DepthRead))
                {
                    mAliasingGroup = HAL::HeapAliasingGroup::RTDSTextures;
                }
                else {
                    mAliasingGroup = HAL::HeapAliasingGroup::NonRTDSTextures;
                }
            }),
            mResourceProperties);
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
        case ColorFormat::R8_Unsigned_Norm:      return DXGI_FORMAT_R8_UNORM;
        case ColorFormat::RG8_Usigned_Norm:     return DXGI_FORMAT_R8G8_UNORM;
        case ColorFormat::RGBA8_Usigned_Norm:   return DXGI_FORMAT_R8G8B8A8_UNORM;
        case ColorFormat::RGBA16_Unsigned_Norm: return DXGI_FORMAT_R16G16B16A16_UNORM;

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

        case ColorFormat::BC1_Unsigned_Norm: return DXGI_FORMAT_BC1_UNORM;
        case ColorFormat::BC2_Unsigned_Norm: return DXGI_FORMAT_BC2_UNORM;
        case ColorFormat::BC3_Unsigned_Norm: return DXGI_FORMAT_BC3_UNORM;
        case ColorFormat::BC4_Unsigned_Norm: return DXGI_FORMAT_BC4_UNORM;
        case ColorFormat::BC5_Unsigned_Norm: return DXGI_FORMAT_BC5_UNORM;
        case ColorFormat::BC5_Signed_Norm:   return DXGI_FORMAT_BC5_SNORM;
        case ColorFormat::BC7_Unsigned_Norm: return DXGI_FORMAT_BC7_UNORM;

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

    DXGI_FORMAT ResourceFormat::D3DFormat(FormatVariant type)
    {
        DXGI_FORMAT d3dFormat{};
        std::visit([&d3dFormat](auto&& concreteFormat) {d3dFormat = D3DFormat(concreteFormat); }, type);
        return d3dFormat;
    }

    std::pair<DXGI_FORMAT, std::optional<DXGI_FORMAT>> ResourceFormat::D3DDepthStecilShaderAccessFormats(DepthStencilFormat type)
    {
        switch (type)
        {
        case DepthStencilFormat::Depth24_Float_Stencil8_Unsigned: return { DXGI_FORMAT_R24_UNORM_X8_TYPELESS, DXGI_FORMAT_X24_TYPELESS_G8_UINT };
        case DepthStencilFormat::Depth32_Float:                   return { DXGI_FORMAT_R32_FLOAT, std::nullopt };
        default: assert_format("Should never be hit"); return {};
        }
    }

    FormatVariant ResourceFormat::FormatFromD3DFormat(DXGI_FORMAT format)
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

        case DXGI_FORMAT_R8_UNORM: return ColorFormat::R8_Unsigned_Norm;
        case DXGI_FORMAT_R8G8_UNORM: return ColorFormat::RG8_Usigned_Norm;
        case DXGI_FORMAT_R8G8B8A8_UNORM: return ColorFormat::RGBA8_Usigned_Norm;
        case DXGI_FORMAT_R16G16B16A16_UNORM: return ColorFormat::RGBA16_Unsigned_Norm;

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

            // Compressed formats
        case DXGI_FORMAT_BC1_UNORM: return ColorFormat::BC1_Unsigned_Norm;
        case DXGI_FORMAT_BC2_UNORM: return ColorFormat::BC2_Unsigned_Norm;
        case DXGI_FORMAT_BC3_UNORM: return ColorFormat::BC3_Unsigned_Norm;
        case DXGI_FORMAT_BC4_UNORM: return ColorFormat::BC4_Unsigned_Norm;
        case DXGI_FORMAT_BC5_UNORM: return ColorFormat::BC5_Unsigned_Norm;
        case DXGI_FORMAT_BC5_SNORM: return ColorFormat::BC5_Signed_Norm;
        case DXGI_FORMAT_BC7_UNORM: return ColorFormat::BC7_Unsigned_Norm;

        default:
            assert_format(false, "Unsupported D3D format");
            return TypelessColorFormat::R8;
        }
    }

}
