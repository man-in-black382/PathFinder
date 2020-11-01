#pragma once

#include <d3d12.h>
#include <optional>
#include <variant>
#include <array>

#include "Device.hpp"
#include "ResourceState.hpp"
#include "Heap.hpp"

#include <Geometry/Dimensions.hpp>
#include <Foundation/MemoryUtils.hpp>

#include <glm/vec4.hpp>

namespace HAL
{

    using ColorClearValue = glm::vec4;

    struct DepthStencilClearValue
    {
        float Depth;
        uint8_t Stencil;
    };

    using ClearValue = std::variant<ColorClearValue, DepthStencilClearValue>;

    enum class TypelessColorFormat
    {
        R8, RG8, RGBA8,
        R16, RG16, RGBA16,
        R32, RG32, RGB32, RGBA32
    };

    enum class ColorFormat
    {
        R8_Unsigned_Norm, RG8_Usigned_Norm, RGBA8_Usigned_Norm, RGBA16_Unsigned_Norm,

        BGRA8_Unsigned_Norm,

        R8_Signed, RG8_Signed, RGBA8_Signed,
        R8_Unsigned, RG8_Unsigned, RGBA8_Unsigned,

        R16_Float, RG16_Float, RGBA16_Float,
        R16_Signed, RG16_Signed, RGBA16_Signed,
        R16_Unsigned, RG16_Unsigned, RGBA16_Unsigned,

        R32_Float, RG32_Float, RGB32_Float, RGBA32_Float,
        R32_Signed, RG32_Signed, RGB32_Signed, RGBA32_Signed,
        R32_Unsigned, RG32_Unsigned, RGB32_Unsigned, RGBA32_Unsigned,

        RGB10A2_Unorm,

        // Compressed formats
        BC1_Unsigned_Norm, BC2_Unsigned_Norm, BC3_Unsigned_Norm, BC4_Unsigned_Norm,
        BC5_Unsigned_Norm, BC5_Signed_Norm, BC7_Unsigned_Norm
    };

    enum class ColorSpace
    {
        Rec709, Rec2020
    };

    enum class DepthStencilFormat
    {
        Depth24_Float_Stencil8_Unsigned, Depth32_Float
    };

    enum class TextureKind { Texture1D, Texture2D, Texture3D };

    using FormatVariant = std::variant<TypelessColorFormat, ColorFormat, DepthStencilFormat>;

    DXGI_FORMAT D3DFormat(TypelessColorFormat type);
    DXGI_FORMAT D3DFormat(ColorFormat type);
    DXGI_FORMAT D3DFormat(DepthStencilFormat type);
    DXGI_FORMAT D3DFormat(FormatVariant type);
    DXGI_COLOR_SPACE_TYPE D3DColorSpace(ColorSpace space);

    std::pair<DXGI_FORMAT, std::optional<DXGI_FORMAT>> D3DDepthStecilShaderAccessFormats(DepthStencilFormat type);

    FormatVariant FormatFromD3DFormat(DXGI_FORMAT format);
    ColorSpace ColorSpaceFromD3DSpace(DXGI_COLOR_SPACE_TYPE space);



    struct TextureProperties
    {
        FormatVariant Format;
        TextureKind Kind;
        Geometry::Dimensions Dimensions;
        ClearValue OptimizedClearValue;
        ResourceState InitialStateMask;
        ResourceState ExpectedStateMask;
        uint32_t MipCount;

        TextureProperties(FormatVariant format, TextureKind kind, const Geometry::Dimensions& dimensions,
            const ClearValue& optimizedClearValue, ResourceState initialStateMask, ResourceState expectedStateMask, uint32_t mipCount = 1);

        TextureProperties(FormatVariant format, TextureKind kind, const Geometry::Dimensions& dimensions,
            ResourceState initialStateMask, ResourceState expectedStateMask, uint32_t mipCount = 1);

        TextureProperties(FormatVariant format, TextureKind kind, const Geometry::Dimensions& dimensions,
            const ClearValue& optimizedClearValue, ResourceState initialStateMask, uint32_t mipCount = 1);

        TextureProperties(FormatVariant format, TextureKind kind, const Geometry::Dimensions& dimensions,
            ResourceState initialStateMask, uint32_t mipCount = 1);

        Geometry::Dimensions MipSize(uint8_t mip) const;
    };



    struct BufferProperties
    {
        uint64_t Size = 1;
        uint64_t Stride = 1;
        ResourceState InitialStateMask = ResourceState::Common;
        ResourceState ExpectedStateMask = ResourceState::Common;

        BufferProperties() = delete;

        template <class Element = uint8_t>
        static BufferProperties Create(uint64_t capacity, uint64_t elementAlignment = 1, ResourceState initialStates = ResourceState::Common, ResourceState expectedStates = ResourceState::Common);
    };

    template <class Element>
    BufferProperties BufferProperties::Create(uint64_t capacity, uint64_t elementAlignment, ResourceState initialStates, ResourceState expectedStates)
    {
        BufferProperties properties{};
        properties.Stride = Foundation::MemoryUtils::Align(sizeof(Element), elementAlignment);
        properties.Size = properties.Stride * capacity;
        properties.InitialStateMask = initialStates;
        properties.ExpectedStateMask = expectedStates == ResourceState::Common ? initialStates : expectedStates;

        return properties;
    }



    using ResourcePropertiesVariant = std::variant<TextureProperties, BufferProperties>;



    class ResourceFormat
    {
    public:
        ResourceFormat(const Device* device, const TextureProperties& textureProperties);
        ResourceFormat(const Device* device, const BufferProperties& bufferProperties);

        void SetExpectedStates(ResourceState expectedStates);

    private:
        void ResolveBufferDemensionData(uint64_t byteCount);
        void ResolveTextureDemensionData(TextureKind kind, const Geometry::Dimensions& dimensions, uint8_t mipCount);
        void QueryAllocationInfo();
        void DetermineExpectedUsageFlags(ResourceState expectedStates);
        void DetermineAliasingGroup(ResourceState expectedStates);

        const Device* mDevice;
        D3D12_RESOURCE_DESC mDescription{};
        ResourcePropertiesVariant mResourceProperties;
        HeapAliasingGroup mAliasingGroup = HeapAliasingGroup::Universal;
        uint64_t mSubresourceCount = 1;
        uint64_t mResourceAlignment = 0;
        uint64_t mResourceSizeInBytes = 0;

    public:
        inline const D3D12_RESOURCE_DESC& D3DResourceDescription() const { return mDescription; }
        inline const auto& GetTextureProperties() const { return std::get<TextureProperties>(mResourceProperties); }
        inline const auto& GetBufferProperties() const { return std::get<BufferProperties>(mResourceProperties); }
        inline const auto& ResourceProperties() const { return mResourceProperties; }
        inline auto ResourceAliasingGroup() const { return mAliasingGroup; }
        inline auto SubresourceCount() const { return mSubresourceCount; }
        inline auto ResourceAlighnment() const { return mResourceAlignment; }
        inline auto ResourceSizeInBytes() const { return mResourceSizeInBytes; }
    };

}
