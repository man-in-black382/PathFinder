#include "ResourceState.hpp"

namespace HAL
{

    D3D12_RESOURCE_STATES D3DResourceState(ResourceState state)
    {
        D3D12_RESOURCE_STATES states = D3D12_RESOURCE_STATE_COMMON;
        if (EnumMaskBitSet(state, ResourceState::UnorderedAccess)) states |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        if (EnumMaskBitSet(state, ResourceState::PixelShaderAccess)) states |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        if (EnumMaskBitSet(state, ResourceState::NonPixelShaderAccess)) states |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        if (EnumMaskBitSet(state, ResourceState::StreamOut)) states |= D3D12_RESOURCE_STATE_STREAM_OUT;
        if (EnumMaskBitSet(state, ResourceState::IndirectArgument)) states |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
        if (EnumMaskBitSet(state, ResourceState::CopyDestination)) states |= D3D12_RESOURCE_STATE_COPY_DEST;
        if (EnumMaskBitSet(state, ResourceState::CopySource)) states |= D3D12_RESOURCE_STATE_COPY_SOURCE;
        if (EnumMaskBitSet(state, ResourceState::GenericRead)) states |= D3D12_RESOURCE_STATE_GENERIC_READ;
        if (EnumMaskBitSet(state, ResourceState::RaytracingAccelerationStructure)) states |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
        if (EnumMaskBitSet(state, ResourceState::Predication)) states |= D3D12_RESOURCE_STATE_PREDICATION;
        if (EnumMaskBitSet(state, ResourceState::RenderTarget)) states |= D3D12_RESOURCE_STATE_RENDER_TARGET;
        if (EnumMaskBitSet(state, ResourceState::ResolveDestination)) states |= D3D12_RESOURCE_STATE_RESOLVE_DEST;
        if (EnumMaskBitSet(state, ResourceState::ResolveSource)) states |= D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
        if (EnumMaskBitSet(state, ResourceState::Present)) states |= D3D12_RESOURCE_STATE_PRESENT;
        if (EnumMaskBitSet(state, ResourceState::DepthRead)) states |= D3D12_RESOURCE_STATE_DEPTH_READ;
        if (EnumMaskBitSet(state, ResourceState::DepthWrite)) states |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
        if (EnumMaskBitSet(state, ResourceState::ConstantBuffer)) states |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        return states;
    }

    bool IsResourceStateReadOnly(ResourceState state)
    {
        return (EnumMaskBitSet(state, ResourceState::PixelShaderAccess)) ||
            (EnumMaskBitSet(state, ResourceState::NonPixelShaderAccess)) ||
            (EnumMaskBitSet(state, ResourceState::IndirectArgument)) ||
            (EnumMaskBitSet(state, ResourceState::CopySource)) ||
            (EnumMaskBitSet(state, ResourceState::GenericRead)) ||
            (EnumMaskBitSet(state, ResourceState::RaytracingAccelerationStructure)) ||
            (EnumMaskBitSet(state, ResourceState::Predication)) ||
            //(EnumMaskBitSet(state, ResourceState::ResolveSource)) ||
            (EnumMaskBitSet(state, ResourceState::Present)) ||
            (EnumMaskBitSet(state, ResourceState::DepthRead)) ||
            (EnumMaskBitSet(state, ResourceState::ConstantBuffer));

    }

    bool IsResourceStateUsageSupportedOnGraphicsQueue(ResourceState state)
    {
        return true;
    }

    bool IsResourceStateTransitionsSupportedOnGraphicsQueue(ResourceState state)
    {
        return true;
    }

    bool IsResourceStateUsageSupportedOnComputeQueue(ResourceState state)
    {
        HAL::ResourceState allowedStates =
            ResourceState::Common | 
            ResourceState::AnyShaderAccess |
            ResourceState::GenericRead | 
            ResourceState::CopyDestination |
            ResourceState::CopySource | 
            ResourceState::UnorderedAccess |
            ResourceState::RaytracingAccelerationStructure |
            ResourceState::ConstantBuffer | 
            ResourceState::DepthRead;

        return !EnumMaskBitSet(state, ~allowedStates);
    }

    bool IsResourceStateTransitionSupportedOnComputeQueue(ResourceState state)
    {
        HAL::ResourceState allowedStates =
            ResourceState::Common |
            ResourceState::NonPixelShaderAccess |
            ResourceState::GenericRead |
            ResourceState::CopyDestination |
            ResourceState::CopySource |
            ResourceState::UnorderedAccess |
            ResourceState::RaytracingAccelerationStructure |
            ResourceState::ConstantBuffer;

        return !EnumMaskBitSet(state, ~allowedStates);
    }

    bool IsResourceStateUsageSupportedOnCopyQueue(ResourceState state)
    {
        HAL::ResourceState allowedStates = ResourceState::Common | ResourceState::CopyDestination | ResourceState::CopySource;
        return !EnumMaskBitSet(state, ~allowedStates);
    }

    bool IsResourceStateTransitionSupportedOnCopyQueue(ResourceState state)
    {
        HAL::ResourceState allowedStates = ResourceState::Common | ResourceState::CopyDestination | ResourceState::CopySource;
        return !EnumMaskBitSet(state, ~allowedStates);
    }

}

