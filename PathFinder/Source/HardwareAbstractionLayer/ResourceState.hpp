#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <variant>

namespace HAL
{

    //enum class TextureResourceState { RenderTarget, ResolveDestination, ResolveSource, Present };

    //enum class DepthStencilTextureResourceState { Read, Write };

    //enum class BufferResourceState { VertexAndConstant, Index };

    //enum class ResourceState { 
    //    Common, UnorderedAccess, PixelShaderAccess, NonPixelShaderAccess, 
    //    StreamOut, IndirectArgument, CopyDestination, CopySource,
    //    GenericRead, RaytracingAccelerationStructure, ShadingRateSource, Predication 
    //};

    //template <ResourceUsage Usage>
    //struct ResourceState {};
    enum class ResourceUsage { Read, Write, ReadWrite };



    template <ResourceUsage Usage>
    struct ResourceStateT {
        virtual D3D12_RESOURCE_STATES D3DState() {
            static_assert(false, "");
        }
    };

    struct CommonResourceStateT                : ResourceStateT<ResourceUsage::Read>       { D3D12_RESOURCE_STATES D3DState() { return D3D12_RESOURCE_STATE_COMMON; } };
    struct PixelShaderAccessResourceStateT     : ResourceStateT<ResourceUsage::Read>       { D3D12_RESOURCE_STATES D3DState() { return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; } };
    struct NonPixelShaderAccessResourceStateT  : ResourceStateT<ResourceUsage::Read>       { D3D12_RESOURCE_STATES D3DState() { return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE; } };
    struct UnorderedAccessResourceStateT       : ResourceStateT<ResourceUsage::ReadWrite>  { D3D12_RESOURCE_STATES D3DState() { return D3D12_RESOURCE_STATE_UNORDERED_ACCESS; } };
    struct GenericReadResourceStateT           : ResourceStateT<ResourceUsage::Read>       { D3D12_RESOURCE_STATES D3DState() { return D3D12_RESOURCE_STATE_GENERIC_READ; } };
    struct CopySourceResourceStateT            : ResourceStateT<ResourceUsage::Read>       { D3D12_RESOURCE_STATES D3DState() { return D3D12_RESOURCE_STATE_COPY_SOURCE; } };
    struct CopyDesctinationResourceStateT      : ResourceStateT<ResourceUsage::Write>      { D3D12_RESOURCE_STATES D3DState() { return D3D12_RESOURCE_STATE_COPY_DEST; } };



    template <ResourceUsage Usage>
    struct BufferResourceStateT {
        virtual D3D12_RESOURCE_STATES D3DState() {
            static_assert(false, ""); 
        } 
    };

    struct VertexAndConstantResourceStateT : BufferResourceStateT<ResourceUsage::Read> { D3D12_RESOURCE_STATES D3DState() { return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER; } };
    struct IndexResourceStateT             : BufferResourceStateT<ResourceUsage::Read> { D3D12_RESOURCE_STATES D3DState() { return D3D12_RESOURCE_STATE_INDEX_BUFFER; } };



    template <ResourceUsage Usage>
    struct TextureResourceStateT {
        virtual D3D12_RESOURCE_STATES D3DState() {
            static_assert(false, "");
        }
    };

    struct PresentResourceStateT      : TextureResourceStateT<ResourceUsage::Read>  { D3D12_RESOURCE_STATES D3DState() { return D3D12_RESOURCE_STATE_PRESENT; } };
    struct RenderTargetResourceStateT : TextureResourceStateT<ResourceUsage::Write> { D3D12_RESOURCE_STATES D3DState() { return D3D12_RESOURCE_STATE_RENDER_TARGET; } };



    template <ResourceUsage Usage>
    struct DepthStencilTextureResourceStateT {
        virtual D3D12_RESOURCE_STATES D3DState() {
            static_assert(false, "");
        }
    };

    struct DepthReadResourceStateT  : DepthStencilTextureResourceStateT<ResourceUsage::Read>  { D3D12_RESOURCE_STATES D3DState() { return D3D12_RESOURCE_STATE_DEPTH_READ; } };
    struct DepthWriteResourceStateT : DepthStencilTextureResourceStateT<ResourceUsage::Write> { D3D12_RESOURCE_STATES D3DState() { return D3D12_RESOURCE_STATE_DEPTH_WRITE; } };



    // Convenience
    using ReadResourceState = ResourceStateT<ResourceUsage::Read>;
    using WriteResourceState = ResourceStateT<ResourceUsage::Write>;
    using ReadWriteResourceState = ResourceStateT<ResourceUsage::ReadWrite>;

    using ReadTextureResourceState = std::variant<ResourceStateT<ResourceUsage::Read>, TextureResourceStateT<ResourceUsage::Read>>;
    using WriteTextureResourceState = std::variant<ResourceStateT<ResourceUsage::Write>, TextureResourceStateT<ResourceUsage::Write>>;
    using ReadWriteTextureResourceState = std::variant<ResourceStateT<ResourceUsage::ReadWrite>, TextureResourceStateT<ResourceUsage::ReadWrite>>;

    using ReadDepthStencilTextureResourceState = std::variant<ResourceStateT<ResourceUsage::Read>, DepthStencilTextureResourceStateT<ResourceUsage::Read>>;
    using WriteDepthStencilTextureResourceState = std::variant<ResourceStateT<ResourceUsage::Write>, DepthStencilTextureResourceStateT<ResourceUsage::Write>>;
    using ReadWriteDepthStencilTextureResourceState = std::variant<ResourceStateT<ResourceUsage::ReadWrite>, DepthStencilTextureResourceStateT<ResourceUsage::ReadWrite>>;

    using ReadBufferResourceState = std::variant<ResourceStateT<ResourceUsage::Read>, BufferResourceStateT<ResourceUsage::Read>>;
    using WriteBufferResourceState = std::variant<ResourceStateT<ResourceUsage::Write>, BufferResourceStateT<ResourceUsage::Write>>;
    using ReadWriteBufferResourceState = std::variant<ResourceStateT<ResourceUsage::ReadWrite>, BufferResourceStateT<ResourceUsage::ReadWrite>>;
}

