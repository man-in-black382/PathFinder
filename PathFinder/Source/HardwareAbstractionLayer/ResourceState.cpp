#include "ResourceState.hpp"

namespace HAL
{

    D3D12_RESOURCE_STATES D3DResourceState(ResourceState stateMask)
    {
        D3D12_RESOURCE_STATES d3dStates = D3D12_RESOURCE_STATE_COMMON;
        ResourceState d = ResourceState::RenderTarget | ResourceState::UnorderedAccess;
        /*  d3dStates |= BitwiseEnumMaskContainsComponent(stateMask, ResourceState::Common) ? D3D12_RESOURCE_STATE_COMMON : d3dStates;
          d3dStates |= BitwiseEnumMaskContainsComponent(stateMask, ResourceState::UnorderedAccess) ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : d3dStates;
          d3dStates |= BitwiseEnumMaskContainsComponent(stateMask, ResourceState::PixelShaderAccess) ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : d3dStates;
          d3dStates |= BitwiseEnumMaskContainsComponent(stateMask, ResourceState::NonPixelShaderAccess) ? D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE : d3dStates;
          d3dStates |= BitwiseEnumMaskContainsComponent(stateMask, ResourceState::StreamOut) ? D3D12_RESOURCE_STATE_STREAM_OUT : d3dStates;
          d3dStates |= BitwiseEnumMaskContainsComponent(stateMask, ResourceState::IndirectArgument) ? D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT : d3dStates;
          d3dStates |= BitwiseEnumMaskContainsComponent(stateMask, ResourceState::CopyDestination) ? D3D12_RESOURCE_STATE_COPY_DEST : d3dStates;
          d3dStates |= BitwiseEnumMaskContainsComponent(stateMask, ResourceState::CopySource) ? D3D12_RESOURCE_STATE_COPY_SOURCE : d3dStates;
          d3dStates |= BitwiseEnumMaskContainsComponent(stateMask, ResourceState::GenericRead) ? D3D12_RESOURCE_STATE_GENERIC_READ : d3dStates;
          d3dStates |= BitwiseEnumMaskContainsComponent(stateMask, ResourceState::RaytracingAccelerationStructure) ? D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE : d3dStates;
          d3dStates |= BitwiseEnumMaskContainsComponent(stateMask, ResourceState::Predication) ? D3D12_RESOURCE_STATE_PREDICATION : d3dStates;
          d3dStates |= BitwiseEnumMaskContainsComponent(stateMask, ResourceState::RenderTarget) ? D3D12_RESOURCE_STATE_RENDER_TARGET : d3dStates;
          d3dStates |= BitwiseEnumMaskContainsComponent(stateMask, ResourceState::ResolveDestination) ? D3D12_RESOURCE_STATE_RESOLVE_DEST : d3dStates;
          d3dStates |= BitwiseEnumMaskContainsComponent(stateMask, ResourceState::ResolveSource) ? D3D12_RESOURCE_STATE_RESOLVE_SOURCE : d3dStates;
          d3dStates |= BitwiseEnumMaskContainsComponent(stateMask, ResourceState::Present) ? D3D12_RESOURCE_STATE_PRESENT : d3dStates;
          d3dStates |= BitwiseEnumMaskContainsComponent(stateMask, ResourceState::DepthRead) ? D3D12_RESOURCE_STATE_DEPTH_READ : d3dStates;
          d3dStates |= BitwiseEnumMaskContainsComponent(stateMask, ResourceState::DepthWrite) ? D3D12_RESOURCE_STATE_DEPTH_WRITE : d3dStates;
          d3dStates |= BitwiseEnumMaskContainsComponent(stateMask, ResourceState::VertexAndConstantBuffer) ? D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER : d3dStates;
          d3dStates |= BitwiseEnumMaskContainsComponent(stateMask, ResourceState::IndexBuffer) ? D3D12_RESOURCE_STATE_INDEX_BUFFER : d3dStates;*/
        return d3dStates;
    }

}

