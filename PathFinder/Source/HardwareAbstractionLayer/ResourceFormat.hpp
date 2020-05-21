#pragma once

#include <d3d12.h>
#include <optional>
#include <variant>
#include <array>

#include "Device.hpp"
#include "ResourceState.hpp"
#include "Heap.hpp"

#include "../Geometry/Dimensions.hpp"

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

        // Compressed formats
        BC1_Unsigned_Norm, BC2_Unsigned_Norm, BC3_Unsigned_Norm, BC4_Unsigned_Norm,
        BC5_Unsigned_Norm, BC5_Signed_Norm, BC7_Unsigned_Norm
    };

    enum class DepthStencilFormat
    {
        Depth24_Float_Stencil8_Unsigned, Depth32_Float
    };

    enum class BufferKind { Buffer };

    enum class TextureKind { Texture1D, Texture2D, Texture3D };

    class ResourceFormat
    {
    public:
        using FormatVariant = std::variant<TypelessColorFormat, ColorFormat, DepthStencilFormat>;

        ResourceFormat(const Device* device, FormatVariant dataType, TextureKind kind, const Geometry::Dimensions& dimensions, uint16_t mipCount, ClearValue optimizedClearValue);
        ResourceFormat(const Device* device, std::optional<FormatVariant> dataType, BufferKind kind, const Geometry::Dimensions& dimensions);

        void SetExpectedStates(ResourceState expectedStates);

        static DXGI_FORMAT D3DFormat(TypelessColorFormat type);
        static DXGI_FORMAT D3DFormat(ColorFormat type);
        static DXGI_FORMAT D3DFormat(DepthStencilFormat type);

        static std::pair<DXGI_FORMAT, std::optional<DXGI_FORMAT>> D3DDepthStecilShaderAccessFormats(DepthStencilFormat type);

        static FormatVariant FormatFromD3DFormat(DXGI_FORMAT format);

    private:
        using KindVariant = std::variant<BufferKind, TextureKind>;

        void ResolveDemensionData(BufferKind kind, const Geometry::Dimensions& dimensions);
        void ResolveDemensionData(TextureKind kind, const Geometry::Dimensions& dimensions, uint8_t mipCount);
        void QueryAllocationInfo();
        void DetermineExpectedUsageFlags(ResourceState expectedStates);
        void DetermineAliasingGroup(ResourceState expectedStates);

        const Device* mDevice;
        D3D12_RESOURCE_DESC mDescription{};
        std::optional<FormatVariant> mDataType;
        std::optional<ClearValue> mClearValue;
        uint64_t mResourceAlignment = 0;
        uint64_t mResourceSizeInBytes = 0;
        uint64_t mSubresourceCount = 0;
        KindVariant mKind;
        HeapAliasingGroup mAliasingGroup = HeapAliasingGroup::Universal;

    public:
        inline const D3D12_RESOURCE_DESC& D3DResourceDescription() const { return mDescription; }
        inline auto OptimizedClearValue() const { return mClearValue; }
        inline auto ResourceAlighnment() const { return mResourceAlignment; }
        inline auto ResourceSizeInBytes() const { return mResourceSizeInBytes; }
        inline auto SubresourceCount() const { return mSubresourceCount; }
        inline auto DataType() const { return mDataType; }
        inline auto Kind() const { return mKind; }
        inline auto ResourceAliasingGroup() const { return mAliasingGroup; }
    };

}

