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



    struct ResourceStateBaseT {
    public:
        ResourceStateBaseT(D3D12_RESOURCE_STATES state) : mState(state) {}
    private:
        D3D12_RESOURCE_STATES mState;
    public:
        inline D3D12_RESOURCE_STATES D3DState() const { return mState; }
    };



    template <ResourceUsage Usage>
    struct ResourceStateT : ResourceStateBaseT {
        using ResourceStateBaseT::ResourceStateBaseT;
    };

    struct CommonResourceStateT                 : ResourceStateT<ResourceUsage::Read>       { CommonResourceStateT()                : ResourceStateT(D3D12_RESOURCE_STATE_COMMON) {} };
    struct PixelShaderAccessResourceStateT      : ResourceStateT<ResourceUsage::Read>       { PixelShaderAccessResourceStateT()     : ResourceStateT(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {} };
    struct NonPixelShaderAccessResourceStateT   : ResourceStateT<ResourceUsage::Read>       { NonPixelShaderAccessResourceStateT()  : ResourceStateT(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) {} };
    struct UnorderedAccessResourceStateT        : ResourceStateT<ResourceUsage::ReadWrite>  { UnorderedAccessResourceStateT()       : ResourceStateT(D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {} };
    struct GenericReadResourceStateT            : ResourceStateT<ResourceUsage::Read>       { GenericReadResourceStateT()           : ResourceStateT(D3D12_RESOURCE_STATE_GENERIC_READ) {} };
    struct CopySourceResourceStateT             : ResourceStateT<ResourceUsage::Read>       { CopySourceResourceStateT()            : ResourceStateT(D3D12_RESOURCE_STATE_COPY_SOURCE) {} };
    struct CopyDesctinationResourceStateT       : ResourceStateT<ResourceUsage::Write>      { CopyDesctinationResourceStateT()      : ResourceStateT(D3D12_RESOURCE_STATE_COPY_DEST) {} };



    template <ResourceUsage Usage>
    struct BufferResourceStateT : ResourceStateBaseT {
        using ResourceStateBaseT::ResourceStateBaseT;
    };

    struct VertexAndConstantResourceStateT : ResourceStateT<ResourceUsage::Read> { VertexAndConstantResourceStateT() : ResourceStateT(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER) {} };
    struct IndexResourceStateT             : ResourceStateT<ResourceUsage::Read> { IndexResourceStateT()             : ResourceStateT(D3D12_RESOURCE_STATE_INDEX_BUFFER) {} };



    template <ResourceUsage Usage>
    struct TextureResourceStateT : ResourceStateBaseT {
        using ResourceStateBaseT::ResourceStateBaseT;
    };

    struct PresentTextureResourceStateT      : ResourceStateT<ResourceUsage::Read>  { PresentTextureResourceStateT()      : ResourceStateT(D3D12_RESOURCE_STATE_PRESENT) {} };
    struct RenderTargetTextureResourceStateT : ResourceStateT<ResourceUsage::Write> { RenderTargetTextureResourceStateT() : ResourceStateT(D3D12_RESOURCE_STATE_RENDER_TARGET) {} };



    template <ResourceUsage Usage>
    struct DepthStencilTextureResourceStateT : ResourceStateBaseT {
        using ResourceStateBaseT::ResourceStateBaseT;
    };

    struct DepthReadResourceStateT  : ResourceStateT<ResourceUsage::Read>  { DepthReadResourceStateT()  : ResourceStateT(D3D12_RESOURCE_STATE_DEPTH_READ) {} };
    struct DepthWriteResourceStateT : ResourceStateT<ResourceUsage::Write> { DepthWriteResourceStateT() : ResourceStateT(D3D12_RESOURCE_STATE_DEPTH_WRITE) {} };



    // Convenience
    using ReadResourceStateT = ResourceStateT<ResourceUsage::Read>;
    using WriteResourceStateT = ResourceStateT<ResourceUsage::Write>;
    using ReadWriteResourceStateT = ResourceStateT<ResourceUsage::ReadWrite>;

    using ReadTextureResourceStateT = std::variant<ResourceStateT<ResourceUsage::Read>, TextureResourceStateT<ResourceUsage::Read>>;
    using WriteTextureResourceStateT = std::variant<ResourceStateT<ResourceUsage::Write>, TextureResourceStateT<ResourceUsage::Write>>;
    using ReadWriteTextureResourceStateT = std::variant<ResourceStateT<ResourceUsage::ReadWrite>, TextureResourceStateT<ResourceUsage::ReadWrite>>;

    using ReadDepthStencilTextureResourceStateT = std::variant<ResourceStateT<ResourceUsage::Read>, DepthStencilTextureResourceStateT<ResourceUsage::Read>>;
    using WriteDepthStencilTextureResourceStateT = std::variant<ResourceStateT<ResourceUsage::Write>, DepthStencilTextureResourceStateT<ResourceUsage::Write>>;
    using ReadWriteDepthStencilTextureResourceStateT = std::variant<ResourceStateT<ResourceUsage::ReadWrite>, DepthStencilTextureResourceStateT<ResourceUsage::ReadWrite>>;

    using ReadBufferResourceStateT = std::variant<ResourceStateT<ResourceUsage::Read>, BufferResourceStateT<ResourceUsage::Read>>;
    using WriteBufferResourceStateT = std::variant<ResourceStateT<ResourceUsage::Write>, BufferResourceStateT<ResourceUsage::Write>>;
    using ReadWriteBufferResourceStateT = std::variant<ResourceStateT<ResourceUsage::ReadWrite>, BufferResourceStateT<ResourceUsage::ReadWrite>>;



    extern CommonResourceStateT CommonResourceState;
    extern PixelShaderAccessResourceStateT PixelShaderAccessResourceState;
    extern NonPixelShaderAccessResourceStateT NonPixelShaderAccessResourceState;
    extern UnorderedAccessResourceStateT UnorderedAccessResourceState;
    extern GenericReadResourceStateT GenericReadResourceState;
    extern CopySourceResourceStateT CopySourceResourceState;
    extern CopyDesctinationResourceStateT CopyDesctinationResourceState;
    extern VertexAndConstantResourceStateT VertexAndConstantResourceState;
    extern IndexResourceStateT IndexResourceState;
    extern PresentTextureResourceStateT PresentResourceState;
    extern RenderTargetTextureResourceStateT RenderTargetResourceState;
    extern DepthReadResourceStateT DepthReadResourceState;
    extern DepthWriteResourceStateT DepthWriteResourceState;
}

