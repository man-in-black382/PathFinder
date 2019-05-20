#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <cstdint>

#include "../Foundation/BitwiseEnum.hpp"

namespace HAL
{

    enum class ResourceState : std::underlying_type_t<D3D12_RESOURCE_STATES>
    {
        Common = D3D12_RESOURCE_STATE_COMMON,
        UnorderedAccess = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        PixelShaderAccess = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        NonPixelShaderAccess = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        StreamOut = D3D12_RESOURCE_STATE_STREAM_OUT,
        IndirectArgument = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
        CopyDestination = D3D12_RESOURCE_STATE_COPY_DEST,
        CopySource = D3D12_RESOURCE_STATE_COPY_SOURCE,
        GenericRead = D3D12_RESOURCE_STATE_GENERIC_READ,
        RaytracingAccelerationStructure = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        Predication = D3D12_RESOURCE_STATE_PREDICATION,
        RenderTarget = D3D12_RESOURCE_STATE_RENDER_TARGET,
        ResolveDestination = D3D12_RESOURCE_STATE_RESOLVE_DEST,
        ResolveSource = D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
        Present = D3D12_RESOURCE_STATE_PRESENT,
        DepthRead = D3D12_RESOURCE_STATE_DEPTH_READ,
        DepthWrite = D3D12_RESOURCE_STATE_DEPTH_WRITE,
        VertexBuffer = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        ConstantBuffer = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        IndexBuffer = D3D12_RESOURCE_STATE_INDEX_BUFFER
    };

}

ENABLE_BITMASK_OPERATORS(HAL::ResourceState);

