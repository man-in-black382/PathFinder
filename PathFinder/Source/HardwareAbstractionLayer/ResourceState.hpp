#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <cstdint>

#include "../Foundation/BitwiseEnum.hpp"

namespace HAL
{

    enum class ResourceState
    {
        Common, UnorderedAccess, PixelShaderAccess, NonPixelShaderAccess,
        StreamOut, IndirectArgument, CopyDestination, CopySource,
        GenericRead, RaytracingAccelerationStructure, Predication,
        RenderTarget, ResolveDestination, ResolveSource, Present, DepthRead, DepthWrite,
        VertexAndConstantBuffer, IndexBuffer
    };

    D3D12_RESOURCE_STATES D3DResourceState(ResourceState stateMask);

}

ENABLE_BITMASK_OPERATORS(HAL::ResourceState);

